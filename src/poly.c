#include "poly.h"
#include "expand.h"
#include "eval.h"
#include "symtab.h"
#include "attr.h"
#include "arithmetic.h"
#include "sort.h"
#include "replace.h"
#include "print.h"
#include "match.h"
#include <string.h>
#include <stdlib.h>

bool contains_any_symbol_from(Expr* expr, Expr* var);

/* BasePower logic */

typedef struct {
    Expr* base;
    Expr* exp;
} BasePower;

typedef struct {
    BasePower* data;
    size_t count;
    size_t capacity;
} BPList;

static void bp_init(BPList* l) {
    l->capacity = 8;
    l->count = 0;
    l->data = malloc(sizeof(BasePower) * l->capacity);
}

static void bp_add(BPList* l, Expr* base, Expr* exp) {
    for (size_t i = 0; i < l->count; i++) {
        if (expr_eq(l->data[i].base, base)) {
            Expr* new_exp = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(l->data[i].exp), expr_copy(exp)}, 2));
            expr_free(l->data[i].exp);
            l->data[i].exp = new_exp;
            return;
        }
    }
    if (l->count == l->capacity) {
        l->capacity *= 2;
        l->data = realloc(l->data, sizeof(BasePower) * l->capacity);
    }
    l->data[l->count].base = expr_copy(base);
    l->data[l->count].exp = expr_copy(exp);
    l->count++;
}

static void bp_free(BPList* l) {
    for (size_t i = 0; i < l->count; i++) {
        expr_free(l->data[i].base);
        expr_free(l->data[i].exp);
    }
    free(l->data);
}

static void decompose_to_bp(Expr* e, BPList* l) {
    if (!e) return;
    if (e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL) {
        const char* head = e->data.function.head->data.symbol;
        if (strcmp(head, "Times") == 0) {
            for (size_t i = 0; i < e->data.function.arg_count; i++) decompose_to_bp(e->data.function.args[i], l);
            return;
        }
        if (strcmp(head, "Power") == 0 && e->data.function.arg_count == 2) {
            Expr* base = e->data.function.args[0];
            Expr* exp = e->data.function.args[1];
            if (exp->type == EXPR_INTEGER || is_rational(exp, NULL, NULL)) {
                bp_add(l, base, exp);
                return;
            }
            if (exp->type == EXPR_FUNCTION && exp->data.function.head->type == EXPR_SYMBOL &&
                strcmp(exp->data.function.head->data.symbol, "Plus") == 0) {
                for (size_t i = 0; i < exp->data.function.arg_count; i++) {
                    Expr* sub_exp = exp->data.function.args[i];
                    Expr* sub_p = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(base), expr_copy(sub_exp)}, 2));
                    decompose_to_bp(sub_p, l);
                    expr_free(sub_p);
                }
                return;
            }
        }
    }
    Expr* one = expr_new_integer(1);
    bp_add(l, e, one);
    expr_free(one);
}

static Expr* rebuild_from_bp(BPList* l) {
    if (l->count == 0) return expr_new_integer(1);
    Expr** args = malloc(sizeof(Expr*) * l->count);
    size_t count = 0;
    for (size_t i = 0; i < l->count; i++) {
        if (l->data[i].exp->type == EXPR_INTEGER) {
            int64_t exp = l->data[i].exp->data.integer;
            if (exp == 0) continue;
            if (exp == 1) args[count++] = expr_copy(l->data[i].base);
            else args[count++] = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(l->data[i].base), expr_copy(l->data[i].exp)}, 2));
        } else {
            args[count++] = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(l->data[i].base), expr_copy(l->data[i].exp)}, 2));
        }
    }
    if (count == 0) { free(args); return expr_new_integer(1); }
    if (count == 1) { Expr* res = args[0]; free(args); return res; }
    Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Times"), args, count));
    free(args);
    return res;
}

/* Coefficient logic */

static int64_t get_k(BPList* term_bp, BPList* target_bp) {
    if (target_bp->count == 0) return 0;
    int64_t k_val = -1;
    for (size_t i = 0; i < target_bp->count; i++) {
        Expr* t_base = target_bp->data[i].base;
        Expr* t_exp = target_bp->data[i].exp;
        bool found = false;
        for (size_t j = 0; j < term_bp->count; j++) {
            if (expr_eq(t_base, term_bp->data[j].base)) {
                found = true;
                Expr* trm_exp = term_bp->data[j].exp;
                if (t_exp->type == EXPR_INTEGER && trm_exp->type == EXPR_INTEGER) {
                    if (trm_exp->data.integer % t_exp->data.integer != 0) return 0;
                    int64_t ratio = trm_exp->data.integer / t_exp->data.integer;
                    if (ratio <= 0) return 0;
                    if (k_val == -1) k_val = ratio;
                    else if (k_val != ratio) return 0;
                } else {
                    if (expr_eq(t_exp, trm_exp)) {
                        if (k_val == -1) k_val = 1;
                        else if (k_val != 1) return 0;
                    } else return 0;
                }
                break;
            }
        }
        if (!found) return 0;
    }
    return k_val == -1 ? 0 : k_val;
}

Expr* builtin_coefficient(Expr* res) {
    if (res->type != EXPR_FUNCTION || (res->data.function.arg_count < 2 || res->data.function.arg_count > 3)) return NULL;
    Expr* original_expr = res->data.function.args[0];
    Expr* form = res->data.function.args[1];
    int64_t n = (res->data.function.arg_count == 3 && res->data.function.args[2]->type == EXPR_INTEGER) ? res->data.function.args[2]->data.integer : 1;

    Expr* expanded = expr_expand(original_expr);
    if (!expanded) return NULL;

    BPList target_bp;
    bp_init(&target_bp);
    decompose_to_bp(form, &target_bp);

    size_t term_count = 1;
    Expr** terms = &expanded;
    if (expanded->type == EXPR_FUNCTION && expanded->data.function.head->type == EXPR_SYMBOL && strcmp(expanded->data.function.head->data.symbol, "Plus") == 0) {
        term_count = expanded->data.function.arg_count;
        terms = expanded->data.function.args;
    }

    Expr** coeffs = malloc(sizeof(Expr*) * term_count);
    size_t c_count = 0;

    for (size_t i = 0; i < term_count; i++) {
        Expr* term = terms[i];
        BPList term_bp;
        bp_init(&term_bp);
        decompose_to_bp(term, &term_bp);

        int64_t k = get_k(&term_bp, &target_bp);

        if (k == n) {
            if (n == 0) {
                coeffs[c_count++] = expr_copy(term);
            } else {
                size_t rem_count = 0;
                Expr** rem_args = malloc(sizeof(Expr*) * term_bp.count);
                for (size_t j = 0; j < term_bp.count; j++) {
                    Expr* base = term_bp.data[j].base;
                    Expr* exp = term_bp.data[j].exp;
                    bool matched = false;
                    for (size_t tj = 0; tj < target_bp.count; tj++) {
                        if (expr_eq(base, target_bp.data[tj].base)) {
                            matched = true; break;
                        }
                    }
                    if (!matched) {
                        if (exp->type == EXPR_INTEGER && exp->data.integer == 1) {
                            rem_args[rem_count++] = expr_copy(base);
                        } else {
                            rem_args[rem_count++] = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(base), expr_copy(exp)}, 2));
                        }
                    }
                }
                if (rem_count == 0) coeffs[c_count++] = expr_new_integer(1);
                else if (rem_count == 1) coeffs[c_count++] = rem_args[0];
                else coeffs[c_count++] = eval_and_free(expr_new_function(expr_new_symbol("Times"), rem_args, rem_count));
                free(rem_args);
            }
        }
        bp_free(&term_bp);
    }

    bp_free(&target_bp);
    expr_free(expanded);

    Expr* final_res;
    if (c_count == 0) final_res = expr_new_integer(0);
    else if (c_count == 1) final_res = coeffs[0];
    else final_res = eval_and_free(expr_new_function(expr_new_symbol("Plus"), coeffs, c_count));

    free(coeffs);
    return final_res;
}

/* Constants & Predicates */

static bool contains_symbol(Expr* expr, const char* sym) {
    if (!expr) return false;
    if (expr->type == EXPR_SYMBOL) return strcmp(expr->data.symbol, sym) == 0;
    if (expr->type == EXPR_FUNCTION) {
        if (contains_symbol(expr->data.function.head, sym)) return true;
        for (size_t i = 0; i < expr->data.function.arg_count; i++)
            if (contains_symbol(expr->data.function.args[i], sym)) return true;
    }
    return false;
}

bool contains_any_symbol_from(Expr* expr, Expr* var) {
    if (!expr || !var) return false;
    if (var->type == EXPR_SYMBOL) return contains_symbol(expr, var->data.symbol);
    if (var->type == EXPR_FUNCTION) {
        if (contains_any_symbol_from(expr, var->data.function.head)) return true;
        for (size_t i = 0; i < var->data.function.arg_count; i++)
            if (contains_any_symbol_from(expr, var->data.function.args[i])) return true;
    }
    return false;
}

static bool is_constant_symbol(Expr* e) {
    if (e->type != EXPR_SYMBOL) return false;
    const char* s = e->data.symbol;
    return (strcmp(s, "Pi") == 0 || strcmp(s, "E") == 0 || strcmp(s, "I") == 0 || 
            strcmp(s, "Infinity") == 0 || strcmp(s, "ComplexInfinity") == 0);
}

