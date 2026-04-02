#ifndef COMPARISONS_H
#define COMPARISONS_H

#include "expr.h"

// Comparison routines
Expr* builtin_sameq(Expr* res);
Expr* builtin_unsameq(Expr* res);
Expr* builtin_equal(Expr* res);
Expr* builtin_unequal(Expr* res);
Expr* builtin_less(Expr* res);
Expr* builtin_greater(Expr* res);
Expr* builtin_lessequal(Expr* res);
Expr* builtin_greaterequal(Expr* res);

// Initialize comparison builtins
void comparisons_init(void);

#endif // COMPARISONS_H
