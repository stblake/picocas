#ifndef RANDOM_H
#define RANDOM_H

#include "expr.h"

Expr* builtin_randominteger(Expr* res);
Expr* builtin_seedrandom(Expr* res);

void random_init(void);

#endif // RANDOM_H
