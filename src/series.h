#ifndef SERIES_H
#define SERIES_H

#include "expr.h"

/*
 * Registers SeriesData, Series, and Normal.
 *
 *   SeriesData[x, x0, {a0, a1, ...}, nmin, nmax, den]
 *     Represents a power series in x about the point x0. The coefficient
 *     ai is attached to (x - x0)^((nmin + i)/den), and an
 *     O[x - x0]^(nmax/den) term represents the dropped higher-order terms.
 *     SeriesData is a pure data head with attribute Protected.
 *
 *   Series[f, {x, x0, n}]
 *     Produces a truncated power-series expansion of f about x = x0 up to
 *     order (x - x0)^n. Handles Taylor, Laurent (negative powers), Puiseux
 *     (fractional powers), and logarithmic expansions. Attributes:
 *     HoldAll, Protected.
 *
 *   Series[f, x -> x0]
 *     Emits the leading term only (expansion at order 0).
 *
 *   Series[f, {x, x0, nx}, {y, y0, ny}, ...]
 *     Iterated multivariate expansion, right-to-left.
 *
 *   Normal[SeriesData[...]]
 *     Drops the O-term and returns the ordinary sum expression.
 */
void series_init(void);

Expr* builtin_series(Expr* res);
Expr* builtin_normal(Expr* res);

#endif // SERIES_H