static bool is_number(Expr* e) {
    if (e->type == EXPR_INTEGER || e->type == EXPR_REAL) return true;
    if (is_rational(e, NULL, NULL)) return true;
    if (is_complex(e, NULL, NULL)) return true;
    return is_constant_symbol(e);
}

void collect_variables(Expr* e, Expr*** vars_ptr, size_t* count, size_t* capacity) {
    if (!e || is_number(e)) return;
    if (e->type == EXPR_FUNCTION) {
        const char* head = (e->data.function.head->type == EXPR_SYMBOL) ? e->data.function.head->data.symbol : "";
        if (strcmp(head, "Plus") == 0 || strcmp(head, "Times") == 0 || strcmp(head, "List") == 0) {
            for (size_t i = 0; i < e->data.function.arg_count; i++) collect_variables(e->data.function.args[i], vars_ptr, count, capacity);
            return;
        }
        if (strcmp(head, "Power") == 0 && e->data.function.arg_count == 2) {
            Expr* base = e->data.function.args[0];
            if (is_number(base)) return;
            if (e->data.function.args[1]->type == EXPR_INTEGER || is_rational(e->data.function.args[1], NULL, NULL)) {
                collect_variables(base, vars_ptr, count, capacity); return;
            }
        }
    }
    for (size_t i = 0; i < *count; i++) if (expr_eq((*vars_ptr)[i], e)) return;
    if (*count == *capacity) { *capacity *= 2; *vars_ptr = realloc(*vars_ptr, sizeof(Expr*) * (*capacity)); }
    (*vars_ptr)[(*count)++] = expr_copy(e);
}

int compare_expr_ptrs(const void* a, const void* b) { return expr_compare(*(Expr**)a, *(Expr**)b); }

Expr* builtin_variables(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    size_t count = 0, capacity = 16;
    Expr** vars = malloc(sizeof(Expr*) * capacity);
    collect_variables(res->data.function.args[0], &vars, &count, &capacity);
    if (count > 0) qsort(vars, count, sizeof(Expr*), compare_expr_ptrs);
    Expr* list = expr_new_function(expr_new_symbol("List"), vars, count);
    free(vars); return list;
}

static bool free_of_any(Expr* expr, Expr** vars, size_t var_count) {
    if (!expr) return true;
    for (size_t i = 0; i < var_count; i++) if (expr_eq(expr, vars[i])) return false;
    if (expr->type == EXPR_FUNCTION) {
        if (!free_of_any(expr->data.function.head, vars, var_count)) return false;
        for (size_t i = 0; i < expr->data.function.arg_count; i++)
            if (!free_of_any(expr->data.function.args[i], vars, var_count)) return false;
    }
    return true;
}

bool is_polynomial(Expr* e, Expr** vars, size_t var_count) {
    if (!e) return false;
    for (size_t i = 0; i < var_count; i++) if (expr_eq(e, vars[i])) return true;
    if (e->type == EXPR_FUNCTION) {
        const char* head = (e->data.function.head->type == EXPR_SYMBOL) ? e->data.function.head->data.symbol : "";
        if (strcmp(head, "Plus") == 0 || strcmp(head, "Times") == 0) {
            for (size_t i = 0; i < e->data.function.arg_count; i++)
                if (!is_polynomial(e->data.function.args[i], vars, var_count)) return false;
            return true;
        }
        if (strcmp(head, "Power") == 0 && e->data.function.arg_count == 2) {
            Expr* exp = e->data.function.args[1];
            if (exp->type == EXPR_INTEGER && exp->data.integer >= 0) return is_polynomial(e->data.function.args[0], vars, var_count);
        }
        if (!free_of_any(e, vars, var_count)) return false;
        for (size_t i = 0; i < var_count; i++) if (contains_any_symbol_from(e, vars[i])) return false;
        return true;
    }
    if (e->type == EXPR_SYMBOL) for (size_t i = 0; i < var_count; i++) if (contains_any_symbol_from(e, vars[i])) return false;
    return true;
}

Expr* builtin_polynomialq(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;
    Expr* vars_expr = res->data.function.args[1];
    Expr** vars = NULL; size_t var_count = 0; bool free_vars = false;
    if (vars_expr->type == EXPR_FUNCTION && vars_expr->data.function.head->type == EXPR_SYMBOL && strcmp(vars_expr->data.function.head->data.symbol, "List") == 0) {
        var_count = vars_expr->data.function.arg_count;
        if (var_count > 0) { vars = malloc(sizeof(Expr*) * var_count); for (size_t i = 0; i < var_count; i++) vars[i] = vars_expr->data.function.args[i]; free_vars = true; }
    } else { vars = &res->data.function.args[1]; var_count = 1; }
    bool poly = is_polynomial(res->data.function.args[0], vars, var_count);
    if (free_vars) free(vars);
    return expr_new_symbol(poly ? "True" : "False");
}

/* GCD Logic */

bool is_zero_poly(Expr* e) {
    if (!e) return true;
    Expr* evaled = evaluate(e);
    bool res = false;
    if (evaled->type == EXPR_INTEGER && evaled->data.integer == 0) res = true;
    if (evaled->type == EXPR_REAL && evaled->data.real == 0.0) res = true;
    expr_free(evaled);
    return res;
}

static bool is_negative(Expr* e) {
    if (!e) return false;
    if (e->type == EXPR_INTEGER && e->data.integer < 0) return true;
    if (e->type == EXPR_REAL && e->data.real < 0.0) return true;
    if (e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL) {
        if (strcmp(e->data.function.head->data.symbol, "Times") == 0 && e->data.function.arg_count > 0) {
            return is_negative(e->data.function.args[0]);
        }
        if (strcmp(e->data.function.head->data.symbol, "Rational") == 0 && e->data.function.arg_count == 2) {
            return is_negative(e->data.function.args[0]);
        }
    }
    return false;
}

static int64_t get_int_content(Expr* e) {
    if (!e) return 1;
    if (e->type == EXPR_INTEGER) return llabs(e->data.integer);
    if (e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL &&
        strcmp(e->data.function.head->data.symbol, "Complex") == 0 && e->data.function.arg_count == 2) {
        return gcd(get_int_content(e->data.function.args[0]), get_int_content(e->data.function.args[1]));
    }
    if (e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL &&
        strcmp(e->data.function.head->data.symbol, "Times") == 0) {
        int64_t c = 1;
        for (size_t i=0; i<e->data.function.arg_count; i++) {
            c *= get_int_content(e->data.function.args[i]);
        }
        return c;
    }
    return 1;
}

static Expr* my_number_gcd(Expr* a, Expr* b) {
    int64_t ca = get_int_content(a);
    int64_t cb = get_int_content(b);
    int64_t g = gcd(ca, cb);
    if (g == 0) g = 1;
    return expr_new_integer(g);
}

int get_degree_poly(Expr* e, Expr* var) {
    if (!e) return 0;
    if (expr_eq(e, var)) return 1;
    if (e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL) {
        const char* head = e->data.function.head->data.symbol;
        if (strcmp(head, "Power") == 0 && e->data.function.arg_count == 2) {
            if (expr_eq(e->data.function.args[0], var)) {
                if (e->data.function.args[1]->type == EXPR_INTEGER) {
                    return (int)e->data.function.args[1]->data.integer;
                }
            }
        } else if (strcmp(head, "Plus") == 0) {
            int max_deg = 0;
            for (size_t i = 0; i < e->data.function.arg_count; i++) {
                int d = get_degree_poly(e->data.function.args[i], var);
                if (d > max_deg) max_deg = d;
            }
            return max_deg;
        } else if (strcmp(head, "Times") == 0) {
            int sum_deg = 0;
            for (size_t i = 0; i < e->data.function.arg_count; i++) {
                sum_deg += get_degree_poly(e->data.function.args[i], var);
            }
            return sum_deg;
        }
    }
    return 0;
}

static Expr* get_coeff(Expr* e, Expr* var, int d) {
    Expr* call = expr_new_function(expr_new_symbol("Coefficient"), (Expr*[]){expr_copy(e), expr_copy(var), expr_new_integer(d)}, 3);
    Expr* res = evaluate(call);
    expr_free(call);
    return res;
}

