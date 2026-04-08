#include "facint.h"
#include "arithmetic.h"
#include "expr.h"
#include "eval.h"
#include "symtab.h"
#include "attr.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

// Modular multiplication: (a * b) % m
static uint64_t mul_mod(uint64_t a, uint64_t b, uint64_t m) {
    return (uint64_t)((__uint128_t)a * b % m);
}

// Modular exponentiation: (base^exp) % mod
static uint64_t power_mod(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t res = 1;
    base %= mod;
    while (exp > 0) {
        if (exp % 2 == 1) res = mul_mod(res, base, mod);
        base = mul_mod(base, base, mod);
        exp /= 2;
    }
    return res;
}

// Miller-Rabin check for a single base
static bool miller_rabin_check(uint64_t n, uint64_t d, int s, uint64_t a) {
    uint64_t x = power_mod(a, d, n);
    if (x == 1 || x == n - 1) return true;

    for (int r = 1; r < s; r++) {
        x = mul_mod(x, x, n);
        if (x == n - 1) return true;
    }
    return false;
}

// Deterministic Miller-Rabin primality test for n < 2^64
static bool is_prime_internal(uint64_t n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0) return false;

    uint64_t d = n - 1;
    int s = 0;
    while (d % 2 == 0) {
        d /= 2;
        s++;
    }

    // Bases for deterministic test up to 2^64
    static const uint64_t bases[] = {2, 325, 9375, 28178, 450775, 9780504, 1795265022};
    
    for (int i = 0; i < 7; i++) {
        uint64_t a = bases[i] % n;
        if (a == 0) continue;
        if (!miller_rabin_check(n, d, s, a)) return false;
    }
    return true;
}

#define MAX_PRIME_LIMIT 1000000 
#define CACHE_A 100
#define CACHE_X 20000

static uint32_t *pi_primes = NULL;
static uint32_t total_pi_primes = 0;
static int32_t **phi_cache = NULL;

static void init_primepi() {
    if (pi_primes) return;

    uint8_t *is_prime = calloc(MAX_PRIME_LIMIT + 1, sizeof(uint8_t));
    for (uint32_t i = 2; i <= MAX_PRIME_LIMIT; i++) is_prime[i] = 1;
    for (uint32_t p = 2; p * p <= MAX_PRIME_LIMIT; p++) {
        if (is_prime[p]) {
            for (uint32_t i = p * p; i <= MAX_PRIME_LIMIT; i += p)
                is_prime[i] = 0;
        }
    }
    for (uint32_t i = 2; i <= MAX_PRIME_LIMIT; i++) {
        if (is_prime[i]) total_pi_primes++;
    }
    pi_primes = malloc(total_pi_primes * sizeof(uint32_t));
    uint32_t idx = 0;
    for (uint32_t i = 2; i <= MAX_PRIME_LIMIT; i++) {
        if (is_prime[i]) pi_primes[idx++] = i;
    }
    free(is_prime);

    phi_cache = malloc(CACHE_A * sizeof(int32_t*));
    for (int i = 0; i < CACHE_A; i++) {
        phi_cache[i] = calloc(CACHE_X, sizeof(int32_t));
    }
}

static uint64_t pi_base(uint64_t x) {
    if (x < 2) return 0;
    if (x > MAX_PRIME_LIMIT) {
        if (x > pi_primes[total_pi_primes - 1]) x = pi_primes[total_pi_primes - 1];
    }
    
    int64_t low = 0, high = (int64_t)total_pi_primes - 1, mid;
    while (low <= high) {
        mid = low + (high - low) / 2;
        if (pi_primes[mid] <= x) low = mid + 1;
        else high = mid - 1;
    }
    return (uint64_t)low; 
}

static uint64_t phi_rec(uint64_t x, uint64_t a) {
    if (a == 0) return x;
    if (a == 1) return x - x / 2;

    if (a < CACHE_A && x < CACHE_X) {
        if (phi_cache[a][x] != 0) return (uint64_t)phi_cache[a][x];
    }

    uint64_t result = phi_rec(x, a - 1) - phi_rec(x / pi_primes[a - 1], a - 1);

    if (a < CACHE_A && x < CACHE_X) {
        phi_cache[a][x] = (int32_t)result;
    }
    return result;
}

static uint64_t p2_calc(uint64_t x, uint64_t a) {
    uint64_t sum = 0;
    uint64_t b = pi_base((uint64_t)sqrt((double)x));
    for (uint64_t i = a + 1; i <= b; i++) {
        sum += pi_base(x / pi_primes[i - 1]) - i + 1;
    }
    return sum;
}

