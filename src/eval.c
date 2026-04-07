/*
 * eval.c
 *
 * This file implements the core evaluation engine of PicoCAS.
 * It follows the "infinite evaluation" semantics of the Mathematica:
 * expressions are repeatedly transformed until they no longer change.
 *
 * The main entry point is evaluate(), which calls evaluate_step() in a loop.
 */

#include "eval.h"
#include "symtab.h"
#include "core.h"
#include "purefunc.h"
#include "print.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* 
 * The maximum number of evaluation steps to prevent infinite recursion
 * in cases of circular definitions. 
 */
#define MAX_ITERATIONS 4096

/*
 * eval_compare_expr_ptrs:
 * Helper for sorting expression arguments when a head has the Orderless attribute.
 * Uses the canonical expr_compare to ensure a stable, deterministic order.
 */
int eval_compare_expr_ptrs(const void* a, const void* b) {
    Expr* ea = *(Expr**)a;
    Expr* eb = *(Expr**)b;
    return expr_compare(ea, eb);
}

/*
 * flatten_args:
 * Implements the Flat (associative) attribute.
 * If a function has the same head as some of its arguments, those arguments
 * are "unwrapped" and their elements are promoted to be direct arguments 
 * of the parent function.
 * Example: f[a, f[b, c], d] -> f[a, b, c, d]
 */
void eval_flatten_args(Expr* e, const char* head_name) {
    size_t new_count = 0;
    bool needs_flattening = false;
    
    /* First pass: calculate the total number of arguments after flattening */
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        Expr* arg = e->data.function.args[i];
        if (arg->type == EXPR_FUNCTION && arg->data.function.head->type == EXPR_SYMBOL && 
            strcmp(arg->data.function.head->data.symbol, head_name) == 0) {
            new_count += arg->data.function.arg_count;
            needs_flattening = true;
        } else {
            new_count++;
        }
    }
    
    /* If no nested occurrences of the head were found, we are done */
    if (!needs_flattening) return;
    
    /* Second pass: allocate new argument array and copy elements */
    Expr** new_args = malloc(sizeof(Expr*) * new_count);
    size_t idx = 0;
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        Expr* arg = e->data.function.args[i];
        if (arg->type == EXPR_FUNCTION && arg->data.function.head->type == EXPR_SYMBOL && 
            strcmp(arg->data.function.head->data.symbol, head_name) == 0) {
            /* Splat nested arguments into the new array */
            for (size_t j = 0; j < arg->data.function.arg_count; j++) {
                new_args[idx++] = expr_copy(arg->data.function.args[j]);
            }
            /* Free the intermediate nested function node */
            expr_free(arg); 
        } else {
            new_args[idx++] = arg;
        }
    }
    
    /* Replace old argument array with the flattened one */
    free(e->data.function.args);
    e->data.function.args = new_args;
    e->data.function.arg_count = new_count;
}

/*
 * has_list_arg:
 * Helper for the Listable attribute.
 * Checks if any argument of the function is an explicit List[...].
 */
static bool has_list_arg(Expr* e) {
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        Expr* arg = e->data.function.args[i];
        if (arg->type == EXPR_FUNCTION && strcmp(arg->data.function.head->data.symbol, "List") == 0) {
            return true;
        }
    }
    return false;
}

/*
 * apply_listable:
 * Implements automatic threading of functions over lists.
 * If a function is Listable and contains list arguments, it maps itself over them.
 * Example: f[{a, b}, c] -> {f[a, c], f[b, c]}
 */
