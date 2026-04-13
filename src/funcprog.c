#include "print.h"

#include "funcprog.h"
#include "eval.h"
#include "arithmetic.h"
#include "match.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    int64_t min;
    int64_t max;
    bool heads;
} LevelSpec;

static int64_t get_depth(Expr* e) {
    if (e->type != EXPR_FUNCTION) return 1;
    if (e->data.function.head->type == EXPR_SYMBOL) {
        const char* h = e->data.function.head->data.symbol;
        if (strcmp(h, "Rational") == 0 || strcmp(h, "Complex") == 0) return 1;
    }
    int64_t max_d = 0;
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        int64_t d = get_depth(e->data.function.args[i]);
        if (d > max_d) max_d = d;
    }
    return max_d + 1;
}

static LevelSpec parse_level_spec(Expr* ls, int64_t default_min, int64_t default_max) {
    LevelSpec spec = {default_min, default_max, false};
    if (!ls) return spec;

    if (ls->type == EXPR_INTEGER) {
        spec.min = 1;
        spec.max = ls->data.integer;
    } else if (ls->type == EXPR_FUNCTION && strcmp(ls->data.function.head->data.symbol, "List") == 0) {
        if (ls->data.function.arg_count == 1 && ls->data.function.args[0]->type == EXPR_INTEGER) {
            spec.min = spec.max = ls->data.function.args[0]->data.integer;
        } else if (ls->data.function.arg_count == 2 && 
                   ls->data.function.args[0]->type == EXPR_INTEGER &&
                   ls->data.function.args[1]->type == EXPR_INTEGER) {
            spec.min = ls->data.function.args[0]->data.integer;
            spec.max = ls->data.function.args[1]->data.integer;
        }
    } else if (ls->type == EXPR_SYMBOL && strcmp(ls->data.symbol, "Infinity") == 0) {
        spec.min = 1;
        spec.max = 1000000; 
    }
    return spec;
}

static void parse_options(Expr* res, size_t start_idx, LevelSpec* spec) {
    for (size_t i = start_idx; i < res->data.function.arg_count; i++) {
        Expr* opt = res->data.function.args[i];
        if (opt->type == EXPR_FUNCTION && strcmp(opt->data.function.head->data.symbol, "Rule") == 0) {
            if (opt->data.function.arg_count == 2 && 
                opt->data.function.args[0]->type == EXPR_SYMBOL &&
                strcmp(opt->data.function.args[0]->data.symbol, "Heads") == 0) {
                if (opt->data.function.args[1]->type == EXPR_SYMBOL &&
                    strcmp(opt->data.function.args[1]->data.symbol, "True") == 0) {
                    spec->heads = true;
                }
            }
        }
    }
}

/* ------------------- Apply ------------------- */

static Expr* apply_at_level(Expr* f, Expr* expr, int64_t current_level, LevelSpec spec) {
    if (expr->type != EXPR_FUNCTION) {
        return expr_copy(expr);
    }

    int64_t d = get_depth(expr);
    bool should_apply = (current_level >= spec.min && current_level <= spec.max) ||
                        (-d >= spec.min && -d <= spec.max);

    if (should_apply) {
        size_t count = expr->data.function.arg_count;
        Expr** args = malloc(sizeof(Expr*) * count);
        for (size_t i = 0; i < count; i++) {
            args[i] = apply_at_level(f, expr->data.function.args[i], current_level + 1, spec);
        }
        Expr* new_func = expr_new_function(expr_copy(f), args, count);
        free(args);
        Expr* eval_res = evaluate(new_func);
        expr_free(new_func);
        return eval_res;
    }

    size_t count = expr->data.function.arg_count;
    Expr** new_args = malloc(sizeof(Expr*) * count);
    for (size_t i = 0; i < count; i++) {
        new_args[i] = apply_at_level(f, expr->data.function.args[i], current_level + 1, spec);
    }
    
    Expr* new_head = NULL;
    if (spec.heads) {
        new_head = apply_at_level(f, expr->data.function.head, current_level + 1, spec);
    } else {
        new_head = expr_copy(expr->data.function.head);
    }
    
    Expr* result = expr_new_function(new_head, new_args, count);
    free(new_args);
    return result;
}

Expr* builtin_apply(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 2) return NULL;
    
    Expr* f = res->data.function.args[0];
    Expr* expr = res->data.function.args[1];
    
    Expr* ls = (res->data.function.arg_count >= 3) ? res->data.function.args[2] : NULL;
    if (ls && ls->type == EXPR_FUNCTION && strcmp(ls->data.function.head->data.symbol, "Rule") == 0) ls = NULL;

    LevelSpec spec = parse_level_spec(ls, 0, 0);
    parse_options(res, ls ? 3 : 2, &spec);

    return apply_at_level(f, expr, 0, spec);
}

/* ------------------- Map ------------------- */