Expr* exact_poly_div(Expr* A, Expr* B, Expr** vars, size_t var_count) {
    if (var_count == 0) {
        if (is_zero_poly(B)) return NULL;
        return eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(A), eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(B), expr_new_integer(-1)}, 2))}, 2));
    }
    
    Expr* x = vars[var_count - 1];
    Expr* expandedA = expr_expand(A);
    Expr* expandedB = expr_expand(B);
    int degA = get_degree_poly(expandedA, x);
    int degB = get_degree_poly(expandedB, x);
    
    if (degA < degB || is_zero_poly(expandedB)) {
        printf("Early NULL! degA: %d, degB: %d\n", degA, degB);
        expr_free(expandedA); expr_free(expandedB); return NULL;
    }
    
    Expr* Q = expr_new_integer(0);
    Expr* R = expandedA;
    Expr* lcB = get_coeff(expandedB, x, degB);
    
    while (true) {
        int degR = get_degree_poly(R, x);
        if (degR < degB || is_zero_poly(R)) break;
        
        Expr* lcR = get_coeff(R, x, degR);
        int d = degR - degB;
        
        Expr* q_coeff = exact_poly_div(lcR, lcB, vars, var_count - 1);
        if (!q_coeff) { 
            printf("q_coeff is NULL! lcR: "); expr_print_fullform(lcR); printf(", lcB: "); expr_print_fullform(lcB); printf("\n");
            expr_free(lcR); break; 
        }
        
        Expr* x_pow = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(x), expr_new_integer(d)}, 2));
        Expr* term = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){q_coeff, x_pow}, 2));
        
        Expr* new_Q = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(Q), expr_copy(term)}, 2));
        expr_free(Q);
        Q = new_Q;
        
        Expr* term_B = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){term, expr_copy(expandedB)}, 2));
        Expr* neg_term_B = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), term_B}, 2));
        Expr* evaluated_plus = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(R), neg_term_B}, 2));
        Expr* new_R = expr_expand(evaluated_plus);
        expr_free(evaluated_plus);
        
        expr_free(R);
        R = new_R;
        expr_free(lcR);
    }
    expr_free(lcB);
    expr_free(expandedB);
    
    if (!is_zero_poly(R)) {
        printf("R is not zero! R: "); expr_print_fullform(R); printf("\n");
        expr_free(R); expr_free(Q); return NULL;
    }
    expr_free(R);
    return Q;
}

static Expr* pseudo_rem(Expr* A, Expr* B, Expr* x) {
    Expr* R = expr_expand(A);
    Expr* expandedB = expr_expand(B);
    int degB = get_degree_poly(expandedB, x);
    Expr* lcB = get_coeff(expandedB, x, degB);
    
    while (true) {
        int degR = get_degree_poly(R, x);
        if (degR < degB || is_zero_poly(R)) break;
        
        Expr* lcR = get_coeff(R, x, degR);
        int d = degR - degB;
        
        Expr* t1 = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(lcB), R}, 2));
        Expr* x_pow = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(x), expr_new_integer(d)}, 2));
        Expr* t2 = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){lcR, x_pow, expr_copy(expandedB)}, 3));
        Expr* neg_t2 = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), t2}, 2));
        Expr* diff = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){t1, neg_t2}, 2));
        
        R = expr_expand(diff);
        expr_free(diff);
    }
    expr_free(lcB);
    expr_free(expandedB);
    return R;
}

static void poly_div_rem(Expr* p, Expr* q, Expr* x, Expr** out_Q, Expr** out_R) {
    Expr* expandedA = expr_expand(p);
    Expr* expandedB = expr_expand(q);
    int degB = get_degree_poly(expandedB, x);

    if (is_zero_poly(expandedB)) {
        *out_Q = NULL;
        *out_R = NULL;
        expr_free(expandedA);
        expr_free(expandedB);
        return;
    }

    Expr* Q = expr_new_integer(0);
    Expr* R = expandedA;
    Expr* lcB = get_coeff(expandedB, x, degB);

    while (true) {
        int degR = get_degree_poly(R, x);
        if (degR < degB || is_zero_poly(R)) break;

        Expr* lcR = get_coeff(R, x, degR);
        int d = degR - degB;

        Expr* lcB_inv = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(lcB), expr_new_integer(-1)}, 2));
        Expr* q_coeff = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){lcR, lcB_inv}, 2));
        
        Expr* x_pow = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(x), expr_new_integer(d)}, 2));
        Expr* term = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){q_coeff, x_pow}, 2));

        Expr* new_Q = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(Q), expr_copy(term)}, 2));
        expr_free(Q);
        Q = new_Q;

        Expr* term_B = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){term, expr_copy(expandedB)}, 2));
        Expr* neg_term_B = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), term_B}, 2));
        Expr* evaluated_plus = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(R), neg_term_B}, 2));
        Expr* new_R = expr_expand(evaluated_plus);
        expr_free(evaluated_plus);

        expr_free(R);
        R = new_R;
    }
    expr_free(lcB);
    expr_free(expandedB);

    *out_Q = Q;
    *out_R = R;
}

Expr* builtin_polynomialquotient(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 3) return NULL;
    Expr* p = res->data.function.args[0];
    Expr* q = res->data.function.args[1];
    Expr* x = res->data.function.args[2];
    
    Expr *Q, *R;
    poly_div_rem(p, q, x, &Q, &R);
    if (!Q) return NULL;
    expr_free(R);
    Expr* expanded_Q = expr_expand(Q);
    expr_free(Q);
    return expanded_Q;
}

Expr* builtin_polynomialremainder(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 3) return NULL;
    Expr* p = res->data.function.args[0];
    Expr* q = res->data.function.args[1];
    Expr* x = res->data.function.args[2];
    
    Expr *Q, *R;
    poly_div_rem(p, q, x, &Q, &R);
    if (!R) return NULL;
    expr_free(Q);
    return R;
}

Expr* poly_gcd_internal(Expr* A, Expr* B, Expr** vars, size_t var_count);

Expr* poly_content(Expr* A, Expr** vars, size_t var_count) {
    if (var_count == 0) return expr_copy(A);
    Expr* x = vars[var_count - 1];
    Expr* expandedA = expr_expand(A);
    int degA = get_degree_poly(expandedA, x);
    Expr* g = expr_new_integer(0);
    for (int i = 0; i <= degA; i++) {
        Expr* c = get_coeff(expandedA, x, i);
        if (!is_zero_poly(c)) {
            if (is_zero_poly(g)) {
                expr_free(g);
                g = c;
            } else {
                Expr* new_g = poly_gcd_internal(g, c, vars, var_count - 1);
                expr_free(g);
                expr_free(c);
                g = new_g;
            }
        } else {
            expr_free(c);
        }
    }
    expr_free(expandedA);
    if (is_zero_poly(g)) {
        expr_free(g); return expr_new_integer(0);
    }
    return g;
}

Expr* poly_gcd_internal(Expr* A, Expr* B, Expr** vars, size_t var_count) {
    if (is_zero_poly(A)) return expr_expand(B);
    if (is_zero_poly(B)) return expr_expand(A);
    
    if (var_count == 0) {
        return my_number_gcd(A, B);
    }
    
    Expr* x = vars[var_count - 1];
    
    Expr* contA = poly_content(A, vars, var_count);
    Expr* ppA = exact_poly_div(A, contA, vars, var_count);
    if (!ppA) ppA = expr_expand(A);
    
    Expr* contB = poly_content(B, vars, var_count);
    Expr* ppB = exact_poly_div(B, contB, vars, var_count);
    if (!ppB) ppB = expr_expand(B);
    
    Expr* contGCD = poly_gcd_internal(contA, contB, vars, var_count - 1);
    expr_free(contA); expr_free(contB);
    
    Expr* U = ppA;
    Expr* V = ppB;
    
    if (get_degree_poly(U, x) < get_degree_poly(V, x)) {
        Expr* tmp = U; U = V; V = tmp;
    }
    
    while (!is_zero_poly(V)) {
        Expr* R = pseudo_rem(U, V, x);
        expr_free(U);
        U = V;
        if (is_zero_poly(R)) {
            expr_free(R);
            V = NULL;
            break;
        }
        Expr* contR = poly_content(R, vars, var_count);
        V = exact_poly_div(R, contR, vars, var_count);
        if (!V) V = expr_copy(R);
        expr_free(R); expr_free(contR);
    }
    if (V) expr_free(V);
    
    // Normalize U to have positive leading coefficient
    Expr* lc = get_coeff(U, x, get_degree_poly(U, x));
    if (is_negative(lc)) {
        Expr* negU = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), expr_copy(U)}, 2));
        expr_free(U);
        U = expr_expand(negU);
        expr_free(negU);
    }
    expr_free(lc);
    
    Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(contGCD), expr_copy(U)}, 2));
    expr_free(contGCD); expr_free(U);
    Expr* expanded_res = expr_expand(res);
    expr_free(res);
    return expanded_res;
}

