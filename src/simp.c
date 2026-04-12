#include "simp.h"
#include "eval.h"
#include "parse.h"
#include "symtab.h"
#include "replace.h"
#include <string.h>

static Expr* trig_to_exp_rules = NULL;

Expr* builtin_trigtoexp(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];

    Expr* replace_expr = expr_new_function(expr_new_symbol("ReplaceAll"), 
                                           (Expr*[]){expr_copy(arg), expr_copy(trig_to_exp_rules)}, 2);
    Expr* replaced = evaluate(replace_expr);
    expr_free(replace_expr);

    Expr* expand_expr = expr_new_function(expr_new_symbol("Expand"), 
                                          (Expr*[]){replaced}, 1);
    Expr* result = evaluate(expand_expr);
    expr_free(expand_expr);

    return result;
}

void simp_init(void) {
    const char* rules_str = "{ "
        "Sin[x_] :> 1/2 I E^(-I x) - 1/2 I E^(I x), "
        "Cos[x_] :> E^(-I x)/2 + E^(I x)/2, "
        "Tan[x_] :> (I (E^(-I x) - E^(I x)))/(E^(-I x) + E^(I x)), "
        "Cot[x_] :> (-I (E^(-I x) + E^(I x)))/(E^(-I x) - E^(I x)), "
        "Sec[x_] :> 2/(E^(-I x) + E^(I x)), "
        "Csc[x_] :> (2 I)/(E^(I x) - E^(-I x)), "
        "Sinh[x_] :> -(E^-x/2) + E^x/2, "
        "Cosh[x_] :> E^-x/2 + E^x/2, "
        "Tanh[x_] :> -(E^-x/(E^-x+E^x)) + E^x/(E^-x+E^x), "
        "Coth[x_] :> E^-x/(E^-x-E^x) + E^x/(E^-x-E^x), "
        "Sech[x_] :> 2/(E^-x + E^x), "
        "Csch[x_] :> 2/(-E^-x + E^x), "
        "ArcSin[x_] :> -I Log[I x + Sqrt[1 - x^2]], "
        "ArcCos[x_] :> Pi/2 + I Log[I x + Sqrt[1 - x^2]], "
        "ArcTan[x_] :> 1/2 I Log[1 - I x] - 1/2 I Log[1 + I x], "
        "ArcTan[x_, y_] :> -I Log[(x + I y)/Sqrt[x^2 + y^2]], "
        "ArcCot[x_] :> 1/2 I Log[1 - I/x] - 1/2 I Log[1 + I/x], "
        "ArcSec[x_] :> Pi/2 + I Log[I/x + Sqrt[1 - 1/x^2]], "
        "ArcCsc[x_] :> -I Log[I/x + Sqrt[1 - 1/x^2]], "
        "ArcSinh[x_] :> Log[x + Sqrt[1 + x^2]], "
        "ArcCosh[x_] :> Log[x + Sqrt[-1 + x] Sqrt[1 + x]], "
        "ArcTanh[x_] :> -(1/2) Log[1 - x] + 1/2 Log[1 + x], "
        "ArcCoth[x_] :> -(1/2) Log[1 - 1/x] + 1/2 Log[1 + 1/x], "
        "ArcSech[x_] :> Log[1/x + Sqrt[-1 + 1/x] Sqrt[1 + 1/x]], "
        "ArcCsch[x_] :> Log[1/x + Sqrt[1 + 1/x^2]] "
    "}";

    trig_to_exp_rules = parse_expression(rules_str);
    
    symtab_add_builtin("TrigToExp", builtin_trigtoexp);
    symtab_get_def("TrigToExp")->attributes |= (ATTR_LISTABLE | ATTR_PROTECTED);
}