static Expr* apply_listable(Expr* e) {
    /* Determine the required length of the result list */
    size_t list_len = 0;
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        Expr* arg = e->data.function.args[i];
        if (arg->type == EXPR_FUNCTION && strcmp(arg->data.function.head->data.symbol, "List") == 0) {
            list_len = arg->data.function.arg_count;
            break;
        }
    }
    
    if (list_len == 0) return NULL;
    
    /* Construct a new List containing the threaded evaluations */
    Expr** new_list_args = malloc(sizeof(Expr*) * list_len);
    for (size_t j = 0; j < list_len; j++) {
        Expr** new_func_args = malloc(sizeof(Expr*) * e->data.function.arg_count);
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            Expr* arg = e->data.function.args[i];
            if (arg->type == EXPR_FUNCTION && strcmp(arg->data.function.head->data.symbol, "List") == 0) {
                /* All list arguments must have identical lengths */
                if (arg->data.function.arg_count != list_len) {
                    char* s = expr_to_string(e);
                    printf("Thread::tdlen: Objects of unequal length in %s cannot be combined.\n", s);
                    free(s);
                    for (size_t k = 0; k < i; k++) expr_free(new_func_args[k]);
                    free(new_func_args);
                    for (size_t k = 0; k < j; k++) expr_free(new_list_args[k]);
                    free(new_list_args);
                    return NULL;
                }
                new_func_args[i] = expr_copy(arg->data.function.args[j]);
            } else {
                /* Non-list arguments are repeated for every element */
                new_func_args[i] = expr_copy(arg);
            }
        }
        /* Recursively evaluate each threaded call */
        Expr* tmp = expr_new_function(expr_copy(e->data.function.head), new_func_args, e->data.function.arg_count);
        free(new_func_args);
        new_list_args[j] = evaluate(tmp);
        expr_free(tmp);
    }
    
    Expr* final_res = expr_new_function(expr_new_symbol("List"), new_list_args, list_len);
    free(new_list_args);
    return final_res;
}

/*
 * apply_assignment:
 * Helper to handle the 'Set' (=) and 'SetDelayed' (:=) primitives.
 * Supports recursive list destructuring.
 * Example: {x, y} = {1, 2}
 */
static bool apply_assignment(Expr* lhs, Expr* rhs, bool is_delayed) {
    if (lhs->type == EXPR_SYMBOL) {
        /* Standard symbol assignment */
        symtab_add_own_value(lhs->data.symbol, lhs, rhs);
        return true;
    } else if (lhs->type == EXPR_FUNCTION) {
        if (lhs->data.function.head->type == EXPR_SYMBOL && 
            strcmp(lhs->data.function.head->data.symbol, "List") == 0 &&
            rhs->type == EXPR_FUNCTION &&
            rhs->data.function.head->type == EXPR_SYMBOL &&
            strcmp(rhs->data.function.head->data.symbol, "List") == 0) {
            
            /* List destructuring: match lengths and recurse */
            if (lhs->data.function.arg_count != rhs->data.function.arg_count) {
                return false;
            }
            
            for (size_t i = 0; i < lhs->data.function.arg_count; i++) {
                apply_assignment(lhs->data.function.args[i], rhs->data.function.args[i], is_delayed);
            }
            return true;
        } else if (lhs->data.function.head->type == EXPR_SYMBOL && strcmp(lhs->data.function.head->data.symbol, "Part") == 0) {
            Expr* expr_part_assign(Expr* lhs, Expr* rhs); // Forward declare or include part.h
            Expr* assigned = expr_part_assign(lhs, rhs);
            if (assigned) {
                expr_free(assigned);
                return true;
            }
            return false;
        } else if (lhs->data.function.head->type == EXPR_SYMBOL) {
            /* Pattern-based assignment (DownValues) */
            /* We use the entire lhs as the pattern, and its head as the key */
            const char* symbol_name = lhs->data.function.head->data.symbol;
            if (strcmp(symbol_name, "Condition") == 0 && lhs->data.function.arg_count == 2) {
                Expr* actual_lhs = lhs->data.function.args[0];
                if (actual_lhs->type == EXPR_FUNCTION && actual_lhs->data.function.head->type == EXPR_SYMBOL) {
                    symbol_name = actual_lhs->data.function.head->data.symbol;
                } else if (actual_lhs->type == EXPR_SYMBOL) {
                    symbol_name = actual_lhs->data.symbol;
                }
            }
            symtab_add_down_value(symbol_name, lhs, rhs);
            return true;
        }
    }
    return false;
}

/*
 * flatten_sequences:
 * Flattens any Sequence[...] heads found in the arguments of e.
 */
