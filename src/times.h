
#ifndef TIMES_H
#define TIMES_H

#include "expr.h"

Expr* builtin_times(Expr* res);
Expr* make_times(Expr* a, Expr* b);

#endif
