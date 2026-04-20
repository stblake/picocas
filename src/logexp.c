#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <complex.h>
#include "logexp.h"
#include "symtab.h"
#include "eval.h"
#include "arithmetic.h"
#include "complex.h"

/*
 * logexp_init:
 * Initializes the logarithmic and exponential built-in functions in PicoCAS.
 * Adds the 'Log' and 'Exp' C functions to the symbol table and protects the constant 'E'.
 */
void logexp_init(void) {
    symtab_add_builtin("Log", builtin_log);
    symtab_add_builtin("Exp", builtin_exp);
    symtab_get_def("E")->attributes |= ATTR_PROTECTED;
}

/*
 * get_approx:
 * Attempts to extract a numeric approximation (double complex) from a given expression.
 * Handles EXPR_REAL, EXPR_INTEGER, Rational[n, d], and Complex[re, im] formats.
 * Returns true if a valid numeric approximation could be extracted, storing it in *out.
 */
static bool get_approx(Expr* e, double complex* out, bool* is_inexact) {
    if (is_inexact) *is_inexact = true;
    if (e->type == EXPR_REAL) {
        *out = e->data.real + 0.0 * I;
        return true;
    }
    
    Expr* re; Expr* im;
    // Check if the expression is a complex number representation
    if (is_complex(e, &re, &im)) {
        bool has_real = false;
        double r = 0.0, i = 0.0;
        int64_t n, d;
        
        // Extract real part
        if (re->type == EXPR_REAL) { r = re->data.real; has_real = true; }
        else if (re->type == EXPR_INTEGER) r = (double)re->data.integer;
        else if (is_rational(re, &n, &d)) r = (double)n / d;
        else return false;
        
        // Extract imaginary part
        if (im->type == EXPR_REAL) { i = im->data.real; has_real = true; }
        else if (im->type == EXPR_INTEGER) i = (double)im->data.integer;
        else if (is_rational(im, &n, &d)) i = (double)n / d;
        else return false;
        
        if (has_real) {
            *out = r + i * I;
            return true;
        }
    }
    return false;
}

/*
 * is_infinity:
 * Checks if the given expression represents the symbol 'Infinity'.
 */
static bool is_infinity(Expr* e) {
    return e->type == EXPR_SYMBOL && strcmp(e->data.symbol, "Infinity") == 0;
}

/*
 * is_minus_infinity:
 * Checks if the given expression represents '-Infinity',
 * which is represented internally as Times[-1, Infinity].
 */
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
 * make_minus_infinity:
 * Constructs and returns a new expression representing '-Infinity'.
 */
static Expr* make_minus_infinity() {
    Expr* args[2] = { expr_new_integer(-1), expr_new_symbol("Infinity") };
    return expr_new_function(expr_new_symbol("Times"), args, 2);
}

/*
 * builtin_log:
 * Implements the evaluation logic for the 'Log' function.
 * Supports Log[z] (natural logarithm) and Log[b, z] (base-b logarithm).
 */
Expr* builtin_log(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;

    // Log[z] - Natural logarithm
    if (res->data.function.arg_count == 1) {
        Expr* z = res->data.function.args[0];

        // Exact evaluations for special constants
        if (z->type == EXPR_INTEGER && z->data.integer == 0) {
            Expr* ret = make_minus_infinity(); // Log[0] = -Infinity
            return ret;
        }
        if (z->type == EXPR_INTEGER && z->data.integer == 1) {
            Expr* ret = expr_new_integer(0); // Log[1] = 0
            return ret;
        }

        // Negative integer: Log[n] = I*Pi + Log[-n] for n < 0
        if ((z->type == EXPR_INTEGER && z->data.integer < 0) ||
            (z->type == EXPR_BIGINT && mpz_sgn(z->data.bigint) < 0)) {
            Expr* neg_z;
            if (z->type == EXPR_INTEGER) {
                if (z->data.integer == INT64_MIN) {
                    mpz_t tmp;
                    mpz_init_set_si(tmp, z->data.integer);
                    mpz_neg(tmp, tmp);
                    neg_z = expr_new_bigint_from_mpz(tmp);
                    mpz_clear(tmp);
                } else {
                    neg_z = expr_new_integer(-z->data.integer);
                }
            } else {
                mpz_t tmp;
                mpz_init(tmp);
                mpz_neg(tmp, z->data.bigint);
                neg_z = expr_new_bigint_from_mpz(tmp);
                mpz_clear(tmp);
                neg_z = expr_bigint_normalize(neg_z);
            }
            Expr* log_args[1] = { neg_z };
            Expr* log_neg = expr_new_function(expr_new_symbol("Log"), log_args, 1);
            Expr* times_args[2] = { expr_new_symbol("I"), expr_new_symbol("Pi") };
            Expr* i_pi = expr_new_function(expr_new_symbol("Times"), times_args, 2);
            Expr* plus_args[2] = { i_pi, log_neg };
            return expr_new_function(expr_new_symbol("Plus"), plus_args, 2);
        }

        if (is_infinity(z)) {
            Expr* ret = expr_new_symbol("Infinity"); // Log[Infinity] = Infinity
            return ret;
        }
        if (z->type == EXPR_SYMBOL && strcmp(z->data.symbol, "E") == 0) {
            Expr* ret = expr_new_integer(1); // Log[E] = 1
            return ret;
        }

        // Approximate numerical evaluation
        double complex c;
        bool inexact = false;
    if (get_approx(z, &c, &inexact) && inexact) {
            double complex s = clog(c);
            // Return real result if output is purely real, otherwise return complex
            Expr* ret = NULL;
            if (cimag(c) == 0.0 && creal(c) > 0.0) ret = expr_new_real(creal(s));
            else ret = make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
            return ret;
        }
    } 
    // Log[b, z] - Logarithm to base b
    else if (res->data.function.arg_count == 2) {
        Expr* b = res->data.function.args[0];
        Expr* z = res->data.function.args[1];

        // Log[b, b] = 1
        if (expr_eq(b, z)) {
            Expr* ret = expr_new_integer(1);
            return ret;
        }

        // Attempt to return exact rational results for integer bases and arguments (e.g. Log[2, 8] = 3)
        if (b->type == EXPR_INTEGER && z->type == EXPR_INTEGER) {
            int64_t bv = b->data.integer;
            int64_t zv = z->data.integer;
            if (bv > 1 && zv > 0) {
                int64_t temp = zv;
                int64_t p = 0;
                while (temp > 1 && temp % bv == 0) {
                    temp /= bv;
                    p++;
                }
                if (temp == 1) {
                    Expr* ret = expr_new_integer(p);
                    return ret;
                }
            }
        }

        // Default rewrite: Log[b, z] -> Log[z] / Log[b]
        Expr* num_args[1] = { expr_copy(z) };
        Expr* den_args[1] = { expr_copy(b) };
        Expr* num = expr_new_function(expr_new_symbol("Log"), num_args, 1);
        Expr* den = expr_new_function(expr_new_symbol("Log"), den_args, 1);

        Expr* pow_args[2] = { den, expr_new_integer(-1) };
        Expr* inv_den = expr_new_function(expr_new_symbol("Power"), pow_args, 2);

        Expr* times_args[2] = { num, inv_den };
        Expr* ret = expr_new_function(expr_new_symbol("Times"), times_args, 2);
        return ret;
    }

    // Remains unevaluated if it doesn't match above rules
    return NULL;
}

