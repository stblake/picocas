#ifndef CORE_H
#define CORE_H

#include "expr.h"

// Core programming routines
Expr* builtin_length(Expr* res);
Expr* builtin_dimensions(Expr* res);
Expr* builtin_clear(Expr* res);
Expr* builtin_append(Expr* res);
Expr* builtin_prepend(Expr* res);
Expr* builtin_append_to(Expr* res);
Expr* builtin_prepend_to(Expr* res);
Expr* builtin_own_values(Expr* res);
Expr* builtin_down_values(Expr* res);
Expr* builtin_out(Expr* res);
Expr* builtin_compoundexpression(Expr* res);
Expr* builtin_atomq(Expr* res);
Expr* builtin_numberq(Expr* res);
Expr* builtin_numericq(Expr* res);
Expr* builtin_integerq(Expr* res);
Expr* builtin_evenq(Expr* res);
Expr* builtin_oddq(Expr* res);
Expr* builtin_mod(Expr* res);
Expr* builtin_quotient(Expr* res);
Expr* builtin_quotientremainder(Expr* res);
Expr* builtin_level(Expr* res);
Expr* builtin_depth(Expr* res);
Expr* builtin_leafcount(Expr* res);
Expr* builtin_bytecount(Expr* res);
Expr* builtin_information(Expr* res);
Expr* builtin_evaluate(Expr* res);
Expr* builtin_releasehold(Expr* res);

// Initialize core builtins
void core_init(void);

#endif // CORE_H
