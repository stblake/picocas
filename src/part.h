
#ifndef PART_H
#define PART_H

#include "expr.h"

Expr* expr_part(Expr* expr, Expr** indices, size_t nindices);
Expr* expr_part_assign(Expr* lhs, Expr* rhs);
Expr* builtin_part(Expr* res);
Expr* expr_head(Expr* e);
Expr* builtin_head(Expr* res);
Expr* builtin_first(Expr* res);
Expr* builtin_last(Expr* res);
Expr* builtin_most(Expr* res);
Expr* builtin_rest(Expr* res);
Expr* expr_insert(Expr* expr, Expr* elem, Expr* pos);
Expr* builtin_insert(Expr* res);
Expr* expr_delete(Expr* expr, Expr* pos);
Expr* builtin_delete(Expr* res);

#endif
