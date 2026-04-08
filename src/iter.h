#ifndef ITER_H
#define ITER_H

#include "expr.h"

Expr* builtin_do(Expr* res);
Expr* builtin_for(Expr* res);

void iter_init(void);

#endif