static void flatten_sequences(Expr* e) {
    if (e->type != EXPR_FUNCTION) return;
    
    size_t new_count = 0;
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        Expr* arg = e->data.function.args[i];
        if (arg->type == EXPR_FUNCTION && arg->data.function.head->type == EXPR_SYMBOL &&
            strcmp(arg->data.function.head->data.symbol, "Sequence") == 0) {
            new_count += arg->data.function.arg_count;
        } else {
            new_count++;
        }
    }
    
    if (new_count == e->data.function.arg_count) return;
    
    Expr** new_args = malloc(sizeof(Expr*) * new_count);
    size_t k = 0;
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        Expr* arg = e->data.function.args[i];
        if (arg->type == EXPR_FUNCTION && arg->data.function.head->type == EXPR_SYMBOL &&
            strcmp(arg->data.function.head->data.symbol, "Sequence") == 0) {
            for (size_t j = 0; j < arg->data.function.arg_count; j++) {
                new_args[k++] = expr_copy(arg->data.function.args[j]);
            }
            expr_free(arg);
        } else {
            new_args[k++] = arg;
        }
    }
    
    free(e->data.function.args);
    e->data.function.args = new_args;
    e->data.function.arg_count = new_count;
}

/*
 * evaluate_step:
 * Performs exactly one level of evaluation transformation.
 */
