
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

typedef enum {
    EXPR_INTEGER,
    EXPR_REAL,
    EXPR_SYMBOL,
    EXPR_STRING,
    EXPR_FUNCTION,
    EXPR_BIGINT
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

/* Helpers used by arithmetic modules */
void  expr_to_mpz(const Expr* e, mpz_t out);
bool  expr_is_integer_like(const Expr* e);
Expr* expr_bigint_normalize(Expr* e);

#endif // EXPR_H
