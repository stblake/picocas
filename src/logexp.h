#ifndef LOGEXP_H
#define LOGEXP_H

#include "expr.h"

Expr* builtin_log(Expr* res);
Expr* builtin_exp(Expr* res);

void logexp_init(void);

#endif // LOGEXP_H