static Expr* map_at_level(Expr* f, Expr* expr, int64_t current_level, LevelSpec spec) {
    Expr* intermediate = NULL;
    if (expr->type == EXPR_FUNCTION) {
        // Recurse first (Bottom-up)
        size_t count = expr->data.function.arg_count;
        Expr** new_args = malloc(sizeof(Expr*) * count);
        for (size_t i = 0; i < count; i++) {
            new_args[i] = map_at_level(f, expr->data.function.args[i], current_level + 1, spec);
        }
        
        Expr* new_head = NULL;
        if (spec.heads) {
            new_head = map_at_level(f, expr->data.function.head, current_level + 1, spec);
        } else {
            new_head = expr_copy(expr->data.function.head);
        }
        
        intermediate = expr_new_function(new_head, new_args, count);
        free(new_args);
    } else {
        intermediate = expr_copy(expr);
    }

    int64_t d = get_depth(intermediate);
    bool should_map = (current_level >= spec.min && current_level <= spec.max) ||
                      (-d >= spec.min && -d <= spec.max);

    if (should_map) {
        Expr* f_copy = expr_copy(f);
        Expr* call = expr_new_function(f_copy, &intermediate, 1);
        Expr* result = evaluate(call);
        expr_free(call);
        return result;
    }

    return intermediate;
}

Expr* builtin_map(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 2) return NULL;

    Expr* f = res->data.function.args[0];
    Expr* expr = res->data.function.args[1];
    
    Expr* ls = (res->data.function.arg_count >= 3) ? res->data.function.args[2] : NULL;
    if (ls && ls->type == EXPR_FUNCTION && strcmp(ls->data.function.head->data.symbol, "Rule") == 0) ls = NULL;

    LevelSpec spec = parse_level_spec(ls, 1, 1);
    parse_options(res, ls ? 3 : 2, &spec);

    return map_at_level(f, expr, 0, spec);
}

/* ------------------- MapAll ------------------- */

Expr* builtin_map_all(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 2) return NULL;

    Expr* f = res->data.function.args[0];
    Expr* expr = res->data.function.args[1];
    
    LevelSpec spec = {0, 1000000, false}; // {0, Infinity}
    parse_options(res, 2, &spec);

    return map_at_level(f, expr, 0, spec);
}

/* ------------------- Select ------------------- */

Expr* builtin_select(Expr* res) {
    if (res->type != EXPR_FUNCTION || (res->data.function.arg_count != 2 && res->data.function.arg_count != 3)) {
        return NULL;
    }
    
    Expr* list = res->data.function.args[0];
    Expr* crit = res->data.function.args[1];
    
    if (list->type != EXPR_FUNCTION) return NULL; // Can only select from compound expressions
    
    int64_t n_max = -1; // -1 means all
    if (res->data.function.arg_count == 3) {
        Expr* n_expr = res->data.function.args[2];
        if (n_expr->type == EXPR_INTEGER) {
            n_max = n_expr->data.integer;
        } else {
            return NULL; // Invalid n
        }
    }
    
    size_t count = list->data.function.arg_count;
    Expr** kept_args = NULL;
    if (count > 0) kept_args = malloc(sizeof(Expr*) * count);
    size_t kept_count = 0;
    
    for (size_t i = 0; i < count; i++) {
        if (n_max >= 0 && (int64_t)kept_count >= n_max) break;
        
        Expr* elem = list->data.function.args[i];
        
        Expr* call_args[1] = { expr_copy(elem) };
        Expr* call = expr_new_function(expr_copy(crit), call_args, 1);
        
        Expr* eval_res = evaluate(call);
        
        if (eval_res->type == EXPR_SYMBOL && strcmp(eval_res->data.symbol, "True") == 0) {
            kept_args[kept_count++] = expr_copy(elem);
        }
        
        expr_free(eval_res);
        expr_free(call);
    }
    
    Expr* result = expr_new_function(expr_copy(list->data.function.head), kept_args, kept_count);
    if (kept_args) free(kept_args);
    
    return result;
}

/* ------------------- Through ------------------- */

static Expr* transform_head(Expr* head, Expr* h_spec, Expr** args, size_t arg_count, bool* transformed) {
    if (head->type != EXPR_FUNCTION) return expr_copy(head);
    
    Expr* P = head->data.function.head;
    bool match = false;
    
    if (h_spec == NULL) {
        match = true;
    } else {
        match = expr_eq(P, h_spec);
    }
    
    if (match) {
        *transformed = true;
        size_t n = head->data.function.arg_count;
        Expr** new_args = malloc(sizeof(Expr*) * n);
        
        for (size_t i = 0; i < n; i++) {
            Expr* fi = head->data.function.args[i];
            
            Expr** call_args = malloc(sizeof(Expr*) * arg_count);
            for (size_t j = 0; j < arg_count; j++) {
                call_args[j] = expr_copy(args[j]);
            }
            Expr* call = expr_new_function(expr_copy(fi), call_args, arg_count);
            free(call_args);
            
            new_args[i] = evaluate(call);
            expr_free(call);
        }
        
        Expr* ret = expr_new_function(expr_copy(P), new_args, n);
        free(new_args);
        return ret;
    } else {
        size_t n = head->data.function.arg_count;
        Expr** new_args = malloc(sizeof(Expr*) * n);
        for (size_t i = 0; i < n; i++) {
            new_args[i] = transform_head(head->data.function.args[i], h_spec, args, arg_count, transformed);
        }
        Expr* ret = expr_new_function(expr_copy(P), new_args, n);
        free(new_args);
        return ret;
    }
}