Expr* builtin_polynomialgcd(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1) return NULL;
    
    size_t count = res->data.function.arg_count;
    BPList* bps = malloc(sizeof(BPList) * count);
    for (size_t i = 0; i < count; i++) {
        bp_init(&bps[i]);
        decompose_to_bp(res->data.function.args[i], &bps[i]);
    }
    
    Expr* numG = expr_new_integer(0);
    for (size_t i = 0; i < count; i++) {
        Expr* num_i = expr_new_integer(1);
        for (size_t j = 0; j < bps[i].count; j++) {
            if (bps[i].data[j].exp->type == EXPR_INTEGER && bps[i].data[j].exp->data.integer == 1) {
                if (is_number(bps[i].data[j].base)) {
                    Expr* next = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){num_i, expr_copy(bps[i].data[j].base)}, 2));
                    num_i = next;
                    bps[i].data[j].exp->data.integer = 0;
                }
            }
        }
        if (i == 0) {
            expr_free(numG);
            numG = num_i;
        } else {
            Expr* next = my_number_gcd(numG, num_i);
            expr_free(numG);
            expr_free(num_i);
            numG = next;
        }
    }
    
    Expr** common_args = malloc(sizeof(Expr*) * bps[0].count);
    size_t common_count = 0;
    
    for (size_t i = 0; i < bps[0].count; i++) {
        Expr* base = bps[0].data[i].base;
        if (bps[0].data[i].exp->type != EXPR_INTEGER || bps[0].data[i].exp->data.integer == 0) continue;
        
        int64_t min_exp = bps[0].data[i].exp->data.integer;
        bool in_all = true;
        for (size_t k = 1; k < count; k++) {
            bool found = false;
            for (size_t j = 0; j < bps[k].count; j++) {
                if (expr_eq(bps[k].data[j].base, base) && bps[k].data[j].exp->type == EXPR_INTEGER) {
                    found = true;
                    if (bps[k].data[j].exp->data.integer < min_exp) {
                        min_exp = bps[k].data[j].exp->data.integer;
                    }
                    break;
                }
            }
            if (!found) { in_all = false; break; }
        }
        
        if (in_all && min_exp > 0) {
            if (min_exp == 1) {
                common_args[common_count++] = expr_copy(base);
            } else {
                common_args[common_count++] = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(base), expr_new_integer(min_exp)}, 2));
            }
            for (size_t k = 0; k < count; k++) {
                for (size_t j = 0; j < bps[k].count; j++) {
                    if (expr_eq(bps[k].data[j].base, base)) {
                        bps[k].data[j].exp->data.integer -= min_exp;
                        break;
                    }
                }
            }
        }
    }
    
    Expr** rems = malloc(sizeof(Expr*) * count);
    for (size_t i = 0; i < count; i++) {
        rems[i] = rebuild_from_bp(&bps[i]);
        bp_free(&bps[i]);
    }
    free(bps);
    
    size_t v_count = 0, v_cap = 16;
    Expr** vars = malloc(sizeof(Expr*) * v_cap);
    for (size_t i = 0; i < count; i++) {
        collect_variables(rems[i], &vars, &v_count, &v_cap);
    }
    if (v_count > 0) qsort(vars, v_count, sizeof(Expr*), compare_expr_ptrs);
    
    Expr* cur_gcd = rems[0];
    for (size_t i = 1; i < count; i++) {
        cur_gcd = poly_gcd_internal(cur_gcd, rems[i], vars, v_count);
    }
    
    size_t final_count = 0;
    if (!(numG->type == EXPR_INTEGER && numG->data.integer == 1)) final_count++;
    final_count += common_count;
    if (!(cur_gcd->type == EXPR_INTEGER && cur_gcd->data.integer == 1)) final_count++;
    
    if (final_count == 0) {
        free(common_args); free(vars); expr_free(numG);
        for(size_t i=1; i<count; i++) expr_free(rems[i]); free(rems);
        return expr_new_integer(1);
    }
    
    Expr** final_args = malloc(sizeof(Expr*) * final_count);
    size_t idx = 0;
    if (!(numG->type == EXPR_INTEGER && numG->data.integer == 1)) final_args[idx++] = numG;
    else expr_free(numG);
    for (size_t i = 0; i < common_count; i++) final_args[idx++] = common_args[i];
    if (!(cur_gcd->type == EXPR_INTEGER && cur_gcd->data.integer == 1)) final_args[idx++] = cur_gcd;
    else expr_free(cur_gcd);
    
    Expr* result;
    if (idx == 1) result = final_args[0];
    else result = eval_and_free(expr_new_function(expr_new_symbol("Times"), final_args, idx));
    
    free(common_args); 
    for (size_t i = 0; i < v_count; i++) expr_free(vars[i]);
    free(vars);
    for(size_t i=1; i<count; i++) expr_free(rems[i]); free(rems);
    free(final_args);
    return result;
}

static Expr* my_number_lcm(Expr* a, Expr* b) {
    int64_t ca = get_int_content(a);
    int64_t cb = get_int_content(b);
    int64_t l = lcm(ca, cb);
    if (l == 0) l = 1;
    return expr_new_integer(l);
}

Expr* builtin_polynomiallcm(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1) return NULL;
    
    size_t count = res->data.function.arg_count;
    BPList* bps = malloc(sizeof(BPList) * count);
    for (size_t i = 0; i < count; i++) {
        bp_init(&bps[i]);
        decompose_to_bp(res->data.function.args[i], &bps[i]);
    }
    
    Expr* numL = expr_new_integer(1);
    for (size_t i = 0; i < count; i++) {
        Expr* num_i = expr_new_integer(1);
        for (size_t j = 0; j < bps[i].count; j++) {
            if (bps[i].data[j].exp->type == EXPR_INTEGER && bps[i].data[j].exp->data.integer == 1) {
                if (is_number(bps[i].data[j].base)) {
                    Expr* next = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){num_i, expr_copy(bps[i].data[j].base)}, 2));
                    num_i = next;
                    bps[i].data[j].exp->data.integer = 0;
                }
            }
        }
        if (i == 0) {
            expr_free(numL);
            numL = num_i;
        } else {
            Expr* next = my_number_lcm(numL, num_i);
            expr_free(numL);
            expr_free(num_i);
            numL = next;
        }
    }
    
    Expr** common_args = malloc(sizeof(Expr*) * 1024);
    size_t common_count = 0;
    size_t common_cap = 1024;
    
    for (size_t i = 0; i < bps[0].count; i++) {
        Expr* base = bps[0].data[i].base;
        if (bps[0].data[i].exp->type != EXPR_INTEGER || bps[0].data[i].exp->data.integer <= 0) continue;
        
        int64_t min_exp = bps[0].data[i].exp->data.integer;
        bool in_all = true;
        for (size_t k = 1; k < count; k++) {
            bool found = false;
            for (size_t j = 0; j < bps[k].count; j++) {
                if (expr_eq(bps[k].data[j].base, base) && bps[k].data[j].exp->type == EXPR_INTEGER) {
                    found = true;
                    if (bps[k].data[j].exp->data.integer < min_exp) {
                        min_exp = bps[k].data[j].exp->data.integer;
                    }
                    break;
                }
            }
            if (!found) { in_all = false; break; }
        }
        
        if (in_all && min_exp > 0) {
            if (common_count == common_cap) { common_cap *= 2; common_args = realloc(common_args, sizeof(Expr*) * common_cap); }
            if (min_exp == 1) {
                common_args[common_count++] = expr_copy(base);
            } else {
                common_args[common_count++] = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(base), expr_new_integer(min_exp)}, 2));
            }
            for (size_t k = 0; k < count; k++) {
                for (size_t j = 0; j < bps[k].count; j++) {
                    if (expr_eq(bps[k].data[j].base, base)) {
                        bps[k].data[j].exp->data.integer -= min_exp;
                        break;
                    }
                }
            }
        }
    }
    
    // Handle denominators (negative exponents)
    Expr** den_bases = malloc(sizeof(Expr*) * 1024);
    size_t den_bases_count = 0;
    size_t den_bases_cap = 1024;
    
    for (size_t i = 0; i < count; i++) {
        for (size_t j = 0; j < bps[i].count; j++) {
            if (bps[i].data[j].exp->type != EXPR_INTEGER || bps[i].data[j].exp->data.integer >= 0) continue;
            Expr* base = bps[i].data[j].base;
            bool found = false;
            for (size_t k = 0; k < den_bases_count; k++) {
                if (expr_eq(den_bases[k], base)) { found = true; break; }
            }
            if (!found) {
                if (den_bases_count == den_bases_cap) { den_bases_cap *= 2; den_bases = realloc(den_bases, sizeof(Expr*) * den_bases_cap); }
                den_bases[den_bases_count++] = expr_copy(base);
            }
        }
    }
    
    for (size_t i = 0; i < den_bases_count; i++) {
        Expr* base = den_bases[i];
        int64_t max_exp = -INT64_MAX; 
        for (size_t k = 0; k < count; k++) {
            int64_t e = 0;
            for (size_t j = 0; j < bps[k].count; j++) {
                if (expr_eq(bps[k].data[j].base, base) && bps[k].data[j].exp->type == EXPR_INTEGER) {
                    e = bps[k].data[j].exp->data.integer;
                    break;
                }
            }
            if (e > max_exp) max_exp = e;
        }
        
        if (max_exp < 0) {
            if (common_count == common_cap) { common_cap *= 2; common_args = realloc(common_args, sizeof(Expr*) * common_cap); }
            if (max_exp == -1) {
                common_args[common_count++] = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(base), expr_new_integer(-1)}, 2));
            } else {
                common_args[common_count++] = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(base), expr_new_integer(max_exp)}, 2));
            }
        }
        
        for (size_t k = 0; k < count; k++) {
            for (size_t j = 0; j < bps[k].count; j++) {
                if (expr_eq(bps[k].data[j].base, base)) {
                    bps[k].data[j].exp->data.integer = 0; 
                    break;
                }
            }
        }
    }
    
    for (size_t i = 0; i < den_bases_count; i++) expr_free(den_bases[i]);
    free(den_bases);

    Expr** rems = malloc(sizeof(Expr*) * count);
    for (size_t i = 0; i < count; i++) {
        rems[i] = rebuild_from_bp(&bps[i]);
        bp_free(&bps[i]);
    }
    free(bps);
    
    size_t v_count = 0, v_cap = 16;
    Expr** vars = malloc(sizeof(Expr*) * v_cap);
    for (size_t i = 0; i < count; i++) {
        collect_variables(rems[i], &vars, &v_count, &v_cap);
    }
    if (v_count > 0) qsort(vars, v_count, sizeof(Expr*), compare_expr_ptrs);
    
    Expr* cur_lcm = rems[0];
    for (size_t i = 1; i < count; i++) {
        Expr* cur_gcd = poly_gcd_internal(cur_lcm, rems[i], vars, v_count);
        Expr* next_lcm;
        if (cur_gcd->type == EXPR_INTEGER && cur_gcd->data.integer == 1) {
            next_lcm = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(cur_lcm), expr_copy(rems[i])}, 2));
        } else {
            Expr* Q1 = exact_poly_div(cur_lcm, cur_gcd, vars, v_count);
            Expr* Q2 = exact_poly_div(rems[i], cur_gcd, vars, v_count);
            if (!Q1) Q1 = expr_copy(cur_lcm); 
            if (!Q2) Q2 = expr_copy(rems[i]); 
            
            int c1 = (Q1->type == EXPR_FUNCTION && Q1->data.function.head->type == EXPR_SYMBOL && strcmp(Q1->data.function.head->data.symbol, "Plus") == 0) ? Q1->data.function.arg_count : 1;
            int c2 = (Q2->type == EXPR_FUNCTION && Q2->data.function.head->type == EXPR_SYMBOL && strcmp(Q2->data.function.head->data.symbol, "Plus") == 0) ? Q2->data.function.arg_count : 1;
            
            if (c1 <= c2) {
                next_lcm = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){Q1, expr_copy(rems[i])}, 2));
                expr_free(Q2);
            } else {
                next_lcm = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(cur_lcm), Q2}, 2));
                expr_free(Q1);
            }
        }
        expr_free(cur_gcd);
        if (i > 1) expr_free(cur_lcm); 
        cur_lcm = next_lcm;
    }
    
    size_t final_count = 0;
    if (!(numL->type == EXPR_INTEGER && numL->data.integer == 1)) final_count++;
    final_count += common_count;
    if (!(cur_lcm->type == EXPR_INTEGER && cur_lcm->data.integer == 1)) final_count++;
    
    if (final_count == 0) {
        free(common_args); free(vars); expr_free(numL);
        for(size_t i=1; i<count; i++) expr_free(rems[i]); free(rems);
        return expr_new_integer(1);
    }
    
    Expr** final_args = malloc(sizeof(Expr*) * final_count);
    size_t idx = 0;
    if (!(numL->type == EXPR_INTEGER && numL->data.integer == 1)) final_args[idx++] = numL;
    else expr_free(numL);
    for (size_t i = 0; i < common_count; i++) final_args[idx++] = common_args[i];
    if (!(cur_lcm->type == EXPR_INTEGER && cur_lcm->data.integer == 1)) final_args[idx++] = cur_lcm;
    else expr_free(cur_lcm);
    
    Expr* result;
    if (idx == 1) result = final_args[0];
    else result = eval_and_free(expr_new_function(expr_new_symbol("Times"), final_args, idx));
    
    free(common_args); 
    for (size_t i = 0; i < v_count; i++) expr_free(vars[i]);
    free(vars);
    for(size_t i=1; i<count; i++) expr_free(rems[i]); free(rems);
    free(final_args);
    return result;
}


