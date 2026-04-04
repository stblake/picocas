#ifndef RAT_H
#define RAT_H

#include "expr.h"

Expr* builtin_numerator(Expr* res);
Expr* builtin_denominator(Expr* res);
Expr* builtin_cancel(Expr* res);
Expr* builtin_together(Expr* res);
Expr* builtin_leafcount(Expr* res);
void rat_init(void);

#endif // RAT_H
