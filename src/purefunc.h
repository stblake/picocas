#ifndef PUREFUNC_H
#define PUREFUNC_H

#include "expr.h"
#include <stdint.h>

// Returns attributes for a pure function
uint32_t pure_function_attributes(Expr* func);

// Applies a pure function to arguments
Expr* apply_pure_function(Expr* func, Expr** args, size_t arg_count);

void purefunc_init(void);

Expr* builtin_slot(Expr* res);
Expr* builtin_slotsequence(Expr* res);

#endif // PUREFUNC_H
