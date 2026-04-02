#ifndef TRIG_H
#define TRIG_H

#include "expr.h"

Expr* builtin_sin(Expr* res);
Expr* builtin_cos(Expr* res);
Expr* builtin_tan(Expr* res);
Expr* builtin_cot(Expr* res);
Expr* builtin_sec(Expr* res);
Expr* builtin_csc(Expr* res);

Expr* builtin_arcsin(Expr* res);
Expr* builtin_arccos(Expr* res);
Expr* builtin_arctan(Expr* res);
Expr* builtin_arccot(Expr* res);
Expr* builtin_arcsec(Expr* res);
Expr* builtin_arccsc(Expr* res);

void trig_init(void);

#endif // TRIG_H