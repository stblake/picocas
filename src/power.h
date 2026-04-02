
#ifndef POWER_H
#define POWER_H

#include "expr.h"

Expr* builtin_power(Expr* res);
Expr* builtin_sqrt(Expr* res);
Expr* make_power(Expr* base, Expr* exp);

#endif
