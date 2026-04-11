#define _USE_MATH_DEFINES
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <complex.h>
#include "trig.h"
#include "arithmetic.h"
#include "plus.h"
#include "times.h"
#include "power.h"
#include "complex.h"
#include "symtab.h"
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif
#include <string.h>
#include <stdint.h>

/* Helper function to construct Sqrt[n] as Power[n, 1/2] */
static Expr* make_sqrt(int64_t n) {
    Expr* base = expr_new_integer(n);
    Expr* exponent = make_rational(1, 2);
    Expr* res = make_power(base, exponent);
    return res;
}

/* Helper function to construct Sqrt[expr] as Power[expr, 1/2] */
static Expr* make_sqrt_expr(Expr* e) {
    Expr* exponent = make_rational(1, 2);
    Expr* res = make_power(e, exponent);
    return res;
}

/* 
 * extract_pi_multiplier:
 * Checks if the expression is of the form n/d * Pi or simply Pi (1/1 * Pi).
 * Returns true if match found, and sets n and d.
 */
static bool extract_pi_multiplier(Expr* e, int64_t* n, int64_t* d) {
    // Case 1: Pi
    if (e->type == EXPR_SYMBOL && strcmp(e->data.symbol, "Pi") == 0) {
        *n = 1; *d = 1;
        return true;
    }
    
    // Case 2: n/d * Pi (Times[Rational[n, d], Pi])
    if (e->type == EXPR_FUNCTION && strcmp(e->data.function.head->data.symbol, "Times") == 0 && e->data.function.arg_count == 2) {
        Expr* first = e->data.function.args[0];
        Expr* second = e->data.function.args[1];
        
        if (second->type == EXPR_SYMBOL && strcmp(second->data.symbol, "Pi") == 0) {
            if (is_rational(first, n, d)) return true;
        }
    }
    return false;
}

/* 
 * get_approx:
 * Tries to get a numeric complex approximation of the expression.
 */
static bool get_approx(Expr* e, double complex* out) {
    if (e->type == EXPR_INTEGER) {
        *out = (double)e->data.integer + 0.0 * I;
        return true;
    }
    if (e->type == EXPR_REAL) {
        *out = e->data.real + 0.0 * I;
        return true;
    }
    int64_t n, d;
    if (is_rational(e, &n, &d)) {
        *out = (double)n / d + 0.0 * I;
        return true;
    }
    Expr *re, *im;
    if (is_complex(e, &re, &im)) {
        double complex r, i;
        if (get_approx(re, &r) && get_approx(im, &i)) {
            *out = creal(r) + creal(i) * I;
            return true;
        }
    }
    return false;
}

// --- Exact Symbolic Trigonometric Values ---

/*
 * exact_sin:
 * Computes exact symbolic evaluations for Sin[n/d * Pi].
 * We reduce the argument using 2Pi periodicity and symmetry properties.
 * Currently supports denominators: 1, 2, 3, 4, 5, 6, 10, 12.
 */
