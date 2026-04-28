#ifndef FACPOLY_H
#define FACPOLY_H

#include "expr.h"

Expr* builtin_factorsquarefree(Expr* res);
Expr* builtin_factor(Expr* res);
Expr* builtin_factorterms(Expr* res);
Expr* builtin_factortermslist(Expr* res);
void facpoly_init(void);

#endif // FACPOLY_H