/*
 * builtin_exp:
 * Implements the evaluation logic for the 'Exp' function.
 */
Expr* builtin_exp(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* z = res->data.function.args[0];

    // Exact evaluations for special constants
    if (z->type == EXPR_INTEGER && z->data.integer == 0) {
        Expr* ret = expr_new_integer(1); // Exp[0] = 1
        return ret;
    }
    if (is_minus_infinity(z)) {
        Expr* ret = expr_new_integer(0); // Exp[-Infinity] = 0
        return ret;
    }
    if (is_infinity(z)) {
        Expr* ret = expr_new_symbol("Infinity"); // Exp[Infinity] = Infinity
        return ret;
    }

    // Exact evaluation of Exp[I * q * Pi] using Euler's formula (e^{i x} = Cos[x] + I Sin[x])
    // The argument z may be internally structured as Times[Complex[0, q], Pi]
    if (z->type == EXPR_FUNCTION && strcmp(z->data.function.head->data.symbol, "Times") == 0) {
        bool has_pi = false;
        Expr* im_coeff = NULL;

        // Scan the arguments of Times to identify a pure imaginary coefficient and Pi
        for (size_t i = 0; i < z->data.function.arg_count; i++) {
            Expr* arg = z->data.function.args[i];
            if (arg->type == EXPR_SYMBOL && strcmp(arg->data.symbol, "Pi") == 0) {
                has_pi = true;
            } else {
                Expr *re, *im;
                if (is_complex(arg, &re, &im)) {
                    // We only rewrite if the complex number is purely imaginary (real part == 0)
                    if (re->type == EXPR_INTEGER && re->data.integer == 0) {
                        im_coeff = im;
                    }
                }
            }
        }

        // If we found Pi and exactly one pure imaginary coefficient (total args = 2)
        if (has_pi && im_coeff && z->data.function.arg_count == 2) {
            int64_t n, d;
            // Only perform expansion if the coefficient is rational or integer (e.g. q * Pi)
            if (im_coeff->type == EXPR_INTEGER || is_rational(im_coeff, &n, &d)) {
                // Construct the angle y = im_coeff * Pi
                Expr* y_args[2] = { expr_copy(im_coeff), expr_new_symbol("Pi") };
                Expr* y = expr_new_function(expr_new_symbol("Times"), y_args, 2);

                // Construct Cos[y]
                Expr* cos_args[1] = { expr_copy(y) };
                Expr* cos_part = expr_new_function(expr_new_symbol("Cos"), cos_args, 1);

                // Construct I * Sin[y]
                Expr* sin_args[1] = { y }; // y ownership is transferred here
                Expr* sin_part = expr_new_function(expr_new_symbol("Sin"), sin_args, 1);

                Expr* i_sym = make_complex(expr_new_integer(0), expr_new_integer(1));
                Expr* i_sin_args[2] = { i_sym, sin_part };
                Expr* i_sin_part = expr_new_function(expr_new_symbol("Times"), i_sin_args, 2);

                // Combine into Plus[Cos[y], Times[I, Sin[y]]]
                Expr* plus_args[2] = { cos_part, i_sin_part };
                Expr* ret = expr_new_function(expr_new_symbol("Plus"), plus_args, 2);
                return ret;
            }
        }
    }

    // Approximate numerical evaluation
    double complex c;
    bool inexact = false;
    if (get_approx(z, &c, &inexact) && inexact) {
        double complex s = cexp(c);
        // Return real result if output is purely real, otherwise return complex
        Expr* ret = NULL;
        if (cimag(c) == 0.0) ret = expr_new_real(creal(s));
        else ret = make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
        return ret;
    }

    // Remains unevaluated if it doesn't match above rules
    return expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_new_symbol("E"), expr_copy(z)}, 2);
}