static bool is_threadable_head(Expr* head) {
    if (head->type != EXPR_SYMBOL) return false;
    const char* s = head->data.symbol;
    return strcmp(s, "List") == 0 || strcmp(s, "Equal") == 0 || strcmp(s, "Unequal") == 0 ||
           strcmp(s, "Less") == 0 || strcmp(s, "LessEqual") == 0 || strcmp(s, "Greater") == 0 ||
           strcmp(s, "GreaterEqual") == 0 || strcmp(s, "And") == 0 || strcmp(s, "Or") == 0 ||
           strcmp(s, "Not") == 0 || strcmp(s, "SameQ") == 0 || strcmp(s, "UnsameQ") == 0;
}

typedef struct {
    Expr* base;
    Expr* exp;
    Expr** coeffs;
    size_t coeff_count;
    size_t coeff_cap;
} CollectGroup;

static Expr* collect_internal(Expr* expr, Expr** vars, size_t num_vars, size_t var_idx, Expr* h) {
    if (expr->type == EXPR_FUNCTION && is_threadable_head(expr->data.function.head)) {
        size_t count = expr->data.function.arg_count;
        Expr** new_args = malloc(sizeof(Expr*) * count);
        for (size_t i = 0; i < count; i++) {
            new_args[i] = collect_internal(expr->data.function.args[i], vars, num_vars, var_idx, h);
        }
        return eval_and_free(expr_new_function(expr_copy(expr->data.function.head), new_args, count));
    }

    if (var_idx == num_vars) {
        if (h) {
            return eval_and_free(expr_new_function(expr_copy(h), (Expr*[]){expr_copy(expr)}, 1));
        }
        return expr_copy(expr);
    }

    Expr* var = vars[var_idx];
    Expr* expanded = expr_expand_patt(expr, var);
    if (!expanded) return expr_copy(expr);

    size_t term_count = 1;
    Expr** terms = &expanded;
    if (expanded->type == EXPR_FUNCTION && expanded->data.function.head->type == EXPR_SYMBOL && strcmp(expanded->data.function.head->data.symbol, "Plus") == 0) {
        term_count = expanded->data.function.arg_count;
        terms = expanded->data.function.args;
    }

    size_t group_cap = 16;
    size_t group_count = 0;
    CollectGroup* groups = malloc(sizeof(CollectGroup) * group_cap);

    for (size_t i = 0; i < term_count; i++) {
        Expr* term = terms[i];
        BPList term_bp;
        bp_init(&term_bp);
        decompose_to_bp(term, &term_bp);

        Expr* match_base = NULL;
        Expr* match_exp = NULL;
        int matched_idx = -1;

        for (size_t j = 0; j < term_bp.count; j++) {
            MatchEnv* env = env_new();
            if (match(term_bp.data[j].base, var, env)) {
                match_base = expr_copy(term_bp.data[j].base);
                match_exp = expr_copy(term_bp.data[j].exp);
                matched_idx = j;
                env_free(env);
                break;
            }
            env_free(env);
        }

        Expr* coeff = NULL;
        if (matched_idx == -1) {
            coeff = rebuild_from_bp(&term_bp);
        } else {
            expr_free(term_bp.data[matched_idx].base);
            expr_free(term_bp.data[matched_idx].exp);
            for (size_t j = matched_idx; j < term_bp.count - 1; j++) {
                term_bp.data[j] = term_bp.data[j + 1];
            }
            term_bp.count--;
            coeff = rebuild_from_bp(&term_bp);
        }
        bp_free(&term_bp);

        if (!match_exp) match_exp = expr_new_integer(0);

        int found = -1;
        for (size_t j = 0; j < group_count; j++) {
            bool base_eq = (!match_base && !groups[j].base) || (match_base && groups[j].base && expr_eq(match_base, groups[j].base));
            bool exp_eq = expr_eq(match_exp, groups[j].exp);
            if (base_eq && exp_eq) {
                found = j;
                break;
            }
        }

        if (found == -1) {
            if (group_count == group_cap) {
                group_cap *= 2;
                groups = realloc(groups, sizeof(CollectGroup) * group_cap);
            }
            groups[group_count].base = match_base; 
            groups[group_count].exp = match_exp;   
            groups[group_count].coeff_cap = 4;
            groups[group_count].coeff_count = 0;
            groups[group_count].coeffs = malloc(sizeof(Expr*) * 4);
            found = group_count;
            group_count++;
        } else {
            if (match_base) expr_free(match_base);
            expr_free(match_exp);
        }

        if (groups[found].coeff_count == groups[found].coeff_cap) {
            groups[found].coeff_cap *= 2;
            groups[found].coeffs = realloc(groups[found].coeffs, sizeof(Expr*) * groups[found].coeff_cap);
        }
        groups[found].coeffs[groups[found].coeff_count++] = coeff;
    }

    Expr** final_terms = malloc(sizeof(Expr*) * group_count);
    size_t final_count = 0;

    for (size_t i = 0; i < group_count; i++) {
        Expr* coeff_sum = NULL;
        if (groups[i].coeff_count == 1) {
            coeff_sum = expr_copy(groups[i].coeffs[0]);
        } else {
            Expr** c_args = malloc(sizeof(Expr*) * groups[i].coeff_count);
            for (size_t j = 0; j < groups[i].coeff_count; j++) c_args[j] = expr_copy(groups[i].coeffs[j]);
            coeff_sum = eval_and_free(expr_new_function(expr_new_symbol("Plus"), c_args, groups[i].coeff_count));
            free(c_args);
        }

        Expr* collected_coeff = collect_internal(coeff_sum, vars, num_vars, var_idx + 1, h);
        expr_free(coeff_sum);

        Expr* term = NULL;
        if (!groups[i].base || (groups[i].exp->type == EXPR_INTEGER && groups[i].exp->data.integer == 0)) {
            term = collected_coeff;
        } else {
            Expr* power = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(groups[i].base), expr_copy(groups[i].exp)}, 2));
            term = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){collected_coeff, power}, 2));
        }

        final_terms[final_count++] = term;

        if (groups[i].base) expr_free(groups[i].base);
        expr_free(groups[i].exp);
        for (size_t j = 0; j < groups[i].coeff_count; j++) {
            expr_free(groups[i].coeffs[j]);
        }
        free(groups[i].coeffs);
    }
    free(groups);

    Expr* result = NULL;
    if (final_count == 0) {
        result = expr_new_integer(0);
    } else if (final_count == 1) {
        result = final_terms[0];
    } else {
        result = eval_and_free(expr_new_function(expr_new_symbol("Plus"), final_terms, final_count));
    }

    free(final_terms);
    expr_free(expanded);
    return result;
}

