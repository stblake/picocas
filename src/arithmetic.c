
#include "arithmetic.h"
#include "eval.h"
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

Expr* builtin_rational(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;
    Expr* n_expr = res->data.function.args[0];
    Expr* d_expr = res->data.function.args[1];
    
    if (n_expr->type == EXPR_INTEGER && d_expr->type == EXPR_INTEGER) {
        int64_t n = n_expr->data.integer;
        int64_t d = d_expr->data.integer;
        if (d == 0) {
            if (n == 0) return expr_new_symbol("Indeterminate");
            else return expr_new_symbol("ComplexInfinity");
        }
        
        Expr* r = make_rational(n, d);
        if (r && r->type == EXPR_FUNCTION && r->data.function.head->type == EXPR_SYMBOL && strcmp(r->data.function.head->data.symbol, "Rational") == 0) {
            Expr* rn = r->data.function.args[0];
            Expr* rd = r->data.function.args[1];
            if (rn->type == EXPR_INTEGER && rd->type == EXPR_INTEGER && rn->data.integer == n && rd->data.integer == d) {
                // No simplification happened
                expr_free(r);
                return NULL;
            }
        }
        return r;
    }
    return NULL;
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
        if (vden == 0.0) {
            printf("Power::infy: Infinite expression 1/0 encountered.\n");
            return expr_new_symbol("ComplexInfinity");
        }
        return expr_new_real(vnum / vden);
    }

    int64_t n1, d1, n2, d2;
    if (is_rational(num, &n1, &d1) && is_rational(den, &n2, &d2)) {
        Expr* r = make_rational(n1 * d2, d1 * n2);
        if (r) return r;
    }

    Expr* minus_one = expr_new_integer(-1);
    Expr* p_args[2] = { expr_copy(den), minus_one };
    Expr* power = expr_new_function(expr_new_symbol("Power"), p_args, 2);
    
    Expr* t_args[2] = { expr_copy(num), power };
    return expr_new_function(expr_new_symbol("Times"), t_args, 2);
}

static int64_t ext_gcd(int64_t a, int64_t b, int64_t *x, int64_t *y) {
    if (a == 0) {
        *x = 0; *y = 1;
        return b;
    }
    int64_t x1, y1;
    int64_t d = ext_gcd(b % a, a, &x1, &y1);
    *x = y1 - (b / a) * x1;
    *y = x1;
    return d;
}

static int64_t mod_inverse(int64_t a, int64_t m) {
    if (m == 0) return -1;
    if (m < 0) m = -m;
    if (m == 1) return 0;
    a %= m;
    if (a < 0) a += m;
    int64_t x, y;
    int64_t g = ext_gcd(a, m, &x, &y);
    if (g != 1) return -1;
    return (x % m + m) % m;
}

static int64_t mod_pow(int64_t a, int64_t b, int64_t m) {
    if (m == 0) return 0;
    if (m < 0) m = -m;
    if (m == 1) return 0;
    a %= m;
    if (a < 0) a += m;
    int64_t res = 1;
    while (b > 0) {
        if (b & 1) res = (int64_t)((__int128_t)res * a % m);
        a = (int64_t)((__int128_t)a * a % m);
        b >>= 1;
    }
    return res;
}

Expr* builtin_powermod(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 3) return NULL;
    
    Expr* a_expr = res->data.function.args[0];
    Expr* b_expr = res->data.function.args[1];
    Expr* m_expr = res->data.function.args[2];
    
    if (a_expr->type != EXPR_INTEGER || m_expr->type != EXPR_INTEGER) return NULL;
    int64_t a = a_expr->data.integer;
    int64_t m = m_expr->data.integer;
    if (m == 0) return NULL;
    
    if (b_expr->type == EXPR_INTEGER) {
        int64_t b = b_expr->data.integer;
        if (b < 0) {
            int64_t inv = mod_inverse(a, m);
            if (inv == -1) return expr_copy(res); // Unevaluated if no inverse
            return expr_new_integer(mod_pow(inv, -b, m));
        } else {
            return expr_new_integer(mod_pow(a, b, m));
        }
    } else if (b_expr->type == EXPR_FUNCTION && b_expr->data.function.head->type == EXPR_SYMBOL && strcmp(b_expr->data.function.head->data.symbol, "Rational") == 0 && b_expr->data.function.arg_count == 2) {
        Expr* num = b_expr->data.function.args[0];
        Expr* den = b_expr->data.function.args[1];
        if (num->type == EXPR_INTEGER && num->data.integer == 1 && den->type == EXPR_INTEGER) {
            int64_t r = den->data.integer;
            if (r > 0) {
                int64_t target_a = a % m;
                if (target_a < 0) target_a += m;
                int64_t abs_m = m < 0 ? -m : m;
                
                // Brute force root finding (only up to a reasonable cap to avoid hanging)
                if (abs_m <= 1000000LL) {
                    for (int64_t x = 0; x < abs_m; x++) {
                        if (mod_pow(x, r, abs_m) == target_a) {
                            return expr_new_integer(x);
                        }
                    }
                    return expr_copy(res); // Unevaluated if no root
                }
            }
        }
    }
    
    return NULL;
}