static Expr* exact_sin(int64_t n, int64_t d) {
    // Reduce to [0, 2Pi)
    n = n % (2 * d);
    if (n < 0) n += 2 * d;
    
    int64_t sign = 1;
    // Map to [0, Pi]
    if (n >= d) {
        sign = -1;
        n -= d;
    }
    // Map to [0, Pi/2]
    if (n > d / 2.0) {
        n = d - n;
    }
    
    // Reduce fraction
    int64_t g = gcd(n, d);
    n /= g; d /= g;
    
    Expr* res = NULL;
    if (d == 1) res = expr_new_integer(0);
    else if (d == 2) res = expr_new_integer(1);
    else if (d == 3) res = make_times(make_rational(1, 2), make_sqrt(3));
    else if (d == 4) res = make_power(expr_new_integer(2), make_rational(-1, 2));
    else if (d == 5) {
        if (n == 1) res = make_times(make_rational(1, 4), make_sqrt_expr(make_plus(expr_new_integer(10), make_times(expr_new_integer(-2), make_sqrt(5)))));
        else if (n == 2) res = make_times(make_rational(1, 4), make_sqrt_expr(make_plus(expr_new_integer(10), make_times(expr_new_integer(2), make_sqrt(5)))));
    }
    else if (d == 6) res = make_rational(1, 2);
    else if (d == 10) {
        if (n == 1) res = make_times(make_rational(1, 4), make_plus(make_sqrt(5), expr_new_integer(-1)));
        else if (n == 3) res = make_times(make_rational(1, 4), make_plus(make_sqrt(5), expr_new_integer(1)));
    }
    else if (d == 12) {
        if (n == 1) res = make_times(make_rational(1, 4), make_plus(make_sqrt(6), make_times(expr_new_integer(-1), make_sqrt(2))));
        else if (n == 5) res = make_times(make_rational(1, 4), make_plus(make_sqrt(6), make_sqrt(2)));
    }
    
    // Apply resolved sign
    if (res && sign == -1) {
        return make_times(expr_new_integer(-1), res);
    }
    return res;
}

/*
 * exact_cos:
 * Computes exact symbolic evaluations for Cos[n/d * Pi].
 * Uses cosine's even symmetry and 2Pi periodicity.
 */
static Expr* exact_cos(int64_t n, int64_t d) {
    // Reduce to [0, 2Pi)
    n = n % (2 * d);
    if (n < 0) n += 2 * d;
    // Exploit symmetry Cos[x] = Cos[2Pi - x] to map to [0, Pi]
    if (n > d) n = 2 * d - n;
    
    int64_t sign = 1;
    // Map to [0, Pi/2]
    if (n > d / 2.0) {
        n = d - n;
        sign = -1;
    }
    
    // Reduce fraction
    int64_t g = gcd(n, d);
    n /= g; d /= g;
    
    Expr* res = NULL;
    if (d == 1) res = expr_new_integer(1);
    else if (d == 2) res = expr_new_integer(0);
    else if (d == 3) res = make_rational(1, 2);
    else if (d == 4) res = make_power(expr_new_integer(2), make_rational(-1, 2));
    else if (d == 5) {
        if (n == 1) res = make_times(make_rational(1, 4), make_plus(make_sqrt(5), expr_new_integer(1)));
        else if (n == 2) res = make_times(make_rational(1, 4), make_plus(make_sqrt(5), expr_new_integer(-1)));
    }
    else if (d == 6) res = make_times(make_rational(1, 2), make_sqrt(3));
    else if (d == 10) {
        if (n == 1) res = make_times(make_rational(1, 4), make_sqrt_expr(make_plus(expr_new_integer(10), make_times(expr_new_integer(2), make_sqrt(5)))));
        else if (n == 3) res = make_times(make_rational(1, 4), make_sqrt_expr(make_plus(expr_new_integer(10), make_times(expr_new_integer(-2), make_sqrt(5)))));
    }
    else if (d == 12) {
        if (n == 1) res = make_times(make_rational(1, 4), make_plus(make_sqrt(6), make_sqrt(2)));
        else if (n == 5) res = make_times(make_rational(1, 4), make_plus(make_sqrt(6), make_times(expr_new_integer(-1), make_sqrt(2))));
    }
    
    // Apply resolved sign
    if (res && sign == -1) {
        return make_times(expr_new_integer(-1), res);
    }
    return res;
}

/*
 * exact_tan:
 * Computes exact symbolic evaluations for Tan[n/d * Pi].
 * Tangent is Pi-periodic. We reduce to [0, Pi/2] and resolve exact symbolic forms.
 */