Expr* builtin_through(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1 || res->data.function.arg_count > 2) return NULL;
    
    Expr* expr = res->data.function.args[0];
    Expr* h_spec = (res->data.function.arg_count == 2) ? res->data.function.args[1] : NULL;
    
    if (expr->type != EXPR_FUNCTION) return expr_copy(expr);
    
    Expr* head = expr->data.function.head;
    Expr** args = expr->data.function.args;
    size_t arg_count = expr->data.function.arg_count;
    
    bool transformed = false;
    Expr* new_expr = NULL;
    
    if (h_spec == NULL) {
        if (head->type == EXPR_FUNCTION) {
            Expr* P = head->data.function.head;
            size_t n = head->data.function.arg_count;
            Expr** new_H_args = malloc(sizeof(Expr*) * n);
            for (size_t i = 0; i < n; i++) {
                Expr* fi = head->data.function.args[i];
                
                Expr** call_args = malloc(sizeof(Expr*) * arg_count);
                for (size_t j = 0; j < arg_count; j++) {
                    call_args[j] = expr_copy(args[j]);
                }
                Expr* call = expr_new_function(expr_copy(fi), call_args, arg_count);
                free(call_args);
                
                new_H_args[i] = evaluate(call);
                expr_free(call);
            }
            new_expr = expr_new_function(expr_copy(P), new_H_args, n);
            free(new_H_args);
            transformed = true;
        }
    } else {
        new_expr = transform_head(head, h_spec, args, arg_count, &transformed);
    }
    
    if (transformed) {
        Expr* eval_res = evaluate(new_expr);
        expr_free(new_expr);
        return eval_res;
    }
    
    if (new_expr) expr_free(new_expr);
    return expr_copy(expr);
}

/* ------------------- FreeQ ------------------- */

static bool freeq_at_level(Expr* expr, Expr* form, int64_t current_level, LevelSpec spec) {
    int64_t d = get_depth(expr);
    bool should_check = false;
    
    if (spec.min == -1 && spec.max == -1) {
        if (expr->type != EXPR_FUNCTION || (expr->type == EXPR_FUNCTION && expr->data.function.head->type == EXPR_SYMBOL && (strcmp(expr->data.function.head->data.symbol, "Rational") == 0 || strcmp(expr->data.function.head->data.symbol, "Complex") == 0))) {
            should_check = true;
        }
    } else {
        should_check = (current_level >= spec.min && current_level <= spec.max) ||
                       (-d >= spec.min && -d <= spec.max);
    }

    if (should_check) {
        MatchEnv* env = env_new();
        if (match(expr, form, env)) {
            env_free(env);
            return false; // Not free!
        }
        env_free(env);
    }

    if (expr->type == EXPR_FUNCTION && !(expr->data.function.head->type == EXPR_SYMBOL && (strcmp(expr->data.function.head->data.symbol, "Rational") == 0 || strcmp(expr->data.function.head->data.symbol, "Complex") == 0))) {
        if (spec.heads) {
            if (!freeq_at_level(expr->data.function.head, form, current_level + 1, spec)) {
                return false;
            }
        }
        for (size_t i = 0; i < expr->data.function.arg_count; i++) {
            if (!freeq_at_level(expr->data.function.args[i], form, current_level + 1, spec)) {
                return false;
            }
        }
    }

    return true;
}

Expr* builtin_freeq(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 2) return NULL;
    Expr* expr = res->data.function.args[0];
    Expr* form = res->data.function.args[1];

    LevelSpec spec = {0, 1000000, true}; // Default: {0, Infinity}, Heads -> True
    if (res->data.function.arg_count >= 3) {
        Expr* ls = res->data.function.args[2];
        spec = parse_level_spec(ls, 1, 1);
        if (ls->type == EXPR_FUNCTION && strcmp(ls->data.function.head->data.symbol, "List") == 0) {
            // Already parsed by parse_level_spec
        } else if (ls->type == EXPR_SYMBOL && strcmp(ls->data.symbol, "All") == 0) {
            spec.min = 0; spec.max = 1000000;
        } else if (ls->type == EXPR_SYMBOL && strcmp(ls->data.symbol, "Infinity") == 0) {
            spec.min = 1; spec.max = 1000000;
        } else if (ls->type == EXPR_INTEGER) {
            spec.min = 1; spec.max = ls->data.integer;
        }
    }

    parse_options(res, res->data.function.arg_count >= 3 ? 3 : 2, &spec);
    
    // Check options anywhere for FreeQ
    for (size_t i = 2; i < res->data.function.arg_count; i++) {
        Expr* opt = res->data.function.args[i];
        if (opt->type == EXPR_FUNCTION && strcmp(opt->data.function.head->data.symbol, "Rule") == 0) {
            if (opt->data.function.arg_count == 2 && 
                opt->data.function.args[0]->type == EXPR_SYMBOL &&
                strcmp(opt->data.function.args[0]->data.symbol, "Heads") == 0) {
                if (opt->data.function.args[1]->type == EXPR_SYMBOL &&
                    strcmp(opt->data.function.args[1]->data.symbol, "False") == 0) {
                    spec.heads = false;
                }
            }
        }
    }

    if (freeq_at_level(expr, form, 0, spec)) {
        return expr_new_symbol("True");
    } else {
        return expr_new_symbol("False");
    }
}