Expr* builtin_factorial(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];

    int64_t n, d;
    if (is_rational(arg, &n, &d)) {
        if (d == 1) {
            if (n < 0) return expr_new_symbol("ComplexInfinity");
            if (n <= 20) {
                int64_t f = 1;
                for (int64_t i = 2; i <= n; i++) f *= i;
                return expr_new_integer(f);
            } else {
                return expr_copy(res);
            }
        } else if (d == 2 || d == -2) {
            if (d == -2) { n = -n; }
            int64_t num = 1;
            int64_t den = 1;
            
            if (n > 0) {
                for (int64_t i = n; i >= 1; i -= 2) num *= i;
                den = 1LL << ((n + 1) / 2);
            } else {
                for (int64_t i = n + 2; i <= -1; i += 2) {
                    num *= 2;
                    den *= i;
                }
            }
            
            Expr* coeff = make_rational(num, den);
            if (!coeff) coeff = expr_new_integer(0);
            
            Expr* pi_sym = expr_new_symbol("Pi");
            Expr* half = make_rational(1, 2);
            Expr* sqrt_pi = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){pi_sym, half}, 2));
            
            if (coeff->type == EXPR_INTEGER && coeff->data.integer == 1) {
                expr_free(coeff);
                return sqrt_pi;
            } else {
                return eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){coeff, sqrt_pi}, 2));
            }
        }
    }

    return NULL;
}

Expr* builtin_binomial(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 2) return NULL;
    Expr* arg_n = res->data.function.args[0];
    Expr* arg_m = res->data.function.args[1];

    if (arg_n->type == EXPR_REAL || arg_m->type == EXPR_REAL) {
        double n_val = (arg_n->type == EXPR_REAL) ? arg_n->data.real : (double)arg_n->data.integer;
        double m_val = (arg_m->type == EXPR_REAL) ? arg_m->data.real : (double)arg_m->data.integer;
        double val = tgamma(n_val + 1.0) / (tgamma(m_val + 1.0) * tgamma(n_val - m_val + 1.0));
        return expr_new_real(val);
    }

    int64_t n_num = 0, n_den = 1, m_num = 0, m_den = 1;
    bool n_is_rat = false;
    if (arg_n->type == EXPR_INTEGER) { n_num = arg_n->data.integer; n_den = 1; n_is_rat = true; }
    else if (is_rational(arg_n, &n_num, &n_den)) { n_is_rat = true; }

    bool m_is_rat = false;
    if (arg_m->type == EXPR_INTEGER) { m_num = arg_m->data.integer; m_den = 1; m_is_rat = true; }
    else if (is_rational(arg_m, &m_num, &m_den)) { m_is_rat = true; }
    
    if (n_is_rat && m_is_rat) {
        if (m_den == 1 && n_den == 1) {
            int64_t m = m_num;
            if (m < 0) return expr_new_integer(0);
            
            int64_t n = n_num;
            if (n >= 0 && m > n) return expr_new_integer(0);
            
            int64_t sign = 1;
            if (n < 0) {
                if (m % 2 != 0) sign = -1;
                n = -n + m - 1;
            }
            
            if (m > n / 2) m = n - m;
            
            int64_t result = 1;
            for (int64_t i = 1; i <= m; i++) {
                int64_t next_num = n - i + 1;
                if (result > INT64_MAX / next_num) {
                    return expr_new_function(expr_new_symbol("Overflow"), NULL, 0);
                }
                result = result * next_num / i;
            }
            return expr_new_integer(result * sign);
        }
        
        int64_t diff_num = n_num * m_den - m_num * n_den;
        int64_t diff_den = n_den * m_den;
        if (diff_num % diff_den == 0) {
            int64_t k = diff_num / diff_den;
            if (k < 0) return expr_new_integer(0);
            Expr* new_m = expr_new_integer(k);
            Expr* new_call = eval_and_free(expr_new_function(expr_new_symbol("Binomial"), (Expr*[]){expr_copy(arg_n), new_m}, 2));
            return new_call;
        }
    }
    
    if (m_is_rat && m_den == 1) {
        int64_t m = m_num;
        if (m < 0) return expr_new_integer(0);
        if (m == 0) return expr_new_integer(1);
        if (m == 1) return expr_copy(arg_n);
        
        Expr** args = malloc(sizeof(Expr*) * m);
        for (int64_t i = 0; i < m; i++) {
            if (i == 0) {
                args[i] = expr_copy(arg_n);
            } else {
                args[i] = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(arg_n), expr_new_integer(-i)}, 2));
            }
        }
        Expr* num = eval_and_free(expr_new_function(expr_new_symbol("Times"), args, m));
        free(args);
        
        Expr* den = eval_and_free(expr_new_function(expr_new_symbol("Factorial"), (Expr*[]){expr_new_integer(m)}, 1));
        Expr* den_inv = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){den, expr_new_integer(-1)}, 2));
        return eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){num, den_inv}, 2));
    }
    
    return NULL;
}
