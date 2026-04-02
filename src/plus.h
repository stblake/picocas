
#ifndef PLUS_H
#define PLUS_H

#include "expr.h"

Expr* builtin_plus(Expr* res);
Expr* make_plus(Expr* a, Expr* b);

#endif
