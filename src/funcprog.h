
#ifndef FUNCPROG_H
#define FUNCPROG_H

#include "expr.h"

Expr* builtin_apply(Expr* res);
Expr* builtin_map(Expr* res);
Expr* builtin_map_all(Expr* res);
Expr* builtin_select(Expr* res);
Expr* builtin_through(Expr* res);
Expr* builtin_freeq(Expr* res);
Expr* builtin_distribute(Expr* res);

#endif

Expr* builtin_inner(Expr* res);
Expr* builtin_outer(Expr* res);
Expr* builtin_tuples(Expr* res);