static uint64_t count_primes(uint64_t x) {
    if (x <= MAX_PRIME_LIMIT) return pi_base(x);
    init_primepi();
    if (x <= MAX_PRIME_LIMIT) return pi_base(x);

    uint64_t a = pi_base((uint64_t)cbrt((double)x));
    uint64_t total_phi = phi_rec(x, a);
    uint64_t total_p2 = p2_calc(x, a);

    return total_phi + a - 1 - total_p2;
}

Expr* builtin_primepi(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* x_expr = res->data.function.args[0];

    int64_t x_val;
    if (x_expr->type == EXPR_INTEGER) {
        x_val = x_expr->data.integer;
    } else if (x_expr->type == EXPR_REAL) {
        x_val = (int64_t)floor(x_expr->data.real);
    } else {
        int64_t n, d;
        if (is_rational(x_expr, &n, &d)) {
            x_val = n / d;
            if (n < 0 && n % d != 0) x_val--;
        } else {
            return NULL;
        }
    }

    if (x_val < 2) return expr_new_integer(0);
    init_primepi();
    uint64_t result = count_primes((uint64_t)x_val);
    return expr_new_integer((int64_t)result);
}

Expr* builtin_primeq(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];

    if (arg->type == EXPR_INTEGER) {
        if (is_prime_internal((uint64_t)llabs(arg->data.integer))) {
            return expr_new_symbol("True");
        } else {
            return expr_new_symbol("False");
        }
    }

    if (arg->type == EXPR_REAL || arg->type == EXPR_STRING) {
        return expr_new_symbol("False");
    }
    if (arg->type == EXPR_FUNCTION && arg->data.function.head->type == EXPR_SYMBOL) {
        const char* head = arg->data.function.head->data.symbol;
        if (strcmp(head, "Rational") == 0 || strcmp(head, "Complex") == 0) {
            return expr_new_symbol("False");
        }
    }

    return NULL;
}

Expr* builtin_nextprime(Expr* res) {
    if (res->type != EXPR_FUNCTION || (res->data.function.arg_count != 1 && res->data.function.arg_count != 2)) return NULL;

    Expr* x_expr = res->data.function.args[0];
    int64_t k = 1;
    if (res->data.function.arg_count == 2) {
        if (res->data.function.args[1]->type != EXPR_INTEGER) return NULL;
        k = res->data.function.args[1]->data.integer;
    }

    if (k == 0) return expr_copy(x_expr);

    int64_t start_val;
    if (x_expr->type == EXPR_INTEGER) {
        start_val = x_expr->data.integer;
    } else if (x_expr->type == EXPR_REAL) {
        start_val = (int64_t)floor(x_expr->data.real);
    } else {
        int64_t n, d;
        if (is_rational(x_expr, &n, &d)) {
            start_val = n / d;
            if (n < 0 && n % d != 0) start_val--; 
        } else {
            return NULL;
        }
    }

    uint64_t current = (uint64_t)start_val;
    int64_t count = llabs(k);

    if (k > 0) {
        current = (start_val < 2) ? 2 : (uint64_t)start_val;
        if (start_val < 2) count--;

        while (count > 0) {
            if (current < 2) current = 2;
            else if (current == 2) current = 3;
            else {
                current++;
                if (current % 2 == 0) current++;
                while (!is_prime_internal(current)) {
                    current += 2;
                }
            }
            count--;
        }
    } else {
        if (start_val <= 2) return NULL;
        
        while (count > 0) {
            if (current == 3) current = 2;
            else if (current <= 2) return NULL; 
            else {
                if (current % 2 == 0) current--;
                else current -= 2;
                
                while (current > 2 && !is_prime_internal(current)) {
                    current -= 2;
                }
                if (current < 2) return NULL;
            }
            count--;
        }
    }

    return expr_new_integer((int64_t)current);
}

static uint64_t abs_diff(uint64_t a, uint64_t b) {
    return a > b ? a - b : b - a;
}

static uint64_t pollard_rho_brent(uint64_t n) {
    if (n % 2 == 0) return 2;
    if (is_prime_internal(n)) return n;

    uint64_t y_start = 2; 
    uint64_t c = 1;       
    uint64_t m = 128;     

    while (1) {
        uint64_t x, y, g, r, q, ys;
        x = y = y_start;
        g = r = q = 1;

        while (g == 1) {
            x = y;
            for (uint64_t i = 0; i < r; i++) {
                y = (mul_mod(y, y, n) + c) % n;
            }

            uint64_t k = 0;
            while (k < r && g == 1) {
                ys = y;
                uint64_t limit = (m < r - k) ? m : r - k;
                for (uint64_t i = 0; i < limit; i++) {
                    y = (mul_mod(y, y, n) + c) % n;
                    q = mul_mod(q, abs_diff(x, y), n);
                }
                g = gcd(q, n);
                k += limit;
            }
            r *= 2;
        }

        if (g == n) {
            do {
                ys = (mul_mod(ys, ys, n) + c) % n;
                g = gcd(abs_diff(x, ys), n);
            } while (g == 1);
        }

        if (g != n) return g;

        y_start++;
        c++;
    }
}