/* ------------------- Distribute ------------------- */

static void distribute_recursive(Expr*** components, size_t* component_counts, size_t n_args, size_t current_arg, Expr** current_tuple, Expr*** results, size_t* res_count, size_t* res_cap, Expr* fp_head) {
    if (current_arg == n_args) {
        Expr** tuple_copy = malloc(sizeof(Expr*) * n_args);
        for (size_t i = 0; i < n_args; i++) tuple_copy[i] = expr_copy(current_tuple[i]);
        Expr* term = expr_new_function(expr_copy(fp_head), tuple_copy, n_args);
        free(tuple_copy);
        
        if (*res_count >= *res_cap) {
            *res_cap = (*res_cap == 0) ? 16 : (*res_cap * 2);
            *results = realloc(*results, sizeof(Expr*) * (*res_cap));
        }
        (*results)[(*res_count)++] = term;
        return;
    }
    
    for (size_t i = 0; i < component_counts[current_arg]; i++) {
        current_tuple[current_arg] = components[current_arg][i];
        distribute_recursive(components, component_counts, n_args, current_arg + 1, current_tuple, results, res_count, res_cap, fp_head);
    }
}

Expr* builtin_distribute(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1 || res->data.function.arg_count > 5) {
        return NULL;
    }
    
    Expr* expr = res->data.function.args[0];
    if (expr->type != EXPR_FUNCTION) return expr_copy(expr);

    Expr* g = (res->data.function.arg_count >= 2) ? res->data.function.args[1] : expr_new_symbol("Plus");
    Expr* f = (res->data.function.arg_count >= 3) ? res->data.function.args[2] : expr_copy(expr->data.function.head);
    Expr* gp = (res->data.function.arg_count >= 4) ? res->data.function.args[3] : expr_copy(g);
    Expr* fp = (res->data.function.arg_count >= 5) ? res->data.function.args[4] : expr_copy(f);

    // If head(expr) != f, return expr
    if (!expr_eq(expr->data.function.head, f)) {
        if (res->data.function.arg_count < 2) expr_free(g);
        if (res->data.function.arg_count < 3) expr_free(f);
        if (res->data.function.arg_count < 4) expr_free(gp);
        if (res->data.function.arg_count < 5) expr_free(fp);
        return expr_copy(expr);
    }

    size_t n_args = expr->data.function.arg_count;
    Expr*** components = malloc(sizeof(Expr**) * n_args);
    size_t* component_counts = malloc(sizeof(size_t) * n_args);
    bool any_g = false;

    for (size_t i = 0; i < n_args; i++) {
        Expr* arg = expr->data.function.args[i];
        if (arg->type == EXPR_FUNCTION && expr_eq(arg->data.function.head, g)) {
            components[i] = arg->data.function.args;
            component_counts[i] = arg->data.function.arg_count;
            any_g = true;
        } else {
            components[i] = &expr->data.function.args[i];
            component_counts[i] = 1;
        }
    }

    if (!any_g) {
        // No distribution possible, but we still apply fp if it's different?
        // Actually Distribute[f[x], g, f, gp, fp] -> gp[fp[x]] if no g is found?
        // Let's re-read: "Distribute explicitly constructs the complete result of a distribution"
        // If no g is found, it usually returns fp[args] wrapped in gp? No, wait.
        // Distribute[f[a], g] -> f[a]
        // Distribute[f[g[a]], g] -> g[f[a]]
        
        // Cleanup and return original if no g found
        free(components);
        free(component_counts);
        if (res->data.function.arg_count < 2) expr_free(g);
        if (res->data.function.arg_count < 3) expr_free(f);
        if (res->data.function.arg_count < 4) expr_free(gp);
        if (res->data.function.arg_count < 5) expr_free(fp);
        return expr_copy(expr);
    }

    Expr** results = NULL;
    size_t res_count = 0;
    size_t res_cap = 0;
    Expr** current_tuple = malloc(sizeof(Expr*) * n_args);

    distribute_recursive(components, component_counts, n_args, 0, current_tuple, &results, &res_count, &res_cap, fp);

    Expr* final_res = expr_new_function(expr_copy(gp), results, res_count);
    if (results) free(results);
    
    // results array itself is consumed by expr_new_function, but only if we don't free it.
    // wait, expr_new_function usually COPIES the array if we pass it, OR it takes ownership?
    // In picocas, expr_new_function takes Expr** args.
    // Let's check expr_new_function implementation.
    
    free(current_tuple);
    free(components);
    free(component_counts);
    if (res->data.function.arg_count < 2) expr_free(g);
    if (res->data.function.arg_count < 3) expr_free(f);
    if (res->data.function.arg_count < 4) expr_free(gp);
    if (res->data.function.arg_count < 5) expr_free(fp);
    
    // We need to evaluate the result because gp or fp might be builtins
    Expr* eval_res = evaluate(final_res);
    expr_free(final_res);
    return eval_res;
}

