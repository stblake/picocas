#ifndef PRINT_H
#define PRINT_H

#include "expr.h"

void expr_print(Expr* e);
void expr_print_fullform(Expr* e);
char* expr_to_string(Expr* e);
char* expr_to_string_fullform(Expr* e);

Expr* builtin_print(Expr* res);
Expr* builtin_fullform(Expr* res);
Expr* builtin_inputform(Expr* res);


#endif // PRINT_H
