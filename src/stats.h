#ifndef STATS_H
#define STATS_H

#include "expr.h"

Expr* builtin_mean(Expr* res);
Expr* builtin_variance(Expr* res);
Expr* builtin_standard_deviation(Expr* res);

void stats_init(void);

#endif
