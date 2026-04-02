#ifndef REPLACE_H
#define REPLACE_H

#include "expr.h"

Expr* builtin_replace_part(Expr* res);
Expr* builtin_replace(Expr* res);
Expr* builtin_replace_all(Expr* res);
Expr* builtin_replace_repeated(Expr* res);
void replace_init(void);

#endif
