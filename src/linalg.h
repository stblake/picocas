#ifndef LINALG_H
#define LINALG_H

#include "expr.h"

Expr* builtin_dot(Expr* res);
Expr* builtin_det(Expr* res);
Expr* builtin_cross(Expr* res);
Expr* builtin_norm(Expr* res);
Expr* builtin_tr(Expr* res);
Expr* builtin_rowreduce(Expr* res);
void linalg_init(void);

#endif // LINALG_H