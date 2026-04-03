#include "rat.h"
#include "eval.h"
#include "symtab.h"
#include "attr.h"
#include "arithmetic.h"
#include <string.h>
#include <stdlib.h>

static Expr* negate_expr(Expr* e) {
    return eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), expr_copy(e)}, 2));
}

static bool is_superficially_negative(Expr* e) {
    if (e->type == EXPR_INTEGER) return e->data.integer < 0;
    if (e->type == EXPR_REAL) return e->data.real < 0.0;
    int64_t n, d;
    if (is_rational(e, &n, &d)) return n < 0;
    if (e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL && strcmp(e->data.function.head->data.symbol, "Times") == 0) {
        if (e->data.function.arg_count > 0) {
            Expr* first = e->data.function.args[0];
            if (first->type == EXPR_INTEGER) return first->data.integer < 0;
            if (first->type == EXPR_REAL) return first->data.real < 0.0;
            if (is_rational(first, &n, &d)) return n < 0;
        }
    }
    return false;
}

static void extract_num_den(Expr* expr, Expr** num_out, Expr** den_out) {
    int64_t n, d;
    if (is_rational(expr, &n, &d)) {
        *num_out = expr_new_integer(n);
        *den_out = expr_new_integer(d);
        return;
    }

    Expr* re; Expr* im;
    if (is_complex(expr, &re, &im)) {
        int64_t rn = 0, rd = 1, in = 0, id = 1;
        is_rational(re, &rn, &rd);
        if (re->type == EXPR_INTEGER) { rn = re->data.integer; rd = 1; }
        is_rational(im, &in, &id);
        if (im->type == EXPR_INTEGER) { in = im->data.integer; id = 1; }

        int64_t common_den = lcm(rd, id);
        
        Expr* d_expr = expr_new_integer(common_den);
        *den_out = d_expr;
        *num_out = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_copy(expr), expr_copy(d_expr)}, 2));
        return;
    }

    if (expr->type == EXPR_FUNCTION && expr->data.function.head->type == EXPR_SYMBOL && (strcmp(expr->data.function.head->data.symbol, "Power") == 0 || strcmp(expr->data.function.head->data.symbol, "Exp") == 0)) {
        bool is_exp = strcmp(expr->data.function.head->data.symbol, "Exp") == 0;
        Expr* base = is_exp ? expr_new_symbol("E") : expr->data.function.args[0];
        Expr* exp = is_exp ? expr->data.function.args[0] : expr->data.function.args[1];

        if (exp->type == EXPR_FUNCTION && exp->data.function.head->type == EXPR_SYMBOL && strcmp(exp->data.function.head->data.symbol, "Plus") == 0) {
            size_t count = exp->data.function.arg_count;
            Expr** num_args = malloc(sizeof(Expr*) * count);
            Expr** den_args = malloc(sizeof(Expr*) * count);
            size_t n_c = 0, d_c = 0;
            for (size_t i = 0; i < count; i++) {
                Expr* arg = exp->data.function.args[i];
                if (is_superficially_negative(arg)) {
                    den_args[d_c++] = negate_expr(arg);
                } else {
                    num_args[n_c++] = expr_copy(arg);
                }
            }
            if (n_c == 0) *num_out = expr_new_integer(1);
            else if (n_c == 1) *num_out = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(base), num_args[0]}, 2));
            else {
                Expr* p = eval_and_free(expr_new_function(expr_new_symbol("Plus"), num_args, n_c));
                *num_out = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(base), p}, 2));
            }
            if (d_c == 0) *den_out = expr_new_integer(1);
            else if (d_c == 1) *den_out = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(base), den_args[0]}, 2));
            else {
                Expr* p = eval_and_free(expr_new_function(expr_new_symbol("Plus"), den_args, d_c));
                *den_out = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(base), p}, 2));
            }
            free(num_args);
            free(den_args);
            if (is_exp) expr_free(base);
            return;
        } else {
            if (is_superficially_negative(exp)) {
                *num_out = expr_new_integer(1);
                *den_out = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(base), negate_expr(exp)}, 2));
            } else {
                if (is_exp) {
                    *num_out = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(base), expr_copy(exp)}, 2));
                } else {
                    *num_out = expr_copy(expr);
                }
                *den_out = expr_new_integer(1);
            }
            if (is_exp) expr_free(base);
            return;
        }
    }

    if (expr->type == EXPR_FUNCTION && expr->data.function.head->type == EXPR_SYMBOL && strcmp(expr->data.function.head->data.symbol, "Times") == 0) {
        size_t count = expr->data.function.arg_count;
        Expr** n_args = malloc(sizeof(Expr*) * count);
        Expr** d_args = malloc(sizeof(Expr*) * count);
        size_t n_c = 0, d_c = 0;
        for (size_t i = 0; i < count; i++) {
            Expr* n_out; Expr* d_out;
            extract_num_den(expr->data.function.args[i], &n_out, &d_out);
            if (!(n_out->type == EXPR_INTEGER && n_out->data.integer == 1)) n_args[n_c++] = n_out;
            else expr_free(n_out);
            if (!(d_out->type == EXPR_INTEGER && d_out->data.integer == 1)) d_args[d_c++] = d_out;
            else expr_free(d_out);
        }
        if (n_c == 0) *num_out = expr_new_integer(1);
        else if (n_c == 1) *num_out = n_args[0];
        else *num_out = eval_and_free(expr_new_function(expr_new_symbol("Times"), n_args, n_c));

        if (d_c == 0) *den_out = expr_new_integer(1);
        else if (d_c == 1) *den_out = d_args[0];
        else *den_out = eval_and_free(expr_new_function(expr_new_symbol("Times"), d_args, d_c));

        free(n_args); free(d_args);
        return;
    }

    *num_out = expr_copy(expr);
    *den_out = expr_new_integer(1);
}

Expr* builtin_numerator(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* n; Expr* d;
    extract_num_den(res->data.function.args[0], &n, &d);
    expr_free(d);
    return n;
}

Expr* builtin_denominator(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* n; Expr* d;
    extract_num_den(res->data.function.args[0], &n, &d);
    expr_free(n);
    return d;
}

void rat_init(void) {
    symtab_add_builtin("Numerator", builtin_numerator);
    symtab_get_def("Numerator")->attributes |= ATTR_LISTABLE | ATTR_PROTECTED;
    symtab_add_builtin("Denominator", builtin_denominator);
    symtab_get_def("Denominator")->attributes |= ATTR_LISTABLE | ATTR_PROTECTED;
}
