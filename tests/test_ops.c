#include "parse.h"
#include "print.h"
#include "expr.h"
#include <stdio.h>

int main() {
    const char* tests[] = {
        "x = 5",
        "f[x_] := x ^ 2",
        "a == b",
        "a === b",
        "x = y = 2"
    };
    for (int i=0; i<5; i++) {
        Expr* e = parse_expression(tests[i]);
        if (e) {
            printf("%s -> ", tests[i]);
            expr_print(e);
            printf("\n");
            expr_free(e);
        }
    }
    return 0;
}
