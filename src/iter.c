#include "iter.h"
#include "symtab.h"
#include "eval.h"
#include "core.h"
#include "arithmetic.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

Expr* builtin_do(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count < 2) return NULL;

    if (res->data.function.arg_count > 2) {
        Expr** inner_args = malloc(sizeof(Expr*) * 2);
        inner_args[0] = expr_copy(res->data.function.args[0]);
        inner_args[1] = expr_copy(res->data.function.args[res->data.function.arg_count - 1]);
        Expr* inner_do = expr_new_function(expr_new_symbol("Do"), inner_args, 2);
        free(inner_args);

        Expr** outer_args = malloc(sizeof(Expr*) * (res->data.function.arg_count - 1));
        outer_args[0] = inner_do;
        for (size_t i = 1; i < res->data.function.arg_count - 1; i++) {
            outer_args[i] = expr_copy(res->data.function.args[i]);
        }
        Expr* outer_do = expr_new_function(expr_new_symbol("Do"), outer_args, res->data.function.arg_count - 1);
        free(outer_args);
        
        Expr* eval_outer = evaluate(outer_do);
        expr_free(outer_do);
        return eval_outer;
    }
    
    Expr* expr = res->data.function.args[0];
    Expr* spec = res->data.function.args[1];
    
    Expr* var_sym = NULL;
    Expr* imin_e = NULL;
    Expr* imax_e = NULL;
    Expr* di_e = NULL;
    Expr* list_e = NULL;
    int is_n_times = 0;
    int is_list_iter = 0;
    double min_val = 0, max_val = 0, di_val = 0;
    bool is_real = false;
    bool is_inf = false;

    if (spec->type == EXPR_FUNCTION && spec->data.function.head->type == EXPR_SYMBOL && strcmp(spec->data.function.head->data.symbol, "List") == 0) {
        size_t len = spec->data.function.arg_count;
        if (len == 1) {
            imax_e = evaluate(spec->data.function.args[0]);
            is_n_times = 1;
        } else if (len >= 2) {
            var_sym = spec->data.function.args[0];
            if (var_sym->type != EXPR_SYMBOL) return NULL; 
            
            if (len == 2) {
                Expr* bound = evaluate(spec->data.function.args[1]);
                if (bound->type == EXPR_FUNCTION && bound->data.function.head->type == EXPR_SYMBOL && strcmp(bound->data.function.head->data.symbol, "List") == 0) {
                    list_e = bound;
                    is_list_iter = 1;
                } else {
                    imin_e = expr_new_integer(1);
                    imax_e = bound;
                    di_e = expr_new_integer(1);
                }
            } else if (len == 3) {
                imin_e = evaluate(spec->data.function.args[1]);
                imax_e = evaluate(spec->data.function.args[2]);
                di_e = expr_new_integer(1);
            } else if (len == 4) {
                imin_e = evaluate(spec->data.function.args[1]);
                imax_e = evaluate(spec->data.function.args[2]);
                di_e = evaluate(spec->data.function.args[3]);
            } else {
                return NULL;
            }
        }
    } else {
        imax_e = evaluate(spec);
        is_n_times = 1;
    }
    
    if (is_n_times) {
        if (imax_e->type == EXPR_SYMBOL && strcmp(imax_e->data.symbol, "Infinity") == 0) {
            is_inf = true;
        } else if (imax_e->type != EXPR_INTEGER) {
            expr_free(imax_e);
            return NULL;
        }
    } else if (!is_list_iter) {
        if (imax_e->type == EXPR_SYMBOL && strcmp(imax_e->data.symbol, "Infinity") == 0) {
            is_inf = true;
        }
        if (imin_e->type == EXPR_REAL || imax_e->type == EXPR_REAL || di_e->type == EXPR_REAL) is_real = true;
        
        int64_t n, d;
        if (imin_e->type == EXPR_INTEGER) min_val = (double)imin_e->data.integer;
        else if (imin_e->type == EXPR_REAL) min_val = imin_e->data.real;
        else if (is_rational(imin_e, &n, &d)) min_val = (double)n / d;
        else {
            if (imin_e) expr_free(imin_e);
            if (imax_e) expr_free(imax_e);
            if (di_e) expr_free(di_e);
            return NULL;
        }
        
        if (!is_inf) {
            if (imax_e->type == EXPR_INTEGER) max_val = (double)imax_e->data.integer;
            else if (imax_e->type == EXPR_REAL) max_val = imax_e->data.real;
            else if (is_rational(imax_e, &n, &d)) max_val = (double)n / d;
            else {
                if (imin_e) expr_free(imin_e);
                if (imax_e) expr_free(imax_e);
                if (di_e) expr_free(di_e);
                return NULL;
            }
        }
        
        if (di_e->type == EXPR_INTEGER) di_val = (double)di_e->data.integer;
        else if (di_e->type == EXPR_REAL) di_val = di_e->data.real;
        else if (is_rational(di_e, &n, &d)) di_val = (double)n / d;
        else {
            if (imin_e) expr_free(imin_e);
            if (imax_e) expr_free(imax_e);
            if (di_e) expr_free(di_e);
            return NULL;
        }
        
        if (di_val == 0) {
            if (imin_e) expr_free(imin_e);
            if (imax_e) expr_free(imax_e);
            if (di_e) expr_free(di_e);
            return NULL;
        }
    }
    
    Rule* old_own = NULL;
    if (var_sym) {
        SymbolDef* def = symtab_get_def(var_sym->data.symbol);
        old_own = def->own_values;
        def->own_values = NULL;
    }

    Expr* returned_val = NULL;
    bool finished = false;

    if (is_n_times) {
        int64_t n = is_inf ? 0 : imax_e->data.integer;
        for (int64_t i = 0; is_inf || i < n; i++) {
            Expr* eval_expr = evaluate(expr);
            if (eval_expr->type == EXPR_FUNCTION && eval_expr->data.function.head->type == EXPR_SYMBOL) {
                const char* hname = eval_expr->data.function.head->data.symbol;
                if (strcmp(hname, "Return") == 0) {
                    if (eval_expr->data.function.arg_count > 0) {
                        returned_val = expr_copy(eval_expr->data.function.args[0]);
                    } else {
                        returned_val = expr_new_symbol("Null");
                    }
                    expr_free(eval_expr);
                    finished = true;
                    break;
                } else if (strcmp(hname, "Break") == 0) {
                    expr_free(eval_expr);
                    finished = true;
                    break;
                } else if (strcmp(hname, "Continue") == 0) {
                    expr_free(eval_expr);
                    continue;
                } else if (strcmp(hname, "Throw") == 0 || strcmp(hname, "Abort") == 0 || strcmp(hname, "Quit") == 0) {
                    returned_val = eval_expr;
                    finished = true;
                    break;
                }
            }
            expr_free(eval_expr);
        }
    } else if (is_list_iter) {
        for (size_t i = 0; i < list_e->data.function.arg_count; i++) {
            symtab_add_own_value(var_sym->data.symbol, var_sym, list_e->data.function.args[i]);
            Expr* eval_expr = evaluate(expr);
            if (eval_expr->type == EXPR_FUNCTION && eval_expr->data.function.head->type == EXPR_SYMBOL) {
                const char* hname = eval_expr->data.function.head->data.symbol;
                if (strcmp(hname, "Return") == 0) {
                    if (eval_expr->data.function.arg_count > 0) {
                        returned_val = expr_copy(eval_expr->data.function.args[0]);
                    } else {
                        returned_val = expr_new_symbol("Null");
                    }
                    expr_free(eval_expr);
                    finished = true;
                    break;
                } else if (strcmp(hname, "Break") == 0) {
                    expr_free(eval_expr);
                    finished = true;
                    break;
                } else if (strcmp(hname, "Continue") == 0) {
                    expr_free(eval_expr);
                    continue;
                } else if (strcmp(hname, "Throw") == 0 || strcmp(hname, "Abort") == 0 || strcmp(hname, "Quit") == 0) {
                    returned_val = eval_expr;
                    finished = true;
                    break;
                }
            }
            expr_free(eval_expr);
        }
    } else {
        double val = min_val;
        int steps = 0;
        Expr* curr_e = expr_copy(imin_e);
        while (is_inf || (di_val > 0 && val <= max_val + 1e-14) || (di_val < 0 && val >= max_val - 1e-14)) {
            Expr* i_val = is_real ? expr_new_real(val) : expr_copy(curr_e);
            symtab_add_own_value(var_sym->data.symbol, var_sym, i_val);
            
            Expr* eval_expr = evaluate(expr);
            expr_free(i_val);
            
            if (eval_expr->type == EXPR_FUNCTION && eval_expr->data.function.head->type == EXPR_SYMBOL) {
                const char* hname = eval_expr->data.function.head->data.symbol;
                if (strcmp(hname, "Return") == 0) {
                    if (eval_expr->data.function.arg_count > 0) {
                        returned_val = expr_copy(eval_expr->data.function.args[0]);
                    } else {
                        returned_val = expr_new_symbol("Null");
                    }
                    expr_free(eval_expr);
                    finished = true;
                    break;
                } else if (strcmp(hname, "Break") == 0) {
                    expr_free(eval_expr);
                    finished = true;
                    break;
                } else if (strcmp(hname, "Continue") == 0) {
                    expr_free(eval_expr);
                    Expr* next_args[2] = { expr_copy(curr_e), expr_copy(di_e) };
                    Expr* next_expr = expr_new_function(expr_new_symbol("Plus"), next_args, 2);
                    Expr* next_e = evaluate(next_expr);
                    expr_free(next_expr);
                    expr_free(curr_e);
                    curr_e = next_e;
                    if (!is_real) {
                        int64_t n, d;
                        if (curr_e->type == EXPR_INTEGER) val = (double)curr_e->data.integer;
                        else if (curr_e->type == EXPR_REAL) val = curr_e->data.real;
                        else if (is_rational(curr_e, &n, &d)) val = (double)n / d;
                    } else {
                        val += di_val;
                    }
                    steps++;
                    continue;
                } else if (strcmp(hname, "Throw") == 0 || strcmp(hname, "Abort") == 0 || strcmp(hname, "Quit") == 0) {
                    returned_val = eval_expr;
                    finished = true;
                    break;
                }
            }
            expr_free(eval_expr);
            
            Expr* next_args[2] = { expr_copy(curr_e), expr_copy(di_e) };
            Expr* next_expr = expr_new_function(expr_new_symbol("Plus"), next_args, 2);
            Expr* next_e = evaluate(next_expr);
            expr_free(next_expr);
            expr_free(curr_e);
            curr_e = next_e;
            
            if (!is_real) {
                int64_t n, d;
                if (curr_e->type == EXPR_INTEGER) val = (double)curr_e->data.integer;
                else if (curr_e->type == EXPR_REAL) val = curr_e->data.real;
                else if (is_rational(curr_e, &n, &d)) val = (double)n / d;
            } else {
                val += di_val;
            }
            
            steps++;
            // Note: unlike Table, Do[expr, Infinity] might loop millions of times, but we don't forcefully break unless needed.
        }
        if (curr_e) expr_free(curr_e);
    }
    
    if (var_sym) {
        SymbolDef* def = symtab_get_def(var_sym->data.symbol);
        if (def->own_values) {
            expr_free(def->own_values->replacement);
            free(def->own_values);
        }
        def->own_values = old_own;
    }

    if (imax_e) expr_free(imax_e);
    if (imin_e) expr_free(imin_e);
    if (di_e) expr_free(di_e);
    if (list_e) expr_free(list_e);

    if (returned_val) return returned_val;

    return expr_new_symbol("Null");
}

void iter_init(void) {
    symtab_add_builtin("Do", builtin_do);
    symtab_get_def("Do")->attributes |= ATTR_HOLDALL | ATTR_PROTECTED;
}