/* ------------------- Inner ------------------- */

static Expr* contract_V_B(Expr* f, Expr* V, Expr* B, Expr* g, Expr* head) {
    if (V->type != EXPR_FUNCTION || B->type != EXPR_FUNCTION) return NULL;
    size_t N = V->data.function.arg_count;
    
    bool b_is_matrix = false;
    if (B->data.function.arg_count > 0 && B->data.function.args[0]->type == EXPR_FUNCTION &&
        expr_eq(B->data.function.args[0]->data.function.head, head)) {
        b_is_matrix = true;
    }
    
    if (b_is_matrix) {
        size_t M = B->data.function.args[0]->data.function.arg_count;
        Expr** res_args = malloc(sizeof(Expr*) * M);
        for (size_t j = 0; j < M; j++) {
            Expr** col_args = malloc(sizeof(Expr*) * N);
            for (size_t i = 0; i < N; i++) {
                if (i < B->data.function.arg_count) {
                    Expr* B_i = B->data.function.args[i];
                    if (B_i->type == EXPR_FUNCTION && j < B_i->data.function.arg_count) {
                        col_args[i] = expr_copy(B_i->data.function.args[j]);
                    } else {
                        col_args[i] = expr_new_symbol("Null");
                    }
                } else {
                    col_args[i] = expr_new_symbol("Null");
                }
            }
            Expr* B_col = expr_new_function(expr_copy(head), col_args, N);
            Expr* contracted = contract_V_B(f, V, B_col, g, head);
            expr_free(B_col);
            if (!contracted) {
                for (size_t k = 0; k < j; k++) expr_free(res_args[k]);
                free(res_args);
                return NULL;
            }
            res_args[j] = contracted;
        }
        Expr* ret = expr_new_function(expr_copy(head), res_args, M);
        free(res_args);
        return ret;
    } else {
        size_t B_len = B->data.function.arg_count;
        size_t min_len = N < B_len ? N : B_len;
        if (N != B_len) {
            // Technically lengths should match. We'll evaluate up to the min_len.
        }
        Expr** g_args = malloc(sizeof(Expr*) * min_len);
        for (size_t i = 0; i < min_len; i++) {
            Expr* f_args[2] = { expr_copy(V->data.function.args[i]), expr_copy(B->data.function.args[i]) };
            g_args[i] = expr_new_function(expr_copy(f), f_args, 2);
        }
        Expr* ret = expr_new_function(expr_copy(g), g_args, min_len);
        free(g_args);
        return ret;
    }
}

static Expr* inner_A(Expr* f, Expr* A, Expr* B, Expr* g, Expr* head) {
    if (A->type != EXPR_FUNCTION) return NULL;
    bool a_is_matrix = false;
    if (A->data.function.arg_count > 0 && A->data.function.args[0]->type == EXPR_FUNCTION &&
        expr_eq(A->data.function.args[0]->data.function.head, head)) {
        a_is_matrix = true;
    }
    
    if (a_is_matrix) {
        size_t N = A->data.function.arg_count;
        Expr** res_args = malloc(sizeof(Expr*) * N);
        for (size_t i = 0; i < N; i++) {
            Expr* inner_res = inner_A(f, A->data.function.args[i], B, g, head);
            if (!inner_res) {
                for (size_t j = 0; j < i; j++) expr_free(res_args[j]);
                free(res_args);
                return NULL;
            }
            res_args[i] = inner_res;
        }
        Expr* ret = expr_new_function(expr_copy(head), res_args, N);
        free(res_args);
        return ret;
    } else {
        return contract_V_B(f, A, B, g, head);
    }
}

Expr* builtin_inner(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 3) return NULL;
    
    Expr* f = res->data.function.args[0];
    Expr* A = res->data.function.args[1];
    Expr* B = res->data.function.args[2];
    Expr* g = (res->data.function.arg_count >= 4) ? res->data.function.args[3] : expr_new_symbol("Plus");
    
    Expr* n_expr = (res->data.function.arg_count >= 5) ? res->data.function.args[4] : NULL;
    
    if (A->type != EXPR_FUNCTION || B->type != EXPR_FUNCTION) {
        if (res->data.function.arg_count < 4) expr_free(g);
        return NULL;
    }
    
    Expr* head = A->data.function.head;
    
    Expr* A_used = expr_copy(A);
    if (n_expr && n_expr->type == EXPR_INTEGER && n_expr->data.integer == 1) {
        Expr* t_args[1] = { A_used };
        Expr* t_expr = expr_new_function(expr_new_symbol("Transpose"), t_args, 1);
        A_used = evaluate(t_expr);
        expr_free(t_expr);
    }
    
    Expr* inner_res = inner_A(f, A_used, B, g, head);
    expr_free(A_used);
    
    if (res->data.function.arg_count < 4) expr_free(g);
    
    Expr* final_eval = evaluate(inner_res);
    expr_free(inner_res);
    return final_eval;
}

