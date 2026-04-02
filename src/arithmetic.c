
#include "arithmetic.h"
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

int64_t gcd(int64_t a, int64_t b) {
    a = llabs(a);
    b = llabs(b);
    while (b) {
        a %= b;
        int64_t tmp = a;
        a = b;
        b = tmp;
    }
    return a;
}

int64_t lcm(int64_t a, int64_t b) {
    if (a == 0 || b == 0) return 0;
    a = llabs(a);
    b = llabs(b);
    return (a / gcd(a, b)) * b;
}

Expr* builtin_gcd(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    size_t count = res->data.function.arg_count;
    if (count == 0) return expr_new_integer(0);

    int64_t running_n = 0;
    int64_t running_d = 1;

    for (size_t i = 0; i < count; i++) {
        int64_t n, d;
        if (!is_rational(res->data.function.args[i], &n, &d)) {
            return NULL;
        }
        if (i == 0) {
            running_n = llabs(n);
            running_d = llabs(d);
        } else {
            running_n = gcd(running_n, n);
            running_d = lcm(running_d, d);
        }
    }

    return make_rational(running_n, running_d);
}

Expr* builtin_lcm(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    size_t count = res->data.function.arg_count;
    if (count == 0) return expr_new_integer(1);

    int64_t running_n = 0;
    int64_t running_d = 0;

    for (size_t i = 0; i < count; i++) {
        int64_t n, d;
        if (!is_rational(res->data.function.args[i], &n, &d)) {
            return NULL;
        }
        if (i == 0) {
            running_n = llabs(n);
            running_d = llabs(d);
        } else {
            if (running_n == 0 || n == 0) {
                running_n = 0;
                running_d = 1;
            } else {
                running_n = lcm(running_n, n);
                running_d = gcd(running_d, d);
            }
        }
    }

    return make_rational(running_n, running_d);
}

Expr* make_rational(int64_t n, int64_t d) {
    if (d == 0) return NULL; // Error
    if (n == 0) return expr_new_integer(0);
    
    int64_t common = gcd(n, d);
    n /= common;
    d /= common;

    if (d < 0) {
        n = -n;
        d = -d;
    }

    if (d == 1) return expr_new_integer(n);

    Expr* args[2];
    args[0] = expr_new_integer(n);
    args[1] = expr_new_integer(d);
    return expr_new_function(expr_new_symbol("Rational"), args, 2);
}

bool is_rational(Expr* e, int64_t* n, int64_t* d) {
    if (e->type == EXPR_INTEGER) {
        if (n) *n = e->data.integer;
        if (d) *d = 1;
        return true;
    }
    if (e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL &&
        strcmp(e->data.function.head->data.symbol, "Rational") == 0) {
        if (e->data.function.arg_count == 2 && 
            e->data.function.args[0]->type == EXPR_INTEGER &&
            e->data.function.args[1]->type == EXPR_INTEGER) {
            if (n) *n = e->data.function.args[0]->data.integer;
            if (d) *d = e->data.function.args[1]->data.integer;
            return true;
        }
    }
    return false;
}

bool is_complex(Expr* e, Expr** re, Expr** im) {
    if (e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL &&
        strcmp(e->data.function.head->data.symbol, "Complex") == 0 &&
        e->data.function.arg_count == 2) {
        if (re) *re = e->data.function.args[0];
        if (im) *im = e->data.function.args[1];
        return true;
    }
    return false;
}

Expr* make_complex(Expr* re, Expr* im) {
    Expr* args[2] = { expr_copy(re), expr_copy(im) };
    return expr_new_function(expr_new_symbol("Complex"), args, 2);
}

Expr* builtin_subtract(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;

    Expr* a = res->data.function.args[0];
    Expr* b = res->data.function.args[1];

    Expr* minus_one = expr_new_integer(-1);
    Expr* mb_args[2] = { minus_one, expr_copy(b) };
    Expr* minus_b = expr_new_function(expr_new_symbol("Times"), mb_args, 2);

    Expr* p_args[2] = { expr_copy(a), minus_b };
    return expr_new_function(expr_new_symbol("Plus"), p_args, 2);
}

Expr* builtin_complex(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;

    Expr* re = res->data.function.args[0];
    Expr* im = res->data.function.args[1];

    if (im->type == EXPR_INTEGER && im->data.integer == 0) {
        return expr_copy(re);
    }
    if (im->type == EXPR_REAL && im->data.real == 0.0) {
        if (re->type == EXPR_INTEGER) {
            return expr_new_real((double)re->data.integer);
        }
        return expr_copy(re);
    }

    return NULL;
}

Expr* builtin_divide(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;
    
    Expr* num = res->data.function.args[0];
    Expr* den = res->data.function.args[1];

    if (num->type == EXPR_REAL || den->type == EXPR_REAL) {
        double vnum = (num->type == EXPR_REAL) ? num->data.real : (double)num->data.integer;
        double vden = (den->type == EXPR_REAL) ? den->data.real : (double)den->data.integer;
        if (vden == 0.0) return NULL; 
        return expr_new_real(vnum / vden);
    }

    int64_t n1, d1, n2, d2;
    if (is_rational(num, &n1, &d1) && is_rational(den, &n2, &d2)) {
        return make_rational(n1 * d2, d1 * n2);
    }

    Expr* minus_one = expr_new_integer(-1);
    Expr* p_args[2] = { expr_copy(den), minus_one };
    Expr* power = expr_new_function(expr_new_symbol("Power"), p_args, 2);
    
    Expr* t_args[2] = { expr_copy(num), power };
    return expr_new_function(expr_new_symbol("Times"), t_args, 2);
}
