
// PicoCAS

#ifndef EXPR_H
#define EXPR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <math.h>
#include <gmp.h>
#ifdef USE_MPFR
#include <mpfr.h>
#endif

typedef enum {
    EXPR_INTEGER,
    EXPR_REAL,
    EXPR_SYMBOL,
    EXPR_STRING,
    EXPR_FUNCTION,
    EXPR_BIGINT
#ifdef USE_MPFR
    , EXPR_MPFR                /* arbitrary-precision real (MPFR) */
#endif
} ExprType;

typedef struct Expr {
    ExprType type;
    union {
        int64_t integer;
        double real;
        char* symbol;
        char* string;
        struct {
            struct Expr* head;
            struct Expr** args;
            size_t arg_count;
        } function;
        mpz_t bigint;
#ifdef USE_MPFR
        mpfr_t mpfr;          /* carries its own precision in bits */
#endif
    } data;
} Expr;

Expr* expr_new_integer(int64_t value);
Expr* expr_new_real(double value);
Expr* expr_new_symbol(const char* name);
Expr* expr_new_string(const char* str);
Expr* expr_new_function(Expr* head, Expr** args, size_t arg_count);
void expr_free(Expr* e);
Expr* expr_copy(Expr* e);

#include <stdbool.h>
bool expr_eq(const Expr* a, const Expr* b);
int expr_compare(const Expr* a, const Expr* b);
uint64_t expr_hash(const Expr* e);

/* BigInt constructors */
Expr* expr_new_bigint_from_mpz(const mpz_t val);
Expr* expr_new_bigint_from_int64(int64_t val);
Expr* expr_new_bigint_from_str(const char* str);

#ifdef USE_MPFR
/* MPFR constructors. Each allocates an Expr whose payload `mpfr_t` is
 * initialized to the requested precision. Caller must `expr_free` when
 * done, which calls `mpfr_clear`. */
Expr* expr_new_mpfr_bits(mpfr_prec_t bits);                       /* zero */
Expr* expr_new_mpfr_from_d(double v, mpfr_prec_t bits);
Expr* expr_new_mpfr_from_si(long v, mpfr_prec_t bits);
Expr* expr_new_mpfr_from_mpz(const mpz_t z, mpfr_prec_t bits);
Expr* expr_new_mpfr_from_str(const char* str, mpfr_prec_t bits);
/* Build an Expr taking ownership of `src`. The mpfr_t is moved, not copied;
 * afterwards the caller must not touch `src`. Precision is inherited. */
Expr* expr_new_mpfr_move(mpfr_t src);
/* Copy constructor: new Expr with an independent mpfr_t at the same
 * precision as `src`. */
Expr* expr_new_mpfr_copy(const mpfr_t src);
#endif

/* Helpers used by arithmetic modules */
void  expr_to_mpz(const Expr* e, mpz_t out);
bool  expr_is_integer_like(const Expr* e);
/* True if `e` represents any concrete number: Integer, BigInt, Real,
 * Rational[n,d], Complex with numeric parts, or (with USE_MPFR) MPFR. */
bool  expr_is_numeric_like(const Expr* e);
Expr* expr_bigint_normalize(Expr* e);

#endif // EXPR_H
