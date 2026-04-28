#ifndef EXPAND_H
#define EXPAND_H

#include "expr.h"

Expr* expr_expand_patt(Expr* e, Expr* patt);
Expr* expr_expand(Expr* e);
Expr* expr_expand_numerator(Expr* e);
Expr* expr_expand_denominator(Expr* e);
Expr* builtin_expand(Expr* res);
Expr* builtin_expand_numerator(Expr* res);
Expr* builtin_expand_denominator(Expr* res);
void expand_init(void);

#endif // EXPAND_H
