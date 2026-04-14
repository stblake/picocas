#ifndef RANDOM_H
#define RANDOM_H

#include "expr.h"

Expr* builtin_randominteger(Expr* res);
Expr* builtin_randomreal(Expr* res);
Expr* builtin_randomcomplex(Expr* res);
Expr* builtin_randomchoice(Expr* res);
Expr* builtin_randomsample(Expr* res);
Expr* builtin_seedrandom(Expr* res);

void random_init(void);

#endif // RANDOM_H
