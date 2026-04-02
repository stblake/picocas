/*
 * piecewise.c
 * 
 * Implementation of PicoCAS piecewise mathematical functions:
 * Floor, Ceiling, Round, IntegerPart, FractionalPart.
 * Suitable for numeric manipulation, automatically threading over lists.
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include "piecewise.h"
#include "symtab.h"
#include "eval.h"
#include "arithmetic.h"
#include "complex.h"

void piecewise_init(void) {
    symtab_add_builtin("Floor", builtin_floor);
    symtab_add_builtin("Ceiling", builtin_ceiling);
    symtab_add_builtin("Round", builtin_round);
    symtab_add_builtin("IntegerPart", builtin_integerpart);
    symtab_add_builtin("FractionalPart", builtin_fractionalpart);

    const char* funcs[] = {"Floor", "Ceiling", "Round", "IntegerPart", "FractionalPart", NULL};
    for (int i = 0; funcs[i] != NULL; i++) {
        symtab_get_def(funcs[i])->attributes |= (ATTR_PROTECTED | ATTR_NUMERICFUNCTION | ATTR_LISTABLE);
    }
}

enum { OP_FLOOR, OP_CEILING, OP_ROUND, OP_INTPART, OP_FRACPART };

/*
 * round_half_even:
 * Rounds numbers of the form x.5 toward the nearest even integer.
 */
static double round_half_even(double x) {
    double f = floor(x);
    double r = x - f;
    if (r < 0.5) return f;
    if (r > 0.5) return f + 1.0;
    // Exactly 0.5
    if (fmod(fabs(f), 2.0) == 0.0) return f;
    return f + 1.0;
}

static bool is_infinity(Expr* e) {
    return e->type == EXPR_SYMBOL && (strcmp(e->data.symbol, "Infinity") == 0 || strcmp(e->data.symbol, "ComplexInfinity") == 0);
}

static bool is_minus_infinity(Expr* e) {
    if (e->type == EXPR_FUNCTION && e->data.function.arg_count == 2 && 
        e->data.function.head->type == EXPR_SYMBOL && strcmp(e->data.function.head->data.symbol, "Times") == 0) {
        Expr* a1 = e->data.function.args[0];
        Expr* a2 = e->data.function.args[1];
        if (a1->type == EXPR_INTEGER && a1->data.integer == -1 && is_infinity(a2)) return true;
        if (a2->type == EXPR_INTEGER && a2->data.integer == -1 && is_infinity(a1)) return true;
    }
    return false;
}

/*
 * do_piecewise_1:
 * Applies the piecewise operation to a single numeric argument.
 */
static Expr* do_piecewise_1(Expr* x, int op) {
    if (is_infinity(x) || is_minus_infinity(x)) {
        if (op == OP_FRACPART) return expr_new_integer(0);
        return expr_copy(x);
    }

    if (x->type == EXPR_INTEGER) {
        if (op == OP_FRACPART) return expr_new_integer(0);
        return expr_copy(x);
    }
    
    if (x->type == EXPR_REAL) {
        double v = x->data.real;
        double res = 0.0;
        if (op == OP_FLOOR) res = floor(v);
        else if (op == OP_CEILING) res = ceil(v);
        else if (op == OP_ROUND) res = round_half_even(v);
        else if (op == OP_INTPART) res = trunc(v);
        else if (op == OP_FRACPART) return expr_new_real(v - trunc(v));
        
        return expr_new_integer((int64_t)res);
    }
    
    int64_t n, d;
    if (is_rational(x, &n, &d)) {
        double v = (double)n / d;
        double res = 0.0;
        if (op == OP_FLOOR) res = floor(v);
        else if (op == OP_CEILING) res = ceil(v);
        else if (op == OP_ROUND) res = round_half_even(v);
        else if (op == OP_INTPART) res = trunc(v);
        else if (op == OP_FRACPART) {
            int64_t int_part = (int64_t)trunc(v);
            return make_rational(n - int_part * d, d);
        }
        return expr_new_integer((int64_t)res);
    }
    
    Expr *re, *im;
    if (is_complex(x, &re, &im)) {
        Expr* re_res = do_piecewise_1(re, op);
        Expr* im_res = do_piecewise_1(im, op);
        if (re_res && im_res) {
            Expr* combined = make_complex(re_res, im_res);
            expr_free(re_res);
            expr_free(im_res);
            return combined;
        }
        if (re_res) expr_free(re_res);
        if (im_res) expr_free(im_res);
    }
    
    return NULL;
}

static Expr* make_divide(Expr* num, Expr* den) {
    Expr* pow_args[2] = { expr_copy(den), expr_new_integer(-1) };
    Expr* inv = expr_new_function(expr_new_symbol("Power"), pow_args, 2);
    Expr* times_args[2] = { expr_copy(num), inv };
    return expr_new_function(expr_new_symbol("Times"), times_args, 2);
}

static Expr* do_piecewise(Expr* res, int op, const char* name, bool allow_2_args) {
    if (res->type != EXPR_FUNCTION) return NULL;
    
    if (res->data.function.arg_count == 1) {
        return do_piecewise_1(res->data.function.args[0], op);
    } else if (allow_2_args && res->data.function.arg_count == 2) {
        Expr* x = res->data.function.args[0];
        Expr* a = res->data.function.args[1];
        
        Expr* div = make_divide(x, a);
        Expr* func_args[1] = { div }; // div ownership passes to function
        Expr* func = expr_new_function(expr_new_symbol(name), func_args, 1);
        
        Expr* times_args[2] = { expr_copy(a), func };
        return expr_new_function(expr_new_symbol("Times"), times_args, 2);
    }
    
    return NULL;
}

Expr* builtin_floor(Expr* res) { return do_piecewise(res, OP_FLOOR, "Floor", true); }
Expr* builtin_ceiling(Expr* res) { return do_piecewise(res, OP_CEILING, "Ceiling", true); }
Expr* builtin_round(Expr* res) { return do_piecewise(res, OP_ROUND, "Round", true); }
Expr* builtin_integerpart(Expr* res) { return do_piecewise(res, OP_INTPART, "IntegerPart", false); }
Expr* builtin_fractionalpart(Expr* res) { return do_piecewise(res, OP_FRACPART, "FractionalPart", false); }
