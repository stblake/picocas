#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <complex.h>
#include <stdint.h>
#include "hyperbolic.h"
#include "symtab.h"
#include "eval.h"
#include "arithmetic.h"
#include "complex.h"

void hyperbolic_init(void) {
    symtab_add_builtin("Sinh", builtin_sinh);
    symtab_add_builtin("Cosh", builtin_cosh);
    symtab_add_builtin("Tanh", builtin_tanh);
    symtab_add_builtin("Coth", builtin_coth);
    symtab_add_builtin("Sech", builtin_sech);
    symtab_add_builtin("Csch", builtin_csch);
    
    symtab_add_builtin("ArcSinh", builtin_arcsinh);
    symtab_add_builtin("ArcCosh", builtin_arccosh);
    symtab_add_builtin("ArcTanh", builtin_arctanh);
    symtab_add_builtin("ArcCoth", builtin_arccoth);
    symtab_add_builtin("ArcSech", builtin_arcsech);
    symtab_add_builtin("ArcCsch", builtin_arccsch);

    const char* hypers[] = {"Sinh", "Cosh", "Tanh", "Coth", "Sech", "Csch", 
                            "ArcSinh", "ArcCosh", "ArcTanh", "ArcCoth", "ArcSech", "ArcCsch", NULL};
    for (int i = 0; hypers[i] != NULL; i++) {
        symtab_get_def(hypers[i])->attributes |= (ATTR_PROTECTED | ATTR_NUMERICFUNCTION | ATTR_LISTABLE);
    }
}

static bool get_approx(Expr* e, double complex* out, bool* is_inexact) {
    if (is_inexact) *is_inexact = true;
    if (e->type == EXPR_REAL) {
        *out = e->data.real + 0.0 * I;
        return true;
    }
    Expr* re; Expr* im;
    if (is_complex(e, &re, &im)) {
        bool has_real = false;
        double r = 0.0, i = 0.0;
        int64_t n, d;
        if (re->type == EXPR_REAL) { r = re->data.real; has_real = true; }
        else if (re->type == EXPR_INTEGER) r = (double)re->data.integer;
        else if (is_rational(re, &n, &d)) r = (double)n / d;
        else return false;
        
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

static bool is_infinity(Expr* e) {
    return e->type == EXPR_SYMBOL && strcmp(e->data.symbol, "Infinity") == 0;
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

Expr* builtin_sinh(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_integer(0);
    if (is_infinity(arg)) return expr_new_symbol("Infinity");
    if (is_minus_infinity(arg)) {
        Expr* args[2] = { expr_new_integer(-1), expr_new_symbol("Infinity") };
        return expr_new_function(expr_new_symbol("Times"), args, 2);
    }

    double complex c;
    bool inexact = false;
    if (get_approx(arg, &c, &inexact) && inexact) {
        double complex s = csinh(c);
        if (cimag(c) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

Expr* builtin_cosh(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_integer(1);
    if (is_infinity(arg) || is_minus_infinity(arg)) return expr_new_symbol("Infinity");

    double complex c;
    bool inexact = false;
    if (get_approx(arg, &c, &inexact) && inexact) {
        double complex s = ccosh(c);
        if (cimag(c) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

Expr* builtin_tanh(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_integer(0);
    if (is_infinity(arg)) return expr_new_integer(1);
    if (is_minus_infinity(arg)) return expr_new_integer(-1);

    double complex c;
    bool inexact = false;
    if (get_approx(arg, &c, &inexact) && inexact) {
        double complex s = ctanh(c);
        if (cimag(c) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

Expr* builtin_coth(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_symbol("ComplexInfinity");
    if (is_infinity(arg)) return expr_new_integer(1);
    if (is_minus_infinity(arg)) return expr_new_integer(-1);

    double complex c;
    bool inexact = false;
    if (get_approx(arg, &c, &inexact) && inexact) {
        double complex s = 1.0 / ctanh(c);
        if (cimag(c) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

Expr* builtin_sech(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_integer(1);
    if (is_infinity(arg) || is_minus_infinity(arg)) return expr_new_integer(0);

    double complex c;
    bool inexact = false;
    if (get_approx(arg, &c, &inexact) && inexact) {
        double complex s = 1.0 / ccosh(c);
        if (cimag(c) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

Expr* builtin_csch(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_symbol("ComplexInfinity");
    if (is_infinity(arg) || is_minus_infinity(arg)) return expr_new_integer(0);

    double complex c;
    bool inexact = false;
    if (get_approx(arg, &c, &inexact) && inexact) {
        double complex s = 1.0 / csinh(c);
        if (cimag(c) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

Expr* builtin_arcsinh(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_integer(0);
    if (is_infinity(arg)) return expr_new_symbol("Infinity");

    double complex c;
    bool inexact = false;
    if (get_approx(arg, &c, &inexact) && inexact) {
        double complex s = casinh(c);
        if (cimag(c) == 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

Expr* builtin_arccosh(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    if (arg->type == EXPR_INTEGER && arg->data.integer == 1) return expr_new_integer(0);
    if (is_infinity(arg)) return expr_new_symbol("Infinity");

    double complex c;
    bool inexact = false;
    if (get_approx(arg, &c, &inexact) && inexact) {
        double complex s = cacosh(c);
        if (cimag(c) == 0.0 && creal(c) >= 1.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

Expr* builtin_arctanh(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    if (arg->type == EXPR_INTEGER && arg->data.integer == 0) return expr_new_integer(0);

    double complex c;
    bool inexact = false;
    if (get_approx(arg, &c, &inexact) && inexact) {
        double complex s = catanh(c);
        if (cimag(c) == 0.0 && creal(c) > -1.0 && creal(c) < 1.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

Expr* builtin_arccoth(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    if (is_infinity(arg) || is_minus_infinity(arg)) return expr_new_integer(0);

    double complex c;
    bool inexact = false;
    if (get_approx(arg, &c, &inexact) && inexact) {
        double complex s = catanh(1.0 / c);
        if (cimag(c) == 0.0 && (creal(c) > 1.0 || creal(c) < -1.0)) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

Expr* builtin_arcsech(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    if (arg->type == EXPR_INTEGER && arg->data.integer == 1) return expr_new_integer(0);

    double complex c;
    bool inexact = false;
    if (get_approx(arg, &c, &inexact) && inexact) {
        double complex s = cacosh(1.0 / c);
        if (cimag(c) == 0.0 && creal(c) > 0.0 && creal(c) <= 1.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}

Expr* builtin_arccsch(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    
    if (is_infinity(arg) || is_minus_infinity(arg)) return expr_new_integer(0);

    double complex c;
    bool inexact = false;
    if (get_approx(arg, &c, &inexact) && inexact) {
        double complex s = casinh(1.0 / c);
        if (cimag(c) == 0.0 && creal(c) != 0.0) return expr_new_real(creal(s));
        return make_complex(expr_new_real(creal(s)), expr_new_real(cimag(s)));
    }
    return NULL;
}
