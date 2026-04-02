#ifndef EXPAND_H
#define EXPAND_H

#include "expr.h"

Expr* expr_expand_patt(Expr* e, Expr* patt);
Expr* expr_expand(Expr* e);
Expr* builtin_expand(Expr* res);
void expand_init(void);

#endif // EXPAND_H