Expr* builtin_collect(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 2 || res->data.function.arg_count > 3) return NULL;
    Expr* expr = res->data.function.args[0];
    Expr* vars_expr = res->data.function.args[1];
    Expr* h = (res->data.function.arg_count == 3) ? res->data.function.args[2] : NULL;

    size_t num_vars = 1;
    Expr** vars = &vars_expr;

    if (vars_expr->type == EXPR_FUNCTION && vars_expr->data.function.head->type == EXPR_SYMBOL && strcmp(vars_expr->data.function.head->data.symbol, "List") == 0) {
        num_vars = vars_expr->data.function.arg_count;
        vars = vars_expr->data.function.args;
    }

    return collect_internal(expr, vars, num_vars, 0, h);
}

static Expr* coeff_list_rec(Expr* expr, Expr** vars, int* max_degrees, size_t num_vars, size_t var_idx) {
    if (var_idx == num_vars) return expr_copy(expr);
    Expr* var = vars[var_idx];
    int d = max_degrees[var_idx];
    Expr* expanded = expr_expand(expr);
    if (!expanded) return expr_copy(expr);
    
    Expr** args = malloc(sizeof(Expr*) * (d + 1));
    for (int i = 0; i <= d; i++) {
        Expr* c = get_coeff(expanded, var, i);
        args[i] = coeff_list_rec(c, vars, max_degrees, num_vars, var_idx + 1);
        expr_free(c);
    }
    expr_free(expanded);
    
    Expr* list = expr_new_function(expr_new_symbol("List"), args, d + 1);
    free(args);
    return list;
}

Expr* builtin_coefficientlist(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;
    Expr* expr = res->data.function.args[0];
    Expr* vars_expr = res->data.function.args[1];

    if (expr->type == EXPR_INTEGER && expr->data.integer == 0) {
        return expr_new_function(expr_new_symbol("List"), NULL, 0);
    }

    size_t num_vars = 1;
    Expr** vars = &vars_expr;

    if (vars_expr->type == EXPR_FUNCTION && vars_expr->data.function.head->type == EXPR_SYMBOL && strcmp(vars_expr->data.function.head->data.symbol, "List") == 0) {
        num_vars = vars_expr->data.function.arg_count;
        vars = vars_expr->data.function.args;
        if (num_vars == 0) return expr_copy(expr);
    }

    Expr* expanded = expr_expand(expr);
    if (!expanded) return expr_copy(expr);

    int* max_degrees = malloc(sizeof(int) * num_vars);
    for (size_t i = 0; i < num_vars; i++) {
        int d = get_degree_poly(expanded, vars[i]);
        max_degrees[i] = (d < 0) ? 0 : d;
    }

    Expr* result = coeff_list_rec(expanded, vars, max_degrees, num_vars, 0);
    free(max_degrees);
    expr_free(expanded);
    return result;
}

static Expr* decompose_recursive(Expr* f, Expr* x) {
    Expr* expanded = expr_expand(f);
    int n = get_degree_poly(expanded, x);
    if (n < 2) {
        Expr* res = expr_new_function(expr_new_symbol("List"), (Expr*[]){expr_copy(expanded)}, 1);
        expr_free(expanded);
        return res;
    }

    int d = 0;
    for (int i = 1; i <= n; i++) {
        Expr* c = get_coeff(expanded, x, i);
        if (!is_zero_poly(c)) {
            if (d == 0) d = i;
            else {
                int64_t tmp_d = gcd(d, i);
                d = (int)tmp_d;
            }
        }
        expr_free(c);
    }

    if (d > 1) {
        Expr* H = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(x), expr_new_integer(d)}, 2));
        
        Expr** g_args = malloc(sizeof(Expr*) * (n/d + 1));
        int g_count = 0;
        for (int i = 0; i <= n; i += d) {
            Expr* c = get_coeff(expanded, x, i);
            if (!is_zero_poly(c)) {
                if (i == 0) {
                    g_args[g_count++] = c;
                } else if (i == d) {
                    Expr* t = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){c, expr_copy(x)}, 2));
                    g_args[g_count++] = t;
                } else {
                    Expr* xp = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(x), expr_new_integer(i/d)}, 2));
                    Expr* t = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){c, xp}, 2));
                    g_args[g_count++] = t;
                }
            } else {
                expr_free(c);
            }
        }
        Expr* g;
        if (g_count == 0) g = expr_new_integer(0);
        else if (g_count == 1) g = g_args[0];
        else g = eval_and_free(expr_new_function(expr_new_symbol("Plus"), g_args, g_count));
        free(g_args);

        expr_free(expanded);

        if (expr_eq(g, x)) {
            expr_free(g);
            return expr_new_function(expr_new_symbol("List"), (Expr*[]){H}, 1);
        }

        Expr* Lg = decompose_recursive(g, x);
        expr_free(g);
        
        size_t Lg_count = Lg->data.function.arg_count;
        Expr** L_args = malloc(sizeof(Expr*) * (Lg_count + 1));
        for (size_t i = 0; i < Lg_count; i++) L_args[i] = expr_copy(Lg->data.function.args[i]);
        L_args[Lg_count] = H;
        
        Expr* res = expr_new_function(expr_new_symbol("List"), L_args, Lg_count + 1);
        free(L_args);
        expr_free(Lg);
        return res;
    }

    Expr* a_n = get_coeff(expanded, x, n);
    for (int s = 2; s < n; s++) {
        if (n % s != 0) continue;
        int r = n / s;
        
        Expr* H = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(x), expr_new_integer(s)}, 2));
        
        bool valid = true;
        for (int k = 1; k < s; k++) {
            Expr* temp_E = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(H), expr_new_integer(r)}, 2));
            Expr* E = expr_expand(temp_E);
            expr_free(temp_E);
            Expr* C = get_coeff(E, x, n - k);
            expr_free(E);
            
            Expr* a_nk = get_coeff(expanded, x, n - k);
            
            Expr* temp_num = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){a_nk, eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), expr_copy(a_n), C}, 3))}, 2));
            Expr* num = expr_expand(temp_num);
            expr_free(temp_num);
            
            Expr* temp_den = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(r), expr_copy(a_n)}, 2));
            Expr* den = expr_expand(temp_den);
            expr_free(temp_den);
            
            size_t v_count = 0, v_cap = 16;
            Expr** vars = malloc(sizeof(Expr*) * v_cap);
            collect_variables(num, &vars, &v_count, &v_cap);
            collect_variables(den, &vars, &v_count, &v_cap);
            if (v_count > 0) qsort(vars, v_count, sizeof(Expr*), compare_expr_ptrs);
            
            size_t vx_count = 0;
            Expr** vars_nox = malloc(sizeof(Expr*) * v_count);
            for (size_t i = 0; i < v_count; i++) {
                if (!expr_eq(vars[i], x)) vars_nox[vx_count++] = vars[i];
            }
            
            Expr* c_sk = exact_poly_div(num, den, vars_nox, vx_count);
            
            for (size_t i = 0; i < v_count; i++) expr_free(vars[i]);
            free(vars);
            free(vars_nox);
            expr_free(num);
            expr_free(den);
            
            if (!c_sk) {
                valid = false;
                break;
            }
            
            Expr* term;
            if (s - k == 1) {
                term = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){c_sk, expr_copy(x)}, 2));
            } else {
                Expr* xp = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(x), expr_new_integer(s - k)}, 2));
                term = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){c_sk, xp}, 2));
            }
            Expr* temp_H = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){H, term}, 2));
            Expr* next_H = expr_expand(temp_H);
            expr_free(temp_H);
            H = next_H;
        }
        
        if (valid) {
            Expr* Q = expr_copy(expanded);
            Expr** g_terms = malloc(sizeof(Expr*) * (r + 1));
            int g_count = 0;
            for (int i = 0; i <= r; i++) {
                Expr *new_Q, *Rem;
                poly_div_rem(Q, H, x, &new_Q, &Rem);
                expr_free(Q);
                Q = new_Q;
                if (get_degree_poly(Rem, x) > 0) {
                    expr_free(Rem);
                    valid = false;
                    break;
                }
                g_terms[g_count++] = Rem;
            }
            
            if (valid && is_zero_poly(Q)) {
                expr_free(Q);
                
                Expr** g_args = malloc(sizeof(Expr*) * g_count);
                int actual_g_count = 0;
                for (int i = 0; i < g_count; i++) {
                    if (!is_zero_poly(g_terms[i])) {
                        if (i == 0) {
                            g_args[actual_g_count++] = expr_copy(g_terms[i]);
                        } else if (i == 1) {
                            g_args[actual_g_count++] = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(g_terms[i]), expr_copy(x)}, 2));
                        } else {
                            Expr* xp = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(x), expr_new_integer(i)}, 2));
                            g_args[actual_g_count++] = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(g_terms[i]), xp}, 2));
                        }
                    }
                    expr_free(g_terms[i]);
                }
                free(g_terms);
                
                Expr* g;
                if (actual_g_count == 0) g = expr_new_integer(0);
                else if (actual_g_count == 1) g = g_args[0];
                else g = eval_and_free(expr_new_function(expr_new_symbol("Plus"), g_args, actual_g_count));
                free(g_args);
                
                Expr* Lg = decompose_recursive(g, x);
                Expr* Lh = decompose_recursive(H, x);
                expr_free(g);
                expr_free(H);
                expr_free(expanded);
                expr_free(a_n);
                
                size_t c1 = Lg->data.function.arg_count;
                size_t c2 = Lh->data.function.arg_count;
                Expr** final_args = malloc(sizeof(Expr*) * (c1 + c2));
                for (size_t i = 0; i < c1; i++) final_args[i] = expr_copy(Lg->data.function.args[i]);
                for (size_t i = 0; i < c2; i++) final_args[c1 + i] = expr_copy(Lh->data.function.args[i]);
                
                Expr* res = expr_new_function(expr_new_symbol("List"), final_args, c1 + c2);
                free(final_args);
                expr_free(Lg);
                expr_free(Lh);
                return res;
            } else {
                for (int i = 0; i < g_count; i++) expr_free(g_terms[i]);
                free(g_terms);
                expr_free(Q);
            }
        }
        expr_free(H);
    }
    expr_free(a_n);
    
    Expr* res = expr_new_function(expr_new_symbol("List"), (Expr*[]){expr_copy(expanded)}, 1);
    expr_free(expanded);
    return res;
}