#include <gmp.h>
#include "ecm.h"

#include <gmp.h>
#include "ecm.h"

typedef struct {
    mpz_t p;
    int64_t count;
} FactorMpz;

static void add_factor_mpz(FactorMpz* factors, int* num_factors, mpz_t p, int64_t count) {
    for (int i = 0; i < *num_factors; i++) {
        if (mpz_cmp(factors[i].p, p) == 0) {
            factors[i].count += count;
            return;
        }
    }
    mpz_init_set(factors[*num_factors].p, p);
    factors[*num_factors].count = count;
    (*num_factors)++;
}

static void factorize_mpz(mpz_t n, FactorMpz* factors, int* num_factors, int* k_limit) {
    if (mpz_cmp_ui(n, 1) <= 0) return;
    if (*k_limit > 0 && *num_factors >= *k_limit) return;

    if (mpz_probab_prime_p(n, 25)) {
        add_factor_mpz(factors, num_factors, n, 1);
        return;
    }

    // Trial division for small primes
    static const uint32_t small_primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
    int n_small = sizeof(small_primes)/sizeof(small_primes[0]);
    for (int i = 0; i < n_small; i++) {
        if (mpz_divisible_ui_p(n, small_primes[i])) {
            int64_t count = 0;
            while (mpz_divisible_ui_p(n, small_primes[i])) {
                count++;
                mpz_divexact_ui(n, n, small_primes[i]);
            }
            mpz_t sp;
            mpz_init_set_ui(sp, small_primes[i]);
            add_factor_mpz(factors, num_factors, sp, count);
            mpz_clear(sp);
            
            if (mpz_cmp_ui(n, 1) == 0) return;
            if (mpz_probab_prime_p(n, 25)) {
                add_factor_mpz(factors, num_factors, n, 1);
                return;
            }
            if (*k_limit > 0 && *num_factors >= *k_limit) return;
        }
    }

    mpz_t f;
    mpz_init(f);
    ecm_params params;
    ecm_init(params);
    params->B1done = 100.0; /* We can do a quick check with small B1 */

    int found = 0;
    // Try ECM with increasing bounds
    double B1_bounds[] = {2000.0, 11000.0, 50000.0, 250000.0, 1000000.0, 3000000.0, 11000000.0};
    int num_bounds = sizeof(B1_bounds)/sizeof(B1_bounds[0]);
    
    for (int i = 0; i < num_bounds && !found; i++) {
        for (int tries = 0; tries < 10 && !found; tries++) {
            if (ecm_factor(f, n, B1_bounds[i], params) > 0) {
                found = 1;
            }
        }
    }
    
    ecm_clear(params);


    if (found) {
        if (mpz_cmp_ui(f, 1) == 0 || mpz_cmp(f, n) == 0) {
            add_factor_mpz(factors, num_factors, n, 1);
            mpz_clear(f);
            return;
        }
        mpz_t n_f;
        mpz_init(n_f);
        mpz_divexact(n_f, n, f);
        
        factorize_mpz(f, factors, num_factors, k_limit);
        factorize_mpz(n_f, factors, num_factors, k_limit);
        mpz_clear(n_f);
    } else {
        // If ECM fails to find a factor within reasonable bounds, just add n as a factor
        add_factor_mpz(factors, num_factors, n, 1);
    }
    mpz_clear(f);
}

static int compare_factors_mpz(const void* a, const void* b) {
    const FactorMpz* fa = (const FactorMpz*)a;
    const FactorMpz* fb = (const FactorMpz*)b;
    return mpz_cmp(fa->p, fb->p);
}