static Expr* exact_tan(int64_t n, int64_t d) {
    n = n % d;
    if (n < 0) n += d;
    
    int64_t sign = 1;
    // Map to [0, Pi/2]
    if (n > d / 2.0) {
        n = d - n;
        sign = -1;
    }
    
    // Reduce fraction
    int64_t g = gcd(n, d);
    n /= g; d /= g;
    
    Expr* res = NULL;
    if (d == 1) res = expr_new_integer(0);
    else if (d == 2) res = expr_new_symbol("ComplexInfinity");
    else if (d == 3) res = make_sqrt(3);
    else if (d == 4) res = expr_new_integer(1);
    else if (d == 5) {
        if (n == 1) res = make_sqrt_expr(make_plus(expr_new_integer(5), make_times(expr_new_integer(-2), make_sqrt(5))));
        else if (n == 2) res = make_sqrt_expr(make_plus(expr_new_integer(5), make_times(expr_new_integer(2), make_sqrt(5))));
    }
    else if (d == 6) res = make_power(expr_new_integer(3), make_rational(-1, 2));
    else if (d == 10) {
        if (n == 1) res = make_sqrt_expr(make_plus(expr_new_integer(1), make_times(make_rational(-2, 5), make_sqrt(5))));
        else if (n == 3) res = make_sqrt_expr(make_plus(expr_new_integer(1), make_times(make_rational(2, 5), make_sqrt(5))));
    }
    else if (d == 12) {
        if (n == 1) res = make_plus(expr_new_integer(2), make_times(expr_new_integer(-1), make_sqrt(3)));
        else if (n == 5) res = make_plus(expr_new_integer(2), make_sqrt(3));
    }
    
    // Apply resolved sign
    if (res && sign == -1) {
        return make_times(expr_new_integer(-1), res);
    }
    return res;
}

/*
 * exact_cot:
 * Computes exact symbolic evaluations for Cot[n/d * Pi].
 * Cotangent is Pi-periodic. We reduce to [0, Pi/2] and resolve exact symbolic forms.
 */
static Expr* exact_cot(int64_t n, int64_t d) {
    n = n % d;
    if (n < 0) n += d;
    
    int64_t sign = 1;
    // Map to [0, Pi/2]
    if (n > d / 2.0) {
        n = d - n;
        sign = -1;
    }
    
    // Reduce fraction
    int64_t g = gcd(n, d);
    n /= g; d /= g;
    
    Expr* res = NULL;
    if (d == 1) res = expr_new_symbol("ComplexInfinity");
    else if (d == 2) res = expr_new_integer(0);
    else if (d == 3) res = make_power(expr_new_integer(3), make_rational(-1, 2));
    else if (d == 4) res = expr_new_integer(1);
    else if (d == 5) {
        if (n == 1) res = make_sqrt_expr(make_plus(expr_new_integer(1), make_times(make_rational(2, 1), make_power(expr_new_integer(5), make_rational(-1, 2)))));
        else if (n == 2) res = make_sqrt_expr(make_plus(expr_new_integer(1), make_times(make_rational(-2, 1), make_power(expr_new_integer(5), make_rational(-1, 2)))));
    }
    else if (d == 6) res = make_sqrt(3);
    else if (d == 10) {
        if (n == 1) res = make_sqrt_expr(make_plus(expr_new_integer(5), make_times(expr_new_integer(2), make_sqrt(5))));
        else if (n == 3) res = make_sqrt_expr(make_plus(expr_new_integer(5), make_times(expr_new_integer(-2), make_sqrt(5))));
    }
    else if (d == 12) {
        if (n == 1) res = make_plus(expr_new_integer(2), make_sqrt(3));
        else if (n == 5) res = make_plus(expr_new_integer(2), make_times(expr_new_integer(-1), make_sqrt(3)));
    }
    
    // Apply resolved sign
    if (res && sign == -1) {
        return make_times(expr_new_integer(-1), res);
    }
    return res;
}

/*
 * exact_sec:
 * Computes exact symbolic evaluations for Sec[n/d * Pi].
 * Uses cosine's 2Pi periodicity and even symmetry.
 */
