#ifndef HYPERBOLIC_H
#define HYPERBOLIC_H

#include "expr.h"

Expr* builtin_sinh(Expr* res);
Expr* builtin_cosh(Expr* res);
Expr* builtin_tanh(Expr* res);
Expr* builtin_coth(Expr* res);
Expr* builtin_sech(Expr* res);
Expr* builtin_csch(Expr* res);

Expr* builtin_arcsinh(Expr* res);
Expr* builtin_arccosh(Expr* res);
Expr* builtin_arctanh(Expr* res);
Expr* builtin_arccoth(Expr* res);
Expr* builtin_arcsech(Expr* res);
Expr* builtin_arccsch(Expr* res);

void hyperbolic_init(void);

#endif // HYPERBOLIC_H