Expr* builtin_factorinteger(Expr* res) {
    if (res->type != EXPR_FUNCTION || (res->data.function.arg_count != 1 && res->data.function.arg_count != 2)) return NULL;
    
    Expr* n_expr = res->data.function.args[0];
    mpz_t num, den;
    bool is_rat = false;

    if (n_expr->type == EXPR_INTEGER || n_expr->type == EXPR_BIGINT) {
        expr_to_mpz(n_expr, num);
        mpz_init_set_ui(den, 1);
        is_rat = true;
    } else {
        int64_t n_num, n_den;
        if (is_rational(n_expr, &n_num, &n_den)) {
            mpz_init_set_si(num, n_num);
            mpz_init_set_si(den, n_den);
            is_rat = true;
        }
    }

    if (!is_rat) return NULL;

    int k_limit = -1;
    if (res->data.function.arg_count == 2) {
        Expr* k_expr = res->data.function.args[1];
        if (k_expr->type == EXPR_INTEGER) {
            k_limit = (int)k_expr->data.integer;
        } else if (k_expr->type == EXPR_SYMBOL && strcmp(k_expr->data.symbol, "Automatic") == 0) {
            
        } else {
            mpz_clear(num); mpz_clear(den);
            return NULL;
        }
    }

    FactorMpz factors[1024];
    int num_factors = 0;

    if (mpz_cmp_ui(num, 0) < 0) {
        mpz_t minus_one;
        mpz_init_set_si(minus_one, -1);
        add_factor_mpz(factors, &num_factors, minus_one, 1);
        mpz_clear(minus_one);
        mpz_neg(num, num);
    } else if (mpz_cmp_ui(num, 0) == 0) {
        mpz_t zero;
        mpz_init_set_ui(zero, 0);
        add_factor_mpz(factors, &num_factors, zero, 1);
        mpz_clear(zero);
        mpz_set_ui(num, 1);
    }

    // Since ECM extracts small factors first, Automatic doesn't need to be treated differently.
    factorize_mpz(num, factors, &num_factors, &k_limit);
    
    if (mpz_cmp_ui(den, 1) > 0) {
        FactorMpz d_factors[1024];
        int d_num_factors = 0;
        int d_limit = -1;
        factorize_mpz(den, d_factors, &d_num_factors, &d_limit);
        for (int i = 0; i < d_num_factors; i++) {
            add_factor_mpz(factors, &num_factors, d_factors[i].p, -d_factors[i].count);
            mpz_clear(d_factors[i].p);
        }
    }

    qsort(factors, num_factors, sizeof(FactorMpz), compare_factors_mpz);

    Expr** list_args = malloc(sizeof(Expr*) * num_factors);
    for (int i = 0; i < num_factors; i++) {
        Expr** pair = malloc(sizeof(Expr*) * 2);
        pair[0] = expr_bigint_normalize(expr_new_bigint_from_mpz(factors[i].p));
        pair[1] = expr_new_integer(factors[i].count);
        list_args[i] = expr_new_function(expr_new_symbol("List"), pair, 2);
        free(pair);
        mpz_clear(factors[i].p);
    }

    Expr* result = expr_new_function(expr_new_symbol("List"), list_args, num_factors);
    free(list_args);
    mpz_clear(num); mpz_clear(den);
    return result;
}

Expr* builtin_eulerphi(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];
    if (arg->type != EXPR_INTEGER) return NULL;

    int64_t val = arg->data.integer;
    if (val == 0) return expr_new_integer(0);
    if (val < 0) val = -val;
    if (val == 1) return expr_new_integer(1);

    uint64_t n = (uint64_t)val;
    uint64_t result = n;

    uint64_t d_primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    for (int i = 0; i < 12; i++) {
        uint64_t p = d_primes[i];
        if (n % p == 0) {
            while (n % p == 0) n /= p;
            result -= result / p;
        }
    }

    while (n > 1) {
        uint64_t d;
        if (is_prime_internal(n)) {
            d = n;
        } else {
            d = pollard_rho_brent(n);
            while (!is_prime_internal(d)) {
                d = pollard_rho_brent(d);
            }
        }
        while (n % d == 0) n /= d;
        result -= result / d;
    }

    return expr_new_integer((int64_t)result);
}

void facint_init(void) {
    symtab_add_builtin("PrimeQ", builtin_primeq);
    symtab_add_builtin("PrimePi", builtin_primepi);
    symtab_add_builtin("FactorInteger", builtin_factorinteger);
    symtab_add_builtin("NextPrime", builtin_nextprime);
    symtab_add_builtin("EulerPhi", builtin_eulerphi);

    symtab_get_def("PrimeQ")->attributes |= (ATTR_PROTECTED | ATTR_LISTABLE);
    symtab_get_def("PrimePi")->attributes |= (ATTR_PROTECTED | ATTR_LISTABLE);
    symtab_get_def("FactorInteger")->attributes |= (ATTR_PROTECTED | ATTR_LISTABLE);
    symtab_get_def("EulerPhi")->attributes |= (ATTR_PROTECTED | ATTR_LISTABLE);
    symtab_get_def("NextPrime")->attributes |= (ATTR_PROTECTED | ATTR_READPROTECTED | ATTR_LISTABLE);
}
