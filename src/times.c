#include "times.h"
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

static Expr* multiply_numbers(Expr* a, Expr* b) {
    if (is_overflow(a) || is_overflow(b)) return expr_new_function(expr_new_symbol("Overflow"), NULL, 0);
    if (a->type == EXPR_REAL || b->type == EXPR_REAL) {
        double va = (a->type == EXPR_REAL) ? a->data.real : (a->type == EXPR_INTEGER) ? (double)a->data.integer : 0.0;
        double vb = (b->type == EXPR_REAL) ? b->data.real : (b->type == EXPR_INTEGER) ? (double)b->data.integer : 0.0;
        int64_t n, d;
        if (is_rational(a, &n, &d)) va = (double)n / d;
        if (is_rational(b, &n, &d)) vb = (double)n / d;
        return expr_new_real(va * vb);
    }
    if (a->type == EXPR_INTEGER && b->type == EXPR_INTEGER) {
        __int128_t res = (__int128_t)a->data.integer * b->data.integer;
        if (res > INT64_MAX || res < INT64_MIN) return expr_new_function(expr_new_symbol("Overflow"), NULL, 0);
        return expr_new_integer((int64_t)res);
    }
    int64_t n1, d1, n2, d2;
    if (is_rational(a, &n1, &d1) && is_rational(b, &n2, &d2)) {
        __int128_t num = (__int128_t)n1 * n2;
        __int128_t den = (__int128_t)d1 * d2;
        if (num > INT64_MAX || num < INT64_MIN || den > INT64_MAX || den < INT64_MIN) return expr_new_function(expr_new_symbol("Overflow"), NULL, 0);
        return make_rational((int64_t)num, (int64_t)den);
    }
    return NULL;
}

Expr* make_times(Expr* a, Expr* b) {
    Expr* args[2] = { a, b };
    return expr_new_function(expr_new_symbol("Times"), args, 2);
}

typedef struct {
    Expr* base;
    Expr* exponent;
} BasePower;

Expr* builtin_times(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    size_t n = res->data.function.arg_count;
    if (n == 0) return expr_new_integer(1);
    if (n == 1) return expr_copy(res->data.function.args[0]);

    Expr* num_prod = expr_new_integer(1);
    Expr* complex_val = NULL;
    
    BasePower* groups = malloc(sizeof(BasePower) * n);
    size_t group_count = 0;

    for (size_t i = 0; i < n; i++) {
        Expr* arg = res->data.function.args[i];
        if (is_overflow(arg)) {
            expr_free(num_prod); if (complex_val) expr_free(complex_val);
            for(size_t j=0; j<group_count; j++) { expr_free(groups[j].base); expr_free(groups[j].exponent); }
            free(groups); return expr_new_function(expr_new_symbol("Overflow"), NULL, 0);
        }

        if (arg->type == EXPR_INTEGER || arg->type == EXPR_REAL || is_rational(arg, NULL, NULL)) {
            Expr* next = multiply_numbers(num_prod, arg); expr_free(num_prod); num_prod = next;
        } else if (is_complex(arg, NULL, NULL) || (arg->type == EXPR_SYMBOL && strcmp(arg->data.symbol, "I") == 0)) {
            Expr* c_arg = (arg->type == EXPR_SYMBOL) ? make_complex(expr_new_integer(0), expr_new_integer(1)) : expr_copy(arg);
            if (!complex_val) complex_val = c_arg;
            else {
                Expr *re1, *im1, *re2, *im2;
                is_complex(complex_val, &re1, &im1); is_complex(c_arg, &re2, &im2);
                Expr* re = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){
                    expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(re1), expr_copy(re2)}, 2),
                    expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(im1), expr_copy(im2)}, 2)}, 2)
                }, 2));
                Expr* im = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){
                    expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(re1), expr_copy(im2)}, 2),
                    expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(re2), expr_copy(im1)}, 2)
                }, 2));
                expr_free(complex_val); expr_free(c_arg);
                complex_val = make_complex(re, im);
                expr_free(re); expr_free(im);
            }
        } else {
            Expr* base = arg; Expr* exponent;
            if (arg->type == EXPR_FUNCTION && arg->data.function.head->type == EXPR_SYMBOL && strcmp(arg->data.function.head->data.symbol, "Power") == 0 && arg->data.function.arg_count == 2) {
                base = arg->data.function.args[0]; exponent = expr_copy(arg->data.function.args[1]);
            } else { exponent = expr_new_integer(1); }
            
            int found = -1;
            for (size_t j = 0; j < group_count; j++) { if (expr_eq(groups[j].base, base)) { found = (int)j; break; } }
            if (found != -1) {
                Expr* new_exp = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){groups[found].exponent, exponent}, 2));
                groups[found].exponent = new_exp;
            } else {
                groups[group_count].base = expr_copy(base);
                groups[group_count].exponent = (arg == base) ? exponent : exponent;
                group_count++;
            }
        }
    }

    if (num_prod->type == EXPR_INTEGER && num_prod->data.integer == 0) {
        if (complex_val) expr_free(complex_val);
        for(size_t j=0; j<group_count; j++) { expr_free(groups[j].base); expr_free(groups[j].exponent); }
        free(groups); return num_prod;
    }

    if (complex_val && !(num_prod->type == EXPR_INTEGER && num_prod->data.integer == 1)) {
        Expr *re, *im; is_complex(complex_val, &re, &im);
        Expr* nr = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(num_prod), expr_copy(re)}, 2));
        Expr* ni = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(num_prod), expr_copy(im)}, 2));
        expr_free(complex_val); complex_val = make_complex(nr, ni);
        expr_free(nr); expr_free(ni); expr_free(num_prod); num_prod = expr_new_integer(1);
    }

    size_t final_count = 0;
    if (!(num_prod->type == EXPR_INTEGER && num_prod->data.integer == 1)) final_count++;
    if (complex_val) final_count++;
    for (size_t i = 0; i < group_count; i++) {
        if (!(groups[i].exponent->type == EXPR_INTEGER && groups[i].exponent->data.integer == 0)) final_count++;
    }

    if (final_count == 0) {
        expr_free(num_prod); if (complex_val) expr_free(complex_val);
        for(size_t j=0; j<group_count; j++) { expr_free(groups[j].base); expr_free(groups[j].exponent); }
        free(groups); return expr_new_integer(1);
    }

    Expr** final_args = malloc(sizeof(Expr*) * final_count); size_t idx = 0;
    if (!(num_prod->type == EXPR_INTEGER && num_prod->data.integer == 1)) final_args[idx++] = num_prod;
    else expr_free(num_prod);
    if (complex_val) final_args[idx++] = complex_val;
    for (size_t i = 0; i < group_count; i++) {
        if (groups[i].exponent->type == EXPR_INTEGER && groups[i].exponent->data.integer == 0) {
            expr_free(groups[i].base); expr_free(groups[i].exponent); continue;
        }
        if (groups[i].exponent->type == EXPR_INTEGER && groups[i].exponent->data.integer == 1) {
            final_args[idx++] = groups[i].base; expr_free(groups[i].exponent);
        } else {
            final_args[idx++] = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){groups[i].base, groups[i].exponent}, 2));
        }
    }
    free(groups);
    if (idx == 1) { Expr* res_final = final_args[0]; free(final_args); return res_final; }
    Expr* result = expr_new_function(expr_new_symbol("Times"), final_args, idx);
    free(final_args); return result;
}
