
#include "plus.h"
#include "arithmetic.h"
#include "complex.h"
#include "eval.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static bool is_overflow(Expr* e) {
    return e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL &&
           strcmp(e->data.function.head->data.symbol, "Overflow") == 0;
}

/* Helper: extract numeric coefficient and base expression from a term. 
   Always returns an OWNED coefficient. Base is NOT owned by default, unless *allocated_base is set to true. */
static void get_coeff_base(Expr* e, Expr** coeff, Expr** base, bool* allocated_base) {
    *allocated_base = false;
    if (is_overflow(e)) {
        *coeff = expr_copy(e);
        *base = NULL;
        return;
    }

    if (e->type == EXPR_INTEGER || e->type == EXPR_REAL || is_rational(e, NULL, NULL) || is_complex(e, NULL, NULL)) {
        *coeff = expr_copy(e);
        *base = NULL;
        return;
    }
    
    if (e->type == EXPR_FUNCTION && strcmp(e->data.function.head->data.symbol, "Times") == 0) {
        if (e->data.function.arg_count >= 2) {
            Expr* first = e->data.function.args[0];
            if (first->type == EXPR_INTEGER || first->type == EXPR_REAL || is_rational(first, NULL, NULL) || is_complex(first, NULL, NULL)) {
                *coeff = expr_copy(first);
                if (e->data.function.arg_count == 2) {
                    *base = e->data.function.args[1];
                } else {
                    Expr** rest_args = malloc(sizeof(Expr*) * (e->data.function.arg_count - 1));
                    for (size_t i = 1; i < e->data.function.arg_count; i++) {
                        rest_args[i-1] = expr_copy(e->data.function.args[i]);
                    }
                    *base = expr_new_function(expr_new_symbol("Times"), rest_args, e->data.function.arg_count - 1);
                    *allocated_base = true;
                    free(rest_args);
                }
                return;
            }
        }
    }
    
    *coeff = expr_new_integer(1);
    *base = e;
}

static Expr* add_numbers(Expr* a, Expr* b) {
    if (is_overflow(a) || is_overflow(b)) return expr_new_function(expr_new_symbol("Overflow"), NULL, 0);

    Expr *re1 = NULL, *im1 = NULL, *re2 = NULL, *im2 = NULL;
    bool a_comp = is_complex(a, &re1, &im1);
    bool b_comp = is_complex(b, &re2, &im2);
    
    if (a_comp || b_comp) {
        if (!a_comp) re1 = a;
        if (!b_comp) re2 = b;
        Expr* p1 = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(re1), expr_copy(re2)}, 2));
        
        Expr* zero = expr_new_integer(0);
        Expr* i1 = a_comp ? expr_copy(im1) : expr_copy(zero);
        Expr* i2 = b_comp ? expr_copy(im2) : expr_copy(zero);
        Expr* p2 = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){i1, i2}, 2));
        expr_free(zero);
        
        if (is_overflow(p1) || is_overflow(p2)) {
            expr_free(p1); expr_free(p2);
            return expr_new_function(expr_new_symbol("Overflow"), NULL, 0);
        }

        Expr* res = make_complex(p1, p2);
        expr_free(p1);
        expr_free(p2);
        return res;
    }

    if (a->type == EXPR_REAL || b->type == EXPR_REAL) {
        double va = 0.0, vb = 0.0;
        int64_t n, d;
        if (a->type == EXPR_REAL) va = a->data.real;
        else if (a->type == EXPR_INTEGER) va = (double)a->data.integer;
        else if (is_rational(a, &n, &d)) va = (double)n / d;

        if (b->type == EXPR_REAL) vb = b->data.real;
        else if (b->type == EXPR_INTEGER) vb = (double)b->data.integer;
        else if (is_rational(b, &n, &d)) vb = (double)n / d;
        
        return expr_new_real(va + vb);
    }

    if (a->type == EXPR_INTEGER && b->type == EXPR_INTEGER) {
        __int128_t res = (__int128_t)a->data.integer + b->data.integer;
        if (res > INT64_MAX || res < INT64_MIN) return expr_new_function(expr_new_symbol("Overflow"), NULL, 0);
        return expr_new_integer((int64_t)res);
    }

    int64_t n1, d1, n2, d2;
    if (is_rational(a, &n1, &d1) && is_rational(b, &n2, &d2)) {
        __int128_t num = (__int128_t)n1 * d2 + (__int128_t)n2 * d1;
        __int128_t den = (__int128_t)d1 * d2;
        if (num > INT64_MAX || num < INT64_MIN || den > INT64_MAX || den < INT64_MIN)
            return expr_new_function(expr_new_symbol("Overflow"), NULL, 0);
        return make_rational((int64_t)num, (int64_t)den);
    }
    
    return NULL;
}