Expr* builtin_decompose(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;
    Expr* poly = res->data.function.args[0];
    Expr* x = res->data.function.args[1];
    
    Expr* pq = eval_and_free(expr_new_function(expr_new_symbol("PolynomialQ"), (Expr*[]){expr_copy(poly), expr_copy(x)}, 2));
    bool is_poly = (pq->type == EXPR_SYMBOL && strcmp(pq->data.symbol, "True") == 0);
    expr_free(pq);
    
    if (!is_poly) {
        return NULL;
    }
    
    return decompose_recursive(poly, x);
}

static Expr* horner_form_rec(Expr* expr, Expr** vars, size_t num_vars) {
    if (num_vars == 0) return expr_copy(expr);
    Expr* v = vars[0];
    
    Expr* expanded = expr_expand(expr);
    
    Expr* pq = eval_and_free(expr_new_function(expr_new_symbol("PolynomialQ"), (Expr*[]){expr_copy(expanded), expr_copy(v)}, 2));
    bool is_poly = (pq->type == EXPR_SYMBOL && strcmp(pq->data.symbol, "True") == 0);
    expr_free(pq);
    
    if (!is_poly) {
        expr_free(expanded);
        return NULL;
    }
    
    Expr* cl = eval_and_free(expr_new_function(expr_new_symbol("CoefficientList"), (Expr*[]){expr_copy(expanded), expr_copy(v)}, 2));
    expr_free(expanded);
    
    if (!cl || cl->type != EXPR_FUNCTION || strcmp(cl->data.function.head->data.symbol, "List") != 0) {
        if (cl) expr_free(cl);
        return NULL;
    }
    
    size_t count = cl->data.function.arg_count;
    if (count == 0) {
        expr_free(cl);
        return expr_new_integer(0);
    }
    
    Expr* H = horner_form_rec(cl->data.function.args[count - 1], vars + 1, num_vars - 1);
    if (!H) {
        expr_free(cl);
        return NULL;
    }
    
    for (int i = (int)count - 2; i >= 0; i--) {
        Expr* c_i = horner_form_rec(cl->data.function.args[i], vars + 1, num_vars - 1);
        if (!c_i) {
            expr_free(cl);
            expr_free(H);
            return NULL;
        }
        
        bool h_zero = is_zero_poly(H);
        
        if (h_zero) {
            expr_free(H);
            H = c_i;
        } else {
            Expr* t = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(v), H}, 2));
            bool c_zero = is_zero_poly(c_i);
            if (c_zero) {
                expr_free(c_i);
                H = t;
            } else {
                H = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){c_i, t}, 2));
            }
        }
    }
    
    expr_free(cl);
    return H;
}

Expr* builtin_hornerform(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1 || res->data.function.arg_count > 3) return NULL;
    
    Expr* expr = res->data.function.args[0];
    
    Expr* num = NULL;
    Expr* den = NULL;
    
    if (expr->type == EXPR_FUNCTION && expr->data.function.head->type == EXPR_SYMBOL && strcmp(expr->data.function.head->data.symbol, "Times") == 0) {
        size_t n_cap = 16, n_count = 0;
        size_t d_cap = 16, d_count = 0;
        Expr** n_args = malloc(sizeof(Expr*) * n_cap);
        Expr** d_args = malloc(sizeof(Expr*) * d_cap);
        
        for (size_t i = 0; i < expr->data.function.arg_count; i++) {
            Expr* arg = expr->data.function.args[i];
            if (arg->type == EXPR_FUNCTION && arg->data.function.head->type == EXPR_SYMBOL && strcmp(arg->data.function.head->data.symbol, "Power") == 0 && arg->data.function.arg_count == 2) {
                Expr* exp = arg->data.function.args[1];
                if ((exp->type == EXPR_INTEGER && exp->data.integer < 0) || 
                    (exp->type == EXPR_FUNCTION && exp->data.function.head->type == EXPR_SYMBOL && strcmp(exp->data.function.head->data.symbol, "Rational") == 0 && exp->data.function.args[0]->data.integer < 0)) {
                    if (d_count == d_cap) { d_cap *= 2; d_args = realloc(d_args, sizeof(Expr*) * d_cap); }
                    if (exp->type == EXPR_INTEGER) {
                        if (exp->data.integer == -1) {
                            d_args[d_count++] = expr_copy(arg->data.function.args[0]);
                        } else {
                            d_args[d_count++] = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(arg->data.function.args[0]), expr_new_integer(-exp->data.integer)}, 2));
                        }
                    } else { 
                        Expr* new_rat = eval_and_free(expr_new_function(expr_new_symbol("Rational"), (Expr*[]){expr_new_integer(-exp->data.function.args[0]->data.integer), expr_copy(exp->data.function.args[1])}, 2));
                        d_args[d_count++] = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(arg->data.function.args[0]), new_rat}, 2));
                    }
                    continue;
                }
            }
            if (n_count == n_cap) { n_cap *= 2; n_args = realloc(n_args, sizeof(Expr*) * n_cap); }
            n_args[n_count++] = expr_copy(arg);
        }
        
        if (n_count == 0) num = expr_new_integer(1);
        else if (n_count == 1) num = n_args[0];
        else num = eval_and_free(expr_new_function(expr_new_symbol("Times"), n_args, n_count));
        
        if (d_count == 0) den = expr_new_integer(1);
        else if (d_count == 1) den = d_args[0];
        else den = eval_and_free(expr_new_function(expr_new_symbol("Times"), d_args, d_count));
        
        free(n_args); free(d_args);
    } else if (expr->type == EXPR_FUNCTION && expr->data.function.head->type == EXPR_SYMBOL && strcmp(expr->data.function.head->data.symbol, "Power") == 0 && expr->data.function.arg_count == 2) {
        Expr* exp = expr->data.function.args[1];
        if (exp->type == EXPR_INTEGER && exp->data.integer < 0) {
            num = expr_new_integer(1);
            if (exp->data.integer == -1) {
                den = expr_copy(expr->data.function.args[0]);
            } else {
                den = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(expr->data.function.args[0]), expr_new_integer(-exp->data.integer)}, 2));
            }
        } else {
            num = expr_copy(expr);
            den = expr_new_integer(1);
        }
    } else {
        num = expr_copy(expr);
        den = expr_new_integer(1);
    }

    Expr* vars1_expr = NULL;
    Expr* vars2_expr = NULL;
    
    if (res->data.function.arg_count == 1) {
        vars1_expr = eval_and_free(expr_new_function(expr_new_symbol("Variables"), (Expr*[]){expr_copy(num)}, 1));
        vars2_expr = eval_and_free(expr_new_function(expr_new_symbol("Variables"), (Expr*[]){expr_copy(den)}, 1));
    } else if (res->data.function.arg_count == 2) {
        vars1_expr = expr_copy(res->data.function.args[1]);
        vars2_expr = expr_copy(res->data.function.args[1]);
    } else if (res->data.function.arg_count == 3) {
        vars1_expr = expr_copy(res->data.function.args[1]);
        vars2_expr = expr_copy(res->data.function.args[2]);
    }
    
    if (vars1_expr && (vars1_expr->type != EXPR_FUNCTION || vars1_expr->data.function.head->type != EXPR_SYMBOL || strcmp(vars1_expr->data.function.head->data.symbol, "List") != 0)) {
        vars1_expr = eval_and_free(expr_new_function(expr_new_symbol("List"), (Expr*[]){vars1_expr}, 1));
    }
    if (vars2_expr && (vars2_expr->type != EXPR_FUNCTION || vars2_expr->data.function.head->type != EXPR_SYMBOL || strcmp(vars2_expr->data.function.head->data.symbol, "List") != 0)) {
        vars2_expr = eval_and_free(expr_new_function(expr_new_symbol("List"), (Expr*[]){vars2_expr}, 1));
    }
    
    Expr** vars1 = vars1_expr ? vars1_expr->data.function.args : NULL;
    size_t num_vars1 = vars1_expr ? vars1_expr->data.function.arg_count : 0;
    
    Expr** vars2 = vars2_expr ? vars2_expr->data.function.args : NULL;
    size_t num_vars2 = vars2_expr ? vars2_expr->data.function.arg_count : 0;
    
    Expr* h_num = horner_form_rec(num, vars1, num_vars1);
    if (!h_num) {
        printf("HornerForm::poly: "); 
        char* s = expr_to_string(expr);
        printf("%s is not a polynomial.\n", s);
        free(s);
        if (vars1_expr) expr_free(vars1_expr);
        if (vars2_expr) expr_free(vars2_expr);
        expr_free(num);
        expr_free(den);
        return expr_copy(res);
    }
    
    Expr* h_den = NULL;
    if (den->type == EXPR_INTEGER && den->data.integer == 1) {
        h_den = expr_copy(den);
    } else {
        h_den = horner_form_rec(den, vars2, num_vars2);
        if (!h_den) {
            printf("HornerForm::poly: "); 
            char* s = expr_to_string(expr);
            printf("%s is not a polynomial.\n", s);
            free(s);
            if (vars1_expr) expr_free(vars1_expr);
            if (vars2_expr) expr_free(vars2_expr);
            expr_free(num);
            expr_free(den);
            expr_free(h_num);
            return expr_copy(res);
        }
    }
    
    Expr* result = NULL;
    if (h_den->type == EXPR_INTEGER && h_den->data.integer == 1) {
        result = h_num;
        expr_free(h_den);
    } else {
        Expr* inv_den = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){h_den, expr_new_integer(-1)}, 2));
        result = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){h_num, inv_den}, 2));
    }
    
    if (vars1_expr) expr_free(vars1_expr);
    if (vars2_expr) expr_free(vars2_expr);
    expr_free(num);
    expr_free(den);
    
    return result;
}