/* ------------------- Outer ------------------- */

static Expr* outer_rec(Expr* f, Expr** orig_tensors, size_t num_tensors, int64_t* target_depths, 
                size_t arg_idx, Expr* curr_subtensor, int64_t curr_depth, Expr** current_atoms, Expr* head) {
    
    bool treat_as_atom = false;
    if (curr_depth >= target_depths[arg_idx]) {
        treat_as_atom = true;
    } else if (curr_subtensor->type != EXPR_FUNCTION || (head && !expr_eq(curr_subtensor->data.function.head, head))) {
        treat_as_atom = true;
    }
    
    if (treat_as_atom) {
        current_atoms[arg_idx] = curr_subtensor;
if (arg_idx + 1 == num_tensors) {
            Expr** f_args = malloc(sizeof(Expr*) * num_tensors);
            for(size_t i=0; i<num_tensors; i++) f_args[i] = expr_copy(current_atoms[i]);
            Expr* ret = expr_new_function(expr_copy(f), f_args, num_tensors);
            free(f_args);
            return ret;
        } else {
            return outer_rec(f, orig_tensors, num_tensors, target_depths, 
                             arg_idx + 1, orig_tensors[arg_idx + 1], 0, current_atoms, head);
        }
    }
    
    size_t count = curr_subtensor->data.function.arg_count;
    Expr** new_args = malloc(sizeof(Expr*) * count);
    for (size_t i = 0; i < count; i++) {
        new_args[i] = outer_rec(f, orig_tensors, num_tensors, target_depths,
                                arg_idx, curr_subtensor->data.function.args[i], curr_depth + 1, current_atoms, head);
    }
    Expr* ret = expr_new_function(expr_copy(head), new_args, count);
    free(new_args);
    return ret;
}

Expr* builtin_outer(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1) return NULL;
    
    Expr* f = res->data.function.args[0];
    
    size_t num_depths = 0;
    for (int64_t i = res->data.function.arg_count - 1; i >= 1; i--) {
        Expr* a = res->data.function.args[i];
        if (a->type == EXPR_INTEGER || (a->type == EXPR_SYMBOL && strcmp(a->data.symbol, "Infinity") == 0)) {
            num_depths++;
        } else {
            break;
        }
    }
    
    size_t num_tensors = res->data.function.arg_count - 1 - num_depths;
    if (num_tensors == 0) {
        Expr* ret = expr_new_function(expr_copy(f), NULL, 0);
        Expr* evaluated = evaluate(ret);
        expr_free(ret);
        return evaluated;
    }
    
    Expr** tensors = &res->data.function.args[1];
    
    int64_t* target_depths = malloc(sizeof(int64_t) * num_tensors);
    for (size_t i = 0; i < num_tensors; i++) target_depths[i] = INT64_MAX;

    if (num_depths == 1) {
        Expr* d = res->data.function.args[1 + num_tensors];
        int64_t val = (d->type == EXPR_INTEGER) ? d->data.integer : INT64_MAX;
        for (size_t i = 0; i < num_tensors; i++) target_depths[i] = val;
    } else if (num_depths > 1) {
        for (size_t i = 0; i < num_depths && i < num_tensors; i++) {
            Expr* d = res->data.function.args[1 + num_tensors + i];
            target_depths[i] = (d->type == EXPR_INTEGER) ? d->data.integer : INT64_MAX;
        }
    }
    
    Expr* head = NULL;
    for (size_t i = 0; i < num_tensors; i++) {
        if (tensors[i]->type == EXPR_FUNCTION) {
            head = tensors[i]->data.function.head;
            break;
        }
    }
    
    Expr** current_atoms = malloc(sizeof(Expr*) * num_tensors);
    Expr* raw_res = outer_rec(f, tensors, num_tensors, target_depths, 0, tensors[0], 0, current_atoms, head);
    
    free(current_atoms);
    free(target_depths);
    
    Expr* final_eval = evaluate(raw_res);
    expr_free(raw_res);
    return final_eval;
}

/* ------------------- Tuples ------------------- */

static void tuples_rec(Expr** lists, size_t num_lists, size_t curr_list, Expr** current_tuple, Expr* head, Expr*** results, size_t* res_count, size_t* res_cap) {
    if (curr_list == num_lists) {
        Expr** t_args = malloc(sizeof(Expr*) * num_lists);
        for(size_t i=0; i<num_lists; i++) t_args[i] = expr_copy(current_tuple[i]);
        Expr* t = expr_new_function(expr_copy(head), t_args, num_lists);
        if (*res_count >= *res_cap) {
            *res_cap = (*res_cap == 0) ? 16 : (*res_cap * 2);
            *results = realloc(*results, sizeof(Expr*) * (*res_cap));
        }
        (*results)[(*res_count)++] = t;
        return;
    }
    
    Expr* lst = lists[curr_list];
    size_t count = lst->type == EXPR_FUNCTION ? lst->data.function.arg_count : 0;
    if (count == 0) return;
    
    for (size_t i = 0; i < count; i++) {
        current_tuple[curr_list] = lst->data.function.args[i];
        tuples_rec(lists, num_lists, curr_list + 1, current_tuple, head, results, res_count, res_cap);
    }
}