Expr* make_plus(Expr* a, Expr* b) {
    Expr* args[2] = { a, b };
    return expr_new_function(expr_new_symbol("Plus"), args, 2);
}

Expr* builtin_plus(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    
    size_t n = res->data.function.arg_count;
    if (n == 0) return expr_new_integer(0);
    if (n == 1) return expr_copy(res->data.function.args[0]);

    Expr* num_sum = expr_new_integer(0);
    
    typedef struct {
        Expr* base;
        Expr* coeff;
        bool temp_base;
    } TermGroup;
    
    TermGroup* groups = malloc(sizeof(TermGroup) * n);
    size_t group_count = 0;
    
    for (size_t i = 0; i < n; i++) {
        Expr* arg = res->data.function.args[i];
        Expr* c = NULL;
        Expr* b = NULL;
        bool is_temp_base = false;
        get_coeff_base(arg, &c, &b, &is_temp_base);
        
        if (is_overflow(c)) {
            expr_free(num_sum);
            for (size_t j = 0; j < group_count; j++) {
                expr_free(groups[j].coeff);
                if (groups[j].temp_base) expr_free(groups[j].base);
            }
            free(groups);
            expr_free(c);
            if (is_temp_base && b != arg) expr_free(b);
            return expr_new_function(expr_new_symbol("Overflow"), NULL, 0);
        }

        if (b == NULL) {
            Expr* next_sum = add_numbers(num_sum, c);
            expr_free(num_sum);
            num_sum = next_sum;
            expr_free(c);
            if (is_temp_base && b != arg) expr_free(b);
            if (is_overflow(num_sum)) {
                for (size_t j = 0; j < group_count; j++) {
                    expr_free(groups[j].coeff);
                    if (groups[j].temp_base) expr_free(groups[j].base);
                }
                free(groups);
                return num_sum;
            }
        } else {
            int found = -1;
            for (size_t j = 0; j < group_count; j++) {
                if (expr_eq(groups[j].base, b)) {
                    found = (int)j;
                    break;
                }
            }
            
            if (found >= 0) {
                Expr* next_c = add_numbers(groups[found].coeff, c);
                expr_free(groups[found].coeff);
                groups[found].coeff = next_c;
                expr_free(c);
                if (is_temp_base) expr_free(b);
                if (is_overflow(groups[found].coeff)) {
                    expr_free(num_sum);
                    for (size_t j = 0; j < group_count; j++) {
                        expr_free(groups[j].coeff);
                        if (groups[j].temp_base) expr_free(groups[j].base);
                    }
                    free(groups);
                    return expr_new_function(expr_new_symbol("Overflow"), NULL, 0);
                }
            } else {
                groups[group_count].base = b; 
                groups[group_count].coeff = c; // Already owned
                groups[group_count].temp_base = is_temp_base;
                group_count++;
            }
        }
    }
    
    size_t final_count = group_count;
    bool has_num = !(num_sum->type == EXPR_INTEGER && num_sum->data.integer == 0);
    if (has_num) final_count++;
    
    if (final_count == 0) {
        expr_free(num_sum);
        free(groups);
        return expr_new_integer(0);
    }
    
    Expr** final_args = malloc(sizeof(Expr*) * final_count);
    size_t idx = 0;
    if (has_num) {
        Expr* re = NULL, *im = NULL;
        if (is_complex(num_sum, &re, &im) && im->type == EXPR_INTEGER && im->data.integer == 0) {
            final_args[idx++] = expr_copy(re);
        } else {
            final_args[idx++] = expr_copy(num_sum);
        }
    }
    
    for (size_t j = 0; j < group_count; j++) {
        if (groups[j].coeff->type == EXPR_INTEGER && groups[j].coeff->data.integer == 0) {
            continue;
        }
        if (groups[j].coeff->type == EXPR_INTEGER && groups[j].coeff->data.integer == 1) {
            final_args[idx++] = expr_copy(groups[j].base);
        } else {
            Expr* t_args[] = {expr_copy(groups[j].coeff), expr_copy(groups[j].base)};
            final_args[idx++] = expr_new_function(expr_new_symbol("Times"), t_args, 2);
        }
    }
    
    Expr* final_res = NULL;
    if (idx == 0) {
        final_res = expr_new_integer(0);
        free(final_args);
    } else if (idx == 1) {
        final_res = final_args[0];
        free(final_args);
    } else {
        final_res = expr_new_function(expr_new_symbol("Plus"), final_args, idx);
        free(final_args);
    }
    
    expr_free(num_sum);
    for(size_t j=0; j<group_count; j++) {
        expr_free(groups[j].coeff);
        if (groups[j].temp_base) expr_free(groups[j].base);
    }
    free(groups);
    
    return final_res;
}