static Expr* resultant_internal(Expr* P, Expr* Q, Expr* var) {
    if (P->type == EXPR_FUNCTION && P->data.function.head->type == EXPR_SYMBOL) {
        if (strcmp(P->data.function.head->data.symbol, "Times") == 0) {
            size_t count = P->data.function.arg_count;
            Expr** args = malloc(sizeof(Expr*) * count);
            for (size_t i = 0; i < count; i++) {
                args[i] = resultant_internal(P->data.function.args[i], Q, var);
            }
            Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Times"), args, count));
            free(args);
            return res;
        } else if (strcmp(P->data.function.head->data.symbol, "Power") == 0 && P->data.function.arg_count == 2) {
            Expr* r = resultant_internal(P->data.function.args[0], Q, var);
            return eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){r, expr_copy(P->data.function.args[1])}, 2));
        }
    }
    
    if (Q->type == EXPR_FUNCTION && Q->data.function.head->type == EXPR_SYMBOL) {
        if (strcmp(Q->data.function.head->data.symbol, "Times") == 0) {
            size_t count = Q->data.function.arg_count;
            Expr** args = malloc(sizeof(Expr*) * count);
            for (size_t i = 0; i < count; i++) {
                args[i] = resultant_internal(P, Q->data.function.args[i], var);
            }
            Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Times"), args, count));
            free(args);
            return res;
        } else if (strcmp(Q->data.function.head->data.symbol, "Power") == 0 && Q->data.function.arg_count == 2) {
            Expr* r = resultant_internal(P, Q->data.function.args[0], var);
            return eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){r, expr_copy(Q->data.function.args[1])}, 2));
        }
    }
    
    Expr* exp_P = expr_expand(P);
    Expr* exp_Q = expr_expand(Q);
    int n = get_degree_poly(exp_P, var);
    int m = get_degree_poly(exp_Q, var);
    
    if (n == 0 && m == 0) {
        expr_free(exp_P); expr_free(exp_Q);
        return expr_new_integer(1);
    }
    if (n == 0) {
        Expr* r = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(exp_P), expr_new_integer(m)}, 2));
        expr_free(exp_P); expr_free(exp_Q);
        return r;
    }
    if (m == 0) {
        Expr* r = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(exp_Q), expr_new_integer(n)}, 2));
        expr_free(exp_P); expr_free(exp_Q);
        return r;
    }
    
    Expr** p_coeffs = malloc(sizeof(Expr*) * (n + 1));
    for (int i = 0; i <= n; i++) p_coeffs[i] = get_coeff(exp_P, var, n - i);
    
    Expr** q_coeffs = malloc(sizeof(Expr*) * (m + 1));
    for (int i = 0; i <= m; i++) q_coeffs[i] = get_coeff(exp_Q, var, m - i);
    
    int dim = n + m;
    Expr** rows = malloc(sizeof(Expr*) * dim);
    for (int i = 0; i < m; i++) {
        Expr** row_elems = malloc(sizeof(Expr*) * dim);
        for (int j = 0; j < dim; j++) {
            if (j >= i && j - i <= n) row_elems[j] = expr_copy(p_coeffs[j - i]);
            else row_elems[j] = expr_new_integer(0);
        }
        rows[i] = expr_new_function(expr_new_symbol("List"), row_elems, dim);
        free(row_elems);
    }
    
    for (int i = 0; i < n; i++) {
        Expr** row_elems = malloc(sizeof(Expr*) * dim);
        for (int j = 0; j < dim; j++) {
            if (j >= i && j - i <= m) row_elems[j] = expr_copy(q_coeffs[j - i]);
            else row_elems[j] = expr_new_integer(0);
        }
        rows[m + i] = expr_new_function(expr_new_symbol("List"), row_elems, dim);
        free(row_elems);
    }
    
    Expr* matrix = expr_new_function(expr_new_symbol("List"), rows, dim);
    free(rows);
    
    Expr* det_call = expr_new_function(expr_new_symbol("Det"), (Expr*[]){matrix}, 1);
    Expr* evaluated_det = evaluate(det_call);
    expr_free(det_call);
    
    Expr* result = expr_expand(evaluated_det);
    expr_free(evaluated_det);
    
    for (int i = 0; i <= n; i++) expr_free(p_coeffs[i]);
    free(p_coeffs);
    for (int i = 0; i <= m; i++) expr_free(q_coeffs[i]);
    free(q_coeffs);
    expr_free(exp_P); expr_free(exp_Q);
    
    return result;
}

Expr* builtin_resultant(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 3) return NULL;
    
    Expr* p1 = res->data.function.args[0];
    Expr* p2 = res->data.function.args[1];
    Expr* var = res->data.function.args[2];
    
    Expr* pq1 = eval_and_free(expr_new_function(expr_new_symbol("PolynomialQ"), (Expr*[]){expr_copy(p1), expr_copy(var)}, 2));
    bool is_poly1 = (pq1->type == EXPR_SYMBOL && strcmp(pq1->data.symbol, "True") == 0);
    expr_free(pq1);
    
    Expr* pq2 = eval_and_free(expr_new_function(expr_new_symbol("PolynomialQ"), (Expr*[]){expr_copy(p2), expr_copy(var)}, 2));
    bool is_poly2 = (pq2->type == EXPR_SYMBOL && strcmp(pq2->data.symbol, "True") == 0);
    expr_free(pq2);
    
    if (!is_poly1 || !is_poly2) {
        return NULL;
    }
    
    return resultant_internal(p1, p2, var);
}

void poly_init(void) {    symtab_add_builtin("PolynomialQ", builtin_polynomialq);
    symtab_get_def("PolynomialQ")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("Variables", builtin_variables);
    symtab_get_def("Variables")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("Coefficient", builtin_coefficient);
    symtab_get_def("Coefficient")->attributes |= ATTR_PROTECTED | ATTR_LISTABLE;
    symtab_add_builtin("CoefficientList", builtin_coefficientlist);
    symtab_get_def("CoefficientList")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("PolynomialGCD", builtin_polynomialgcd);
    symtab_get_def("PolynomialGCD")->attributes |= ATTR_PROTECTED | ATTR_LISTABLE;
    symtab_add_builtin("PolynomialLCM", builtin_polynomiallcm);
    symtab_get_def("PolynomialLCM")->attributes |= ATTR_PROTECTED | ATTR_LISTABLE;
    symtab_add_builtin("PolynomialQuotient", builtin_polynomialquotient);
    symtab_get_def("PolynomialQuotient")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("PolynomialRemainder", builtin_polynomialremainder);
    symtab_get_def("PolynomialRemainder")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("Collect", builtin_collect);
    symtab_get_def("Collect")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("Decompose", builtin_decompose);
    symtab_get_def("Decompose")->attributes |= ATTR_PROTECTED | ATTR_LISTABLE;
    symtab_add_builtin("HornerForm", builtin_hornerform);
    symtab_get_def("HornerForm")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("Resultant", builtin_resultant);
    symtab_get_def("Resultant")->attributes |= ATTR_PROTECTED | ATTR_LISTABLE;
}