Expr* evaluate_step(Expr* e) {
    if (!e) return NULL;

    switch (e->type) {        /* Atomics evaluate to themselves */
        case EXPR_INTEGER:
        case EXPR_REAL:
        case EXPR_STRING:
            return expr_copy(e);
            
        case EXPR_SYMBOL: {
            /* Check for immediate assignments (OwnValues) like x = 5 */
            Expr* own = apply_own_values(e);
            if (own) return own;
            return expr_copy(e);
        }
            
        case EXPR_FUNCTION: {
            /* 1. Evaluate the head recursively (e.g. f[x][y]) */
            Expr* head = evaluate(e->data.function.head);
            
            uint32_t attrs = ATTR_NONE;
            if (head->type == EXPR_SYMBOL) {
                attrs = get_attributes(head->data.symbol);
            } else if (head->type == EXPR_FUNCTION) {
                attrs = pure_function_attributes(head);
            }
            
            /* 2. Handle 'Hold' attributes */
            
            /* ATTR_HOLDALLCOMPLETE: Do not evaluate arguments or process attributes */
            if (attrs & ATTR_HOLDALLCOMPLETE) {
                Expr** new_args = malloc(sizeof(Expr*) * e->data.function.arg_count);
                for (size_t i = 0; i < e->data.function.arg_count; i++) {
                    new_args[i] = expr_copy(e->data.function.args[i]);
                }
                Expr* ret = expr_new_function(head, new_args, e->data.function.arg_count);
                free(new_args);
                return ret;
            }
            
            /* Evaluate arguments unless suppressed by HoldFirst, HoldRest, or HoldAll */
            Expr** new_args = malloc(sizeof(Expr*) * e->data.function.arg_count);
            for (size_t i = 0; i < e->data.function.arg_count; i++) {
                bool hold = false;
                if (i == 0 && (attrs & ATTR_HOLDFIRST)) hold = true;
                if (i > 0 && (attrs & ATTR_HOLDREST)) hold = true;
                if (attrs & ATTR_HOLDALL) hold = true;
                
                if (hold) {
                    new_args[i] = expr_copy(e->data.function.args[i]);
                } else {
                    new_args[i] = evaluate(e->data.function.args[i]);
                }
            }
            
            Expr* res = expr_new_function(head, new_args, e->data.function.arg_count);
            free(new_args);
            
    /* 2.5 Flatten Sequences - must happen before attributes */
    if (head->type == EXPR_SYMBOL && 
        (strcmp(head->data.symbol, "Set") == 0 || strcmp(head->data.symbol, "SetDelayed") == 0 ||
         strcmp(head->data.symbol, "Rule") == 0 || strcmp(head->data.symbol, "RuleDelayed") == 0)) {
        // Do not flatten sequences in assignments or rules
    } else {
        flatten_sequences(res);
    }

            /* 3. Apply structural and semantic attributes */
            /* Listable: automatic threading */
            if ((attrs & ATTR_LISTABLE) && has_list_arg(res)) {
                Expr* list_res = apply_listable(res);
                if (list_res) {
                    expr_free(res);
                    return list_res;
                }
            }

            if (head->type == EXPR_SYMBOL) {
                const char* head_name = head->data.symbol;
                
                /* Flat: associative flattening */
                if (attrs & ATTR_FLAT) {
                    eval_flatten_args(res, head_name);
                }
                
                /* Orderless: commutative sorting */
                if (attrs & ATTR_ORDERLESS) {
                    qsort(res->data.function.args, res->data.function.arg_count, sizeof(Expr*), eval_compare_expr_ptrs);
                }

                /* 4. Call C-level Built-in Functions */
                SymbolDef* def = symtab_get_def(head_name);
                if (def && def->builtin_func) {
                    Expr* ret = def->builtin_func(res);
                    if (ret) {
                        expr_free(res);
                        return ret;
                    }
                }
                
                /* 5. Special primitives (Set, SetDelayed) */
                if ((strcmp(head_name, "Set") == 0 || strcmp(head_name, "SetDelayed") == 0) && res->data.function.arg_count == 2) {
                    Expr* lhs = res->data.function.args[0];
                    Expr* rhs = res->data.function.args[1];
                    int is_delayed = (strcmp(head_name, "SetDelayed") == 0);
                    
                    /* For Set (not SetDelayed), we might need to evaluate the LHS to find the actual target */
                    /* e.g. f[x] = 1 where x=c should define f[c]=1 */
                    Expr* target_lhs = lhs;
                    bool free_target = false;
                    if (!is_delayed && lhs->type == EXPR_FUNCTION) {
                        /* Only evaluate arguments, not the head, to avoid matching existing rules */
                        Expr** eval_args = malloc(sizeof(Expr*) * lhs->data.function.arg_count);
                        bool is_part = (lhs->data.function.head->type == EXPR_SYMBOL && strcmp(lhs->data.function.head->data.symbol, "Part") == 0);
                        
                        for (size_t i = 0; i < lhs->data.function.arg_count; i++) {
                            if (is_part && i == 0) {
                                eval_args[i] = expr_copy(lhs->data.function.args[i]); // Hold the first argument of Part
                            } else {
                                eval_args[i] = evaluate(lhs->data.function.args[i]);
                            }
                        }
                        target_lhs = expr_new_function(expr_copy(lhs->data.function.head), eval_args, lhs->data.function.arg_count);
                        free(eval_args);
                        free_target = true;
                    }

                    if (apply_assignment(target_lhs, rhs, is_delayed)) {
                        Expr* ret = is_delayed ? expr_new_symbol("Null") : evaluate(rhs);
                        if (free_target) expr_free(target_lhs);
                        expr_free(res);
                        return ret;
                    }
                    if (free_target) expr_free(target_lhs);
                }
                
                /* 6. Apply User-defined Rules (DownValues) via Pattern Matching */
                Expr* down = apply_down_values(res);
                if (down) {
                    expr_free(res);
                    return down;
                }

                /* OneIdentity: f[x] -> x if f is OneIdentity and no other rules applied */
                if ((attrs & ATTR_ONEIDENTITY) && res->data.function.arg_count == 1) {
                    Expr* inner = expr_copy(res->data.function.args[0]);
                    expr_free(res);
                    return inner;
                }
            } else if (head->type == EXPR_FUNCTION && head->data.function.head->type == EXPR_SYMBOL &&
                       strcmp(head->data.function.head->data.symbol, "Function") == 0) {
                
                /* 7. Apply Pure Function */
                Expr* applied = apply_pure_function(head, res->data.function.args, res->data.function.arg_count);
                if (applied) {
                    expr_free(res);
                    return applied;
                }
            }
            
            return res;
        }
    }
    return expr_copy(e);
}

/*
 * evaluate:
 * The primary evaluator loop. Repeatedly applies evaluate_step until the 
 * expression reaches a fixed point (no further changes) or the iteration 
 * limit is reached.
 */
Expr* evaluate(Expr* e) {
    if (!e) return NULL;
    
    Expr* current = expr_copy(e);
    Expr* next = NULL;
    int iterations = 0;
    
    while (iterations < MAX_ITERATIONS) {
        next = evaluate_step(current);
        
        /* Fixed point check: if step produced an identical expression, we are done */
        if (expr_eq(current, next)) {
            expr_free(next);
            return current;
        }
        
        /* Prepare for the next iteration */
        expr_free(current);
        current = next;
        iterations++;
    }
    
    fprintf(stderr, "$IterationLimit exceeded\n");
    return current;
}