static Expr* reshape_rec(Expr** flat_args, size_t* offset, int64_t* dims, size_t dims_count, size_t current_dim, Expr* head) {
    if (current_dim == dims_count - 1) {
        size_t n = dims[current_dim];
        Expr** args = malloc(sizeof(Expr*) * n);
        for (size_t i = 0; i < n; i++) {
            args[i] = expr_copy(flat_args[(*offset)++]);
        }
        Expr* ret = expr_new_function(expr_copy(head), args, n);
        free(args);
        return ret;
    } else {
        size_t n = dims[current_dim];
        Expr** args = malloc(sizeof(Expr*) * n);
        for (size_t i = 0; i < n; i++) {
            args[i] = reshape_rec(flat_args, offset, dims, dims_count, current_dim + 1, head);
        }
        Expr* ret = expr_new_function(expr_copy(head), args, n);
        free(args);
        return ret;
    }
}

Expr* builtin_tuples(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1 || res->data.function.arg_count > 2) return NULL;
    
    if (res->data.function.arg_count == 1) {
        Expr* arg = res->data.function.args[0];
        if (arg->type != EXPR_FUNCTION) return expr_copy(res);
        Expr* head = arg->data.function.head;
        
        size_t num_lists = arg->data.function.arg_count;
        Expr** lists = arg->data.function.args;
        
        if (num_lists == 0) {
            Expr* t = expr_new_function(expr_copy(head), NULL, 0);
            Expr* final_res = expr_new_function(expr_copy(head), (Expr*[]){t}, 1);
            return final_res;
        }

        Expr** results = NULL;
        size_t res_count = 0, res_cap = 0;
        Expr** current_tuple = malloc(sizeof(Expr*) * num_lists);
        
        tuples_rec(lists, num_lists, 0, current_tuple, head, &results, &res_count, &res_cap);
        free(current_tuple);
        
        Expr* final_res = expr_new_function(expr_copy(head), results, res_count);
        return final_res;
    } else {
        Expr* list = res->data.function.args[0];
        Expr* n_expr = res->data.function.args[1];
        if (list->type != EXPR_FUNCTION) return expr_copy(res);
        Expr* head = list->data.function.head;
        
        if (n_expr->type == EXPR_INTEGER) {
            int64_t n = n_expr->data.integer;
            if (n < 0) n = 0;
            if (n == 0) {
                Expr* t = expr_new_function(expr_copy(head), NULL, 0);
                Expr* final_res = expr_new_function(expr_copy(head), (Expr*[]){t}, 1);
                return final_res;
            }

            Expr** lists = malloc(sizeof(Expr*) * n);
            for (int64_t i = 0; i < n; i++) lists[i] = list;
            
            Expr** results = NULL;
            size_t res_count = 0, res_cap = 0;
            Expr** current_tuple = malloc(sizeof(Expr*) * n);
            
            tuples_rec(lists, n, 0, current_tuple, head, &results, &res_count, &res_cap);
            free(current_tuple);
            free(lists);
            
            return expr_new_function(expr_new_symbol("List"), results, res_count);
        } else if (n_expr->type == EXPR_FUNCTION && expr_eq(n_expr->data.function.head, head)) {
            size_t dims_count = n_expr->data.function.arg_count;
            int64_t total_elements = 1;
            int64_t* dims = malloc(sizeof(int64_t) * dims_count);
            for (size_t i = 0; i < dims_count; i++) {
                if (n_expr->data.function.args[i]->type != EXPR_INTEGER) {
                    free(dims);
                    return expr_copy(res);
                }
                dims[i] = n_expr->data.function.args[i]->data.integer;
                if (dims[i] < 0) dims[i] = 0;
                total_elements *= dims[i];
            }
            
            if (total_elements == 0) {
                free(dims);
                return expr_new_function(expr_copy(head), NULL, 0);
            }

            Expr** lists = malloc(sizeof(Expr*) * total_elements);
            for (int64_t i = 0; i < total_elements; i++) lists[i] = list;
            
            Expr** results = NULL;
            size_t res_count = 0, res_cap = 0;
            Expr** current_tuple = malloc(sizeof(Expr*) * total_elements);
            
            tuples_rec(lists, total_elements, 0, current_tuple, head, &results, &res_count, &res_cap);
            free(current_tuple);
            free(lists);
            
            for (size_t i = 0; i < res_count; i++) {
                size_t offset = 0;
                Expr* reshaped = reshape_rec(results[i]->data.function.args, &offset, dims, dims_count, 0, head);
                expr_free(results[i]);
                results[i] = reshaped;
            }
            
            free(dims);
            return expr_new_function(expr_new_symbol("List"), results, res_count);
        }
    }
    return expr_copy(res);
}

