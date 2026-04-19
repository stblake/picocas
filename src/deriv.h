/*
 * deriv.h -- Symbolic differentiation (D, Dt, Derivative)
 *
 * Provides a native C implementation of Mathematica-style differentiation,
 * replacing the earlier rule-based src/internal/deriv.m bootstrap. All
 * elementary differentiation identities (sums, products, powers, exp/log,
 * trig, hyperbolic, inverse trig/hyperbolic, chain rule on unknown
 * functions) are encoded directly in C so that differentiation does not
 * depend on the pattern matcher for its core cases. See deriv.c for the
 * full implementation and a discussion of the evaluation strategy.
 */
#ifndef PICOCAS_DERIV_H
#define PICOCAS_DERIV_H

#include "expr.h"

/* Register D, Dt and Derivative with the symbol table. Safe to call
 * multiple times; idempotent with respect to attributes. */
void deriv_init(void);

/* Public built-ins. The semantics follow Mathematica:
 *   D[f, x]              -- partial derivative of f with respect to x.
 *   D[f, {x, n}]         -- n-th partial derivative (n >= 0).
 *   D[f, x, y, ...]      -- mixed derivative (chained).
 *   Dt[f]                -- total derivative of f (symbols are dependent
 *                            unless they are numeric or recognized constants).
 *   Dt[f, x]             -- equivalent to D[f, x] (partial form).
 *   Dt[f, {x, n}]        -- equivalent to D[f, {x, n}].
 *   Dt[f, x, y, ...]     -- mixed total derivative.
 *   Derivative[...][f]   -- the higher-order derivative operator. The
 *                            built-in below only provides attribute hooks
 *                            for the head symbol Derivative; the composite
 *                            forms Derivative[n][f][x] are simplified
 *                            implicitly by D itself when they appear inside
 *                            a larger expression.
 */
Expr* builtin_d(Expr* res);
Expr* builtin_dt(Expr* res);
Expr* builtin_derivative(Expr* res);

#endif /* PICOCAS_DERIV_H */