static Expr* exact_sec(int64_t n, int64_t d) {
    n = n % (2 * d);
    if (n < 0) n += 2 * d;
    if (n > d) n = 2 * d - n;
    
    int64_t sign = 1;
    // Map to [0, Pi/2]
    if (n > d / 2.0) {
        n = d - n;
        sign = -1;
    }
    
    // Reduce fraction
    int64_t g = gcd(n, d);
    n /= g; d /= g;
    
    Expr* res = NULL;
    if (d == 1) res = expr_new_integer(1);
    else if (d == 2) res = expr_new_symbol("ComplexInfinity");
    else if (d == 3) res = expr_new_integer(2);
    else if (d == 4) res = make_sqrt(2);
    else if (d == 5) {
        if (n == 1) res = make_plus(make_sqrt(5), expr_new_integer(-1));
        else if (n == 2) res = make_plus(make_sqrt(5), expr_new_integer(1));
    }
    else if (d == 6) res = make_times(expr_new_integer(2), make_power(expr_new_integer(3), make_rational(-1, 2)));
    else if (d == 10) {
        if (n == 1) res = make_power(make_sqrt_expr(make_plus(make_rational(5, 8), make_times(make_rational(1, 8), make_sqrt(5)))), expr_new_integer(-1));
        else if (n == 3) res = make_power(make_sqrt_expr(make_plus(make_rational(5, 8), make_times(make_rational(-1, 8), make_sqrt(5)))), expr_new_integer(-1));
    }
    else if (d == 12) {
        if (n == 1) res = make_times(make_sqrt(2), make_plus(make_sqrt(3), expr_new_integer(-1)));
        else if (n == 5) res = make_times(make_sqrt(2), make_plus(make_sqrt(3), expr_new_integer(1)));
    }
    
    // Apply resolved sign
    if (res && sign == -1) {
        return make_times(expr_new_integer(-1), res);
    }
    return res;
}

/*
 * exact_csc:
 * Computes exact symbolic evaluations for Csc[n/d * Pi].
 * Uses sine's 2Pi periodicity and odd symmetry.
 */
static Expr* exact_csc(int64_t n, int64_t d) {
    n = n % (2 * d);
    if (n < 0) n += 2 * d;
    
    int64_t sign = 1;
    if (n >= d) {
        sign = -1;
        n -= d;
    }
    if (n > d / 2.0) {
        n = d - n;
    }
    
    int64_t g = gcd(n, d);
    n /= g; d /= g;
    
    Expr* res = NULL;
    if (d == 1) res = expr_new_symbol("ComplexInfinity");
    else if (d == 2) res = expr_new_integer(1);
    else if (d == 3) res = make_times(expr_new_integer(2), make_power(expr_new_integer(3), make_rational(-1, 2)));
    else if (d == 4) res = make_sqrt(2);
    else if (d == 5) {
        if (n == 1) res = make_power(make_sqrt_expr(make_plus(make_rational(5, 8), make_times(make_rational(1, 8), make_sqrt(5)))), expr_new_integer(-1));
        else if (n == 2) res = make_power(make_sqrt_expr(make_plus(make_rational(5, 8), make_times(make_rational(1, 8), make_sqrt(5)))), expr_new_integer(-1));
    }
    else if (d == 6) res = expr_new_integer(2);
    else if (d == 10) {
        if (n == 1) res = make_plus(make_sqrt(5), expr_new_integer(1));
        else if (n == 3) res = make_plus(make_sqrt(5), expr_new_integer(-1));
    }
    else if (d == 12) {
        if (n == 1) res = make_times(make_sqrt(2), make_plus(make_sqrt(3), expr_new_integer(1)));
        else if (n == 5) res = make_times(make_sqrt(2), make_plus(make_sqrt(3), expr_new_integer(-1)));
    }
    
    if (res && sign == -1) {
        return make_times(expr_new_integer(-1), res);
    }
    return res;
}


