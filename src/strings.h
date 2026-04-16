#ifndef STRINGS_H
#define STRINGS_H

#include "expr.h"

Expr* builtin_stringlength(Expr* res);
Expr* builtin_characters(Expr* res);
Expr* builtin_stringjoin(Expr* res);

void strings_init(void);

#endif // STRINGS_H
