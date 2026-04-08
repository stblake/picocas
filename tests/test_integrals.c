#include "core.h"
#include "test_utils.h"
#include "expr.h"
#include "eval.h"
#include "parse.h"
#include "symtab.h"
#include <stdio.h>
#include <string.h>

void test_integrals() {
    symtab_init();
    core_init();
    
    Expr* load_cmd = parse_expression("Get[\"src/internal/integrals.m\"]");
    Expr* res = evaluate(load_cmd);
    
    // If running from tests/ directory, the above might fail (return $Failed).
    // Let's fallback if needed.
    if (res->type == EXPR_SYMBOL && strcmp(res->data.symbol, "$Failed") == 0) {
        expr_free(res);
        expr_free(load_cmd);
        load_cmd = parse_expression("Get[\"../src/internal/integrals.m\"]");
        res = evaluate(load_cmd);
    }
    
    expr_free(res);
    expr_free(load_cmd);
    
    printf("Formula 59\n");
    assert_eval_eq("Integrate[(1 + 2x)^3/(3 + 4x)^2, x]", "(-6 Integrate[(1 + 2 x)^3/(3 + 4 x), x] + (1 + 2 x)^4/(3 + 4 x))/2", 0);
    
    printf("Formula 60\n");
    assert_eval_eq("Integrate[1/(3 + 4x^2), x]", "ArcTan[(2 Sqrt[3] x)/3]/(2 Sqrt[3])", 0);
    
    printf("Formula 61\n");
    assert_eval_eq("Integrate[1/(3 - 4x^2), x]", "Log[(3 + 2 Sqrt[3] x)/(3 - 2 Sqrt[3] x)]/(4 Sqrt[3])", 0);
    
    printf("Formula 62\n");
    assert_eval_eq("Integrate[1/(9 + 16x^2), x]", "ArcTan[(4 x)/3]/12", 0);
    
    printf("Formula 63\n");
    assert_eval_eq("Integrate[x/(3 + 4x^2), x]", "Log[3 + 4 x^2]/8", 0);
    
    printf("Formula 64\n");
    assert_eval_eq("Integrate[x^2/(3 + 4x^2), x]", "-(3 ArcTan[(2 Sqrt[3] x)/3]/Sqrt[3])/8 + x/4", 0);
    printf("Done!\n");
}

int main() {
    TEST(test_integrals);
    return 0;
}