/* ------------------- Permutations ------------------- */

typedef struct {
    Expr* expr;
    int count;
} UniqueElement;

static void permutations_rec(UniqueElement* elements, size_t num_unique, size_t target_len, size_t current_len, 
                             Expr** current_perm, Expr* head, Expr*** results, size_t* res_count, size_t* res_cap) {
    if (current_len == target_len) {
        Expr** p_args = NULL;
        if (target_len > 0) {
            p_args = malloc(sizeof(Expr*) * target_len);
            for (size_t i = 0; i < target_len; i++) p_args[i] = expr_copy(current_perm[i]);
        }
        Expr* p = expr_new_function(expr_copy(head), p_args, target_len);
        if (p_args) free(p_args);
        if (*res_count >= *res_cap) {
            *res_cap = (*res_cap == 0) ? 16 : (*res_cap * 2);
            *results = realloc(*results, sizeof(Expr*) * (*res_cap));
        }
        (*results)[(*res_count)++] = p;
        return;
    }
    
    for (size_t i = 0; i < num_unique; i++) {
        if (elements[i].count > 0) {
            elements[i].count--;
            current_perm[current_len] = elements[i].expr;
            permutations_rec(elements, num_unique, target_len, current_len + 1, current_perm, head, results, res_count, res_cap);
            elements[i].count++;
        }
    }
}

Expr* builtin_permutations(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 1 || res->data.function.arg_count > 2) return NULL;
    
    Expr* list = res->data.function.args[0];
    if (list->type != EXPR_FUNCTION) return expr_copy(res); 
    
    Expr* head = list->data.function.head;
    size_t len = list->data.function.arg_count;
    
    int64_t n_min = len;
    int64_t n_max = len;
    int64_t dn = 1;
    
    if (res->data.function.arg_count == 2) {
        Expr* spec = res->data.function.args[1];
        if (spec->type == EXPR_INTEGER) {
            int64_t n = spec->data.integer;
            if (n > (int64_t)len) n = len;
            if (n < 0) n = 0;
            n_min = n;
            n_max = n;
            dn = 1;
        } else if (spec->type == EXPR_SYMBOL && strcmp(spec->data.symbol, "All") == 0) {
            n_min = 0;
            n_max = len;
            dn = 1;
        } else if (spec->type == EXPR_FUNCTION && strcmp(spec->data.function.head->data.symbol, "List") == 0) {
            size_t s_len = spec->data.function.arg_count;
            if (s_len == 1 && spec->data.function.args[0]->type == EXPR_INTEGER) {
                n_min = n_max = spec->data.function.args[0]->data.integer;
            } else if (s_len == 2 && spec->data.function.args[0]->type == EXPR_INTEGER && spec->data.function.args[1]->type == EXPR_INTEGER) {
                n_min = spec->data.function.args[0]->data.integer;
                n_max = spec->data.function.args[1]->data.integer;
                dn = (n_min <= n_max) ? 1 : -1;
            } else if (s_len == 3 && spec->data.function.args[0]->type == EXPR_INTEGER && spec->data.function.args[1]->type == EXPR_INTEGER && spec->data.function.args[2]->type == EXPR_INTEGER) {
                n_min = spec->data.function.args[0]->data.integer;
                n_max = spec->data.function.args[1]->data.integer;
                dn = spec->data.function.args[2]->data.integer;
            }
        }
    }
    
    UniqueElement* elements = malloc(sizeof(UniqueElement) * len);
    size_t num_unique = 0;
    for (size_t i = 0; i < len; i++) {
        Expr* e = list->data.function.args[i];
        bool found = false;
        for (size_t j = 0; j < num_unique; j++) {
            if (expr_eq(e, elements[j].expr)) {
                elements[j].count++;
                found = true;
                break;
            }
        }
        if (!found) {
            elements[num_unique].expr = e;
            elements[num_unique].count = 1;
            num_unique++;
        }
    }
    
    Expr** results = NULL;
    size_t res_count = 0, res_cap = 0;
    Expr** current_perm = malloc(sizeof(Expr*) * (n_min > n_max ? n_min : n_max) + sizeof(Expr*) * len); 
    
    if (dn != 0) {
        if (dn > 0) {
            for (int64_t l = n_min; l <= n_max; l += dn) {
                if (l >= 0 && l <= (int64_t)len) {
                    permutations_rec(elements, num_unique, l, 0, current_perm, head, &results, &res_count, &res_cap);
                }
            }
        } else {
            for (int64_t l = n_min; l >= n_max; l += dn) {
                if (l >= 0 && l <= (int64_t)len) {
                    permutations_rec(elements, num_unique, l, 0, current_perm, head, &results, &res_count, &res_cap);
                }
            }
        }
    }
    
    free(current_perm);
    free(elements);
    
    Expr* final_res = expr_new_function(expr_new_symbol("List"), results, res_count);
    if (results) free(results);
    return final_res;
}