// --- Built-in Functions ---

/*
 * builtin_sin:
 * Implements the standard evaluation logic for Sin.
 * Handles exact evaluations for integer zeros and specific Pi rational multiples.
 * Falls back to approximate numeric evaluation via C standard math (csin) if possible.
 */
Expr* builtin_sin(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    // Sin[0] = 0
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_integer(0);
    
    // Attempt exact evaluation if argument is a rational multiple of Pi
    int64_t n, d;
    if (extract_pi_multiplier(arg, &n, &d)) {
        Expr* exact = exact_sin(n, d);
        if (exact) return exact;
    }
    
    // Approximate numerical evaluation
    double complex c;
    if (get_approx(arg, &c)) {
        double complex s = csin(c);
        if (cimag(c) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

/*
 * builtin_cos:
 * Implements the standard evaluation logic for Cos.
 * Handles exact evaluations for integer zeros and specific Pi rational multiples.
 * Falls back to approximate numeric evaluation via C standard math (ccos) if possible.
 */
Expr* builtin_cos(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    // Cos[0] = 1
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_integer(1);
    
    // Attempt exact evaluation if argument is a rational multiple of Pi
    int64_t n, d;
    if (extract_pi_multiplier(arg, &n, &d)) {
        Expr* exact = exact_cos(n, d);
        if (exact) return exact;
    }
    
    // Approximate numerical evaluation
    double complex c;
    if (get_approx(arg, &c)) {
        double complex s = ccos(c);
        if (cimag(c) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

/*
 * builtin_tan:
 * Implements the standard evaluation logic for Tan.
 * Handles exact evaluations for integer zeros and specific Pi rational multiples.
 * Falls back to approximate numeric evaluation via C standard math (ctan) if possible.
 */
Expr* builtin_tan(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    // Tan[0] = 0
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_integer(0);
    
    // Attempt exact evaluation if argument is a rational multiple of Pi
    int64_t n, d;
    if (extract_pi_multiplier(arg, &n, &d)) {
        Expr* exact = exact_tan(n, d);
        if (exact) return exact;
    }
    
    // Approximate numerical evaluation
    double complex cplx;
    if (get_approx(arg, &cplx)) {
        double complex s = catan(cplx);
        if (cimag(cplx) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

/*
 * builtin_cot:
 * Implements the standard evaluation logic for Cot.
 * Handles exact evaluations for integer zeros and specific Pi rational multiples.
 * Falls back to approximate numeric evaluation via C standard math (1/ctan) if possible.
 */
Expr* builtin_cot(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    // Cot[0] = ComplexInfinity
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_symbol("ComplexInfinity");
    
    // Attempt exact evaluation if argument is a rational multiple of Pi
    int64_t n, d;
    if (extract_pi_multiplier(arg, &n, &d)) {
        Expr* exact = exact_cot(n, d);
        if (exact) return exact;
    }
    
    // Approximate numerical evaluation
    double complex cplx;
    if (get_approx(arg, &cplx)) {
        double complex s = 1.0 / ctan(cplx);
        if (cimag(cplx) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

/*
 * builtin_sec:
 * Implements the standard evaluation logic for Sec.
 * Handles exact evaluations for integer zeros and specific Pi rational multiples.
 * Falls back to approximate numeric evaluation via C standard math (1/ccos) if possible.
 */
Expr* builtin_sec(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    // Sec[0] = 1
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_integer(1);
    
    // Attempt exact evaluation if argument is a rational multiple of Pi
    int64_t n, d;
    if (extract_pi_multiplier(arg, &n, &d)) {
        Expr* exact = exact_sec(n, d);
        if (exact) return exact;
    }
    
    // Approximate numerical evaluation
    double complex cplx;
    if (get_approx(arg, &cplx)) {
        double complex s = 1.0 / ccos(cplx);
        if (cimag(cplx) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

/*
 * builtin_csc:
 * Implements the standard evaluation logic for Csc.
 * Handles exact evaluations for integer zeros and specific Pi rational multiples.
 * Falls back to approximate numeric evaluation via C standard math (1/csin) if possible.
 */
Expr* builtin_csc(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    // Csc[0] = ComplexInfinity
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_symbol("ComplexInfinity");
    
    // Attempt exact evaluation if argument is a rational multiple of Pi
    int64_t n, d;
    if (extract_pi_multiplier(arg, &n, &d)) {
        Expr* exact = exact_csc(n, d);
        if (exact) return exact;
    }
    
    // Approximate numerical evaluation
    double complex cplx;
    if (get_approx(arg, &cplx)) {
        double complex s = 1.0 / csin(cplx);
        if (cimag(cplx) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

static Expr* exact_arcsin(Expr* arg) {
    int64_t ds[] = {1, 2, 3, 4, 5, 6, 10, 12};
    for (int i = 0; i < 8; i++) {
        int64_t d = ds[i];
        for (int64_t n = -d; n <= d; n++) {
            Expr* val = exact_sin(n, d);
            if (val) {
                if (expr_eq(arg, val)) {
                    expr_free(val);
                    return make_times(make_rational(n, d), expr_new_symbol("Pi"));
                }
                expr_free(val);
            }
        }
    }
    return NULL;
}

static Expr* exact_arccos(Expr* arg) {
    int64_t ds[] = {1, 2, 3, 4, 5, 6, 10, 12};
    for (int i = 0; i < 8; i++) {
        int64_t d = ds[i];
        for (int64_t n = 0; n <= d; n++) {
            Expr* val = exact_cos(n, d);
            if (val) {
                if (expr_eq(arg, val)) {
                    expr_free(val);
                    return make_times(make_rational(n, d), expr_new_symbol("Pi"));
                }
                expr_free(val);
            }
        }
    }
    return NULL;
}

static Expr* exact_arctan(Expr* arg) {
    int64_t ds[] = {1, 2, 3, 4, 5, 6, 10, 12};
    for (int i = 0; i < 8; i++) {
        int64_t d = ds[i];
        for (int64_t n = -d/2; n <= d/2; n++) {
            Expr* val = exact_tan(n, d);
            if (val) {
                if (expr_eq(arg, val)) {
                    expr_free(val);
                    return make_times(make_rational(n, d), expr_new_symbol("Pi"));
                }
                expr_free(val);
            }
        }
    }
    return NULL;
}

static Expr* exact_arccot(Expr* arg) {
    int64_t ds[] = {1, 2, 3, 4, 5, 6, 10, 12};
    for (int i = 0; i < 8; i++) {
        int64_t d = ds[i];
        for (int64_t n = 0; n < d; n++) {
            Expr* val = exact_cot(n, d);
            if (val) {
                if (expr_eq(arg, val)) {
                    expr_free(val);
                    return make_times(make_rational(n, d), expr_new_symbol("Pi"));
                }
                expr_free(val);
            }
        }
    }
    return NULL;
}

static Expr* exact_arcsec(Expr* arg) {
    int64_t ds[] = {1, 2, 3, 4, 5, 6, 10, 12};
    for (int i = 0; i < 8; i++) {
        int64_t d = ds[i];
        for (int64_t n = 0; n <= d; n++) {
            Expr* val = exact_sec(n, d);
            if (val) {
                if (expr_eq(arg, val)) {
                    expr_free(val);
                    return make_times(make_rational(n, d), expr_new_symbol("Pi"));
                }
                expr_free(val);
            }
        }
    }
    return NULL;
}

static Expr* exact_arccsc(Expr* arg) {
    int64_t ds[] = {1, 2, 3, 4, 5, 6, 10, 12};
    for (int i = 0; i < 8; i++) {
        int64_t d = ds[i];
        for (int64_t n = -d; n <= d; n++) {
            Expr* val = exact_csc(n, d);
            if (val) {
                if (expr_eq(arg, val)) {
                    expr_free(val);
                    return make_times(make_rational(n, d), expr_new_symbol("Pi"));
                }
                expr_free(val);
            }
        }
    }
    return NULL;
}


/*
 * builtin_arcsin:
 * Implements the evaluation logic for the inverse sine (ArcSin).
 * Reverses exact algebraic mappings for supported inputs.
 * Falls back to C standard math (casin) for approximate numerical inputs.
 */
Expr* builtin_arcsin(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    // Attempt exact inverse evaluation
    Expr* exact = exact_arcsin(arg);
    if (exact) return exact;
    
    // Approximate numerical evaluation - only if input is already inexact
    double complex c;
    if ((arg->type == EXPR_REAL || is_complex(arg, NULL, NULL)) && get_approx(arg, &c)) {
        double complex s = casin(c);
        if (cimag(c) == 0.0 && creal(c) >= -1.0 && creal(c) <= 1.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

/*
 * builtin_arccos:
 * Implements the evaluation logic for the inverse cosine (ArcCos).
 * Reverses exact algebraic mappings for supported inputs.
 * Falls back to C standard math (cacos) for approximate numerical inputs.
 */
Expr* builtin_arccos(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    // Attempt exact inverse evaluation
    Expr* exact = exact_arccos(arg);
    if (exact) return exact;
    
    // Approximate numerical evaluation - only if input is already inexact
    double complex c;
    if ((arg->type == EXPR_REAL || is_complex(arg, NULL, NULL)) && get_approx(arg, &c)) {
        double complex s = cacos(c);
        if (cimag(c) == 0.0 && creal(c) >= -1.0 && creal(c) <= 1.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

/*
 * builtin_arctan:
 * Implements the evaluation logic for the inverse tangent (ArcTan).
 * Supports both ArcTan[z] (1 argument) and ArcTan[x, y] (2 arguments).
 * Reverses exact algebraic mappings for supported inputs.
 * Uses C standard math (catan and atan2) for numeric evaluations.
 */
Expr* builtin_arctan(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    
    // Single argument ArcTan[z]
    if (res->data.function.arg_count == 1) {
        Expr* arg = res->data.function.args[0];
        
        // Attempt exact inverse evaluation
        Expr* exact = exact_arctan(arg);
        if (exact) return exact;
        
        // Approximate numerical evaluation - only if input is already inexact
        double complex c;
        if ((arg->type == EXPR_REAL || is_complex(arg, NULL, NULL)) && get_approx(arg, &c)) {
            double complex s = catan(c);
            if (cimag(c) == 0.0) return expr_new_real(creal(s));
            return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
        }
    } 
    // Two argument ArcTan[x, y] (computes argument of x + I*y)
    else if (res->data.function.arg_count == 2) {
        Expr* x = res->data.function.args[0];
        Expr* y = res->data.function.args[1];
        
        // Exact discrete evaluations based on sign quadrant mapping
        if (x->type == EXPR_INTEGER && y->type == EXPR_INTEGER) {
            int64_t xv = x->data.integer;
            int64_t yv = y->data.integer;
            if (xv == 0 && yv == 0) return NULL; // Indeterminate form
            if (yv == 0) {
                if (xv > 0) return expr_new_integer(0);
                if (xv < 0) return expr_new_symbol("Pi");
            }
            if (xv == 0) {
                if (yv > 0) return make_times(make_rational(1, 2), expr_new_symbol("Pi"));
                if (yv < 0) return make_times(make_rational(-1, 2), expr_new_symbol("Pi"));
            }
            if (xv == yv) {
                if (xv > 0) return make_times(make_rational(1, 4), expr_new_symbol("Pi"));
                if (xv < 0) return make_times(make_rational(-3, 4), expr_new_symbol("Pi"));
            }
            if (xv == -yv) {
                if (xv > 0) return make_times(make_rational(-1, 4), expr_new_symbol("Pi"));
                if (xv < 0) return make_times(make_rational(3, 4), expr_new_symbol("Pi"));
            }
        }
        
        // Approximate numerical evaluation using atan2 for strictly real inputs
        double complex cx, cy;
        if ((x->type == EXPR_REAL || y->type == EXPR_REAL) && get_approx(x, &cx) && get_approx(y, &cy)) {
            if (cimag(cx) == 0.0 && cimag(cy) == 0.0) {
                return expr_new_real(atan2(creal(cy), creal(cx)));
            }
        }
    }
    return NULL;
}

/*
 * builtin_arccot:
 * Implements the evaluation logic for the inverse cotangent (ArcCot).
 * Reverses exact algebraic mappings for supported inputs.
 * Falls back to C standard math (catan) for approximate numerical inputs.
 */
Expr* builtin_arccot(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    // Attempt exact inverse evaluation
    Expr* exact = exact_arccot(arg);
    if (exact) return exact;
    
    // Approximate numerical evaluation
    double complex c;
    if ((arg->type == EXPR_REAL || is_complex(arg, NULL, NULL)) && get_approx(arg, &c)) {
        if (c == 0.0) return expr_new_real(M_PI / 2.0); // ArcCot[0] = Pi/2
        double complex s = catan(1.0 / c);
        if (cimag(c) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

/*
 * builtin_arcsec:
 * Implements the evaluation logic for the inverse secant (ArcSec).
 * Reverses exact algebraic mappings for supported inputs.
 * Falls back to C standard math (cacos) for approximate numerical inputs.
 */
Expr* builtin_arcsec(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    // Attempt exact inverse evaluation
    Expr* exact = exact_arcsec(arg);
    if (exact) return exact;
    
    // Approximate numerical evaluation
    double complex c;
    if ((arg->type == EXPR_REAL || is_complex(arg, NULL, NULL)) && get_approx(arg, &c)) {
        if (c == 0.0) return expr_new_symbol("ComplexInfinity"); // ArcSec[0] = ComplexInfinity
        double complex s = cacos(1.0 / c);
        if (cimag(c) == 0.0 && (creal(c) <= -1.0 || creal(c) >= 1.0)) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

/*
 * builtin_arccsc:
 * Implements the evaluation logic for the inverse cosecant (ArcCsc).
 * Reverses exact algebraic mappings for supported inputs.
 * Falls back to C standard math (casin) for approximate numerical inputs.
 */
Expr* builtin_arccsc(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    // Attempt exact inverse evaluation
    Expr* exact = exact_arccsc(arg);
    if (exact) return exact;
    
    // Approximate numerical evaluation
    double complex c;
    if ((arg->type == EXPR_REAL || is_complex(arg, NULL, NULL)) && get_approx(arg, &c)) {
        if (c == 0.0) return expr_new_symbol("ComplexInfinity"); // ArcCsc[0] = ComplexInfinity
        double complex s = casin(1.0 / c);
        if (cimag(c) == 0.0 && (creal(c) <= -1.0 || creal(c) >= 1.0)) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

void trig_init(void) {
    symtab_add_builtin("Sin", builtin_sin);
    symtab_add_builtin("Cos", builtin_cos);
    symtab_add_builtin("Tan", builtin_tan);
    symtab_add_builtin("Cot", builtin_cot);
    symtab_add_builtin("Sec", builtin_sec);
    symtab_add_builtin("Csc", builtin_csc);
    
    symtab_add_builtin("ArcSin", builtin_arcsin);
    symtab_add_builtin("ArcCos", builtin_arccos);
    symtab_add_builtin("ArcTan", builtin_arctan);
    symtab_add_builtin("ArcCot", builtin_arccot);
    symtab_add_builtin("ArcSec", builtin_arcsec);
    symtab_add_builtin("ArcCsc", builtin_arccsc);
}
