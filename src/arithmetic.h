#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include "expr.h"

// GCD helper
int64_t gcd(int64_t a, int64_t b);

// LCM helper
int64_t lcm(int64_t a, int64_t b);

// Create a simplified Rational or Integer
Expr* make_rational(int64_t n, int64_t d);

// Check if expression is a Rational[...]
bool is_rational(Expr* e, int64_t* n, int64_t* d);

// Check if expression is a Complex[re, im]
bool is_complex(Expr* e, Expr** re, Expr** im);

// Create a Complex expression
Expr* make_complex(Expr* re, Expr* im);

Expr* builtin_divide(Expr* res);
Expr* builtin_subtract(Expr* res);
Expr* builtin_complex(Expr* res);
Expr* builtin_gcd(Expr* res);
Expr* builtin_lcm(Expr* res);
Expr* builtin_powermod(Expr* res);
Expr* builtin_factorial(Expr* res);
Expr* builtin_binomial(Expr* res);

#endif
