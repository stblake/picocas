#ifndef PATTERNS_H
#define PATTERNS_H

#include "expr.h"

Expr* builtin_cases(Expr* res);
Expr* builtin_position(Expr* res);

void patterns_init(void);

#endif
