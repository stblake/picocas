#include "purefunc.h"
#include "symtab.h"
#include "eval.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "attr.h"

static uint32_t parse_pure_attr(Expr* attr_expr) {
    if (attr_expr->type == EXPR_SYMBOL) {
        const char* name = attr_expr->data.symbol;
        if (strcmp(name, "Listable") == 0) return ATTR_LISTABLE;
        if (strcmp(name, "Flat") == 0) return ATTR_FLAT;
        if (strcmp(name, "Orderless") == 0) return ATTR_ORDERLESS;
        if (strcmp(name, "NumericFunction") == 0) return ATTR_NUMERICFUNCTION;
        if (strcmp(name, "OneIdentity") == 0) return ATTR_ONEIDENTITY;
        if (strcmp(name, "HoldAll") == 0) return ATTR_HOLDALL;
    }
    return ATTR_NONE;
}

uint32_t pure_function_attributes(Expr* func) {
    if (func->type == EXPR_FUNCTION && func->data.function.head->type == EXPR_SYMBOL &&
        strcmp(func->data.function.head->data.symbol, "Function") == 0) {
        
        uint32_t attrs = ATTR_HOLDALL;
        if (func->data.function.arg_count >= 3) {
            Expr* attr_spec = func->data.function.args[2];
            if (attr_spec->type == EXPR_SYMBOL) {
                attrs |= parse_pure_attr(attr_spec);
            } else if (attr_spec->type == EXPR_FUNCTION && strcmp(attr_spec->data.function.head->data.symbol, "List") == 0) {
                for (size_t i = 0; i < attr_spec->data.function.arg_count; i++) {
                    attrs |= parse_pure_attr(attr_spec->data.function.args[i]);
                }
            }
        }
        return attrs;
    }
    return ATTR_NONE;
}

void purefunc_init(void) {
    symtab_add_builtin("Slot", builtin_slot);
    symtab_add_builtin("SlotSequence", builtin_slotsequence);
    symtab_get_def("Slot")->attributes |= ATTR_PROTECTED;
    symtab_get_def("SlotSequence")->attributes |= ATTR_PROTECTED;
}

Expr* builtin_slot(Expr* res) {
    (void)res;
    // Slot stays unevaluated. 
    // It's only replaced during apply_pure_function.
    return NULL;
}

Expr* builtin_slotsequence(Expr* res) {
    (void)res;
    // SlotSequence stays unevaluated.
    return NULL;
}

static Expr* substitute_slots(Expr* e, Expr** args, size_t arg_count) {
    if (!e) return NULL;
    
    if (e->type == EXPR_FUNCTION) {
        // If it's another Function, don't recurse into it. 
        // We only substitute slots for the current level.
        if (e->data.function.head->type == EXPR_SYMBOL && 
            strcmp(e->data.function.head->data.symbol, "Function") == 0) {
            return expr_copy(e);
        }
        
        // Check if this function is Slot[n] or SlotSequence[n]
        if (e->data.function.head->type == EXPR_SYMBOL) {
            const char* head = e->data.function.head->data.symbol;
            
            if (strcmp(head, "Slot") == 0 && e->data.function.arg_count == 1) {
                Expr* arg = e->data.function.args[0];
                if (arg->type == EXPR_INTEGER) {
                    int64_t idx = arg->data.integer;
                    if (idx >= 1 && (size_t)idx <= arg_count) {
                        return expr_copy(args[idx - 1]);
                    }
                }
            }
            
            if (strcmp(head, "SlotSequence") == 0 && e->data.function.arg_count == 1) {
                Expr* arg = e->data.function.args[0];
                if (arg->type == EXPR_INTEGER) {
                    int64_t idx = arg->data.integer;
                    if (idx >= 1 && (size_t)idx <= arg_count) {
                        size_t seq_count = arg_count - (size_t)idx + 1;
                        Expr** seq_args = malloc(sizeof(Expr*) * seq_count);
                        for (size_t i = 0; i < seq_count; i++) {
                            seq_args[i] = expr_copy(args[(size_t)idx - 1 + i]);
                        }
                        Expr* seq = expr_new_function(expr_new_symbol("Sequence"), seq_args, seq_count);
                        free(seq_args);
                        return seq;
                    }
                }
            }
        }
        
        // Standard recursion for other functions
        Expr** new_args = malloc(sizeof(Expr*) * e->data.function.arg_count);
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            new_args[i] = substitute_slots(e->data.function.args[i], args, arg_count);
        }
        Expr* new_head = substitute_slots(e->data.function.head, args, arg_count);
        Expr* res = expr_new_function(new_head, new_args, e->data.function.arg_count);
        free(new_args);
        return res;
    }
    
    return expr_copy(e);
}

Expr* apply_pure_function(Expr* head, Expr** args, size_t arg_count) {
    if (head->type != EXPR_FUNCTION) return NULL;
    
    // Function[body]
    if (head->data.function.arg_count == 1) {
        Expr* body = head->data.function.args[0];
        Expr* substituted = substitute_slots(body, args, arg_count);
        Expr* result = evaluate(substituted);
        expr_free(substituted);
        return result;
    }
    
    // Function[{x, y, ...}, body]
    if (head->data.function.arg_count >= 2) {
        Expr* vars = head->data.function.args[0];
        Expr* body = head->data.function.args[1];
        
        if (vars->type == EXPR_FUNCTION && strcmp(vars->data.function.head->data.symbol, "List") == 0) {
            size_t var_count = vars->data.function.arg_count;
            
            // Temporary bind variables
            Rule* old_rules = malloc(sizeof(Rule) * var_count);
            for (size_t i = 0; i < var_count; i++) {
                Expr* var = vars->data.function.args[i];
                if (var->type == EXPR_SYMBOL) {
                    SymbolDef* def = symtab_get_def(var->data.symbol);
                    old_rules[i].next = def->own_values;
                    def->own_values = NULL; // Simple shadowing
                    if (i < arg_count) {
                        symtab_add_own_value(var->data.symbol, var, args[i]);
                    }
                }
            }
            
            Expr* result = evaluate(body);
            
            // Restore variables
            for (size_t i = 0; i < var_count; i++) {
                Expr* var = vars->data.function.args[i];
                if (var->type == EXPR_SYMBOL) {
                    SymbolDef* def = symtab_get_def(var->data.symbol);
                    // Free the temporary own value
                    if (def->own_values) {
                        Rule* r = def->own_values;
                        expr_free(r->pattern);
                        expr_free(r->replacement);
                        free(r);
                    }
                    def->own_values = old_rules[i].next;
                }
            }
            free(old_rules);
            return result;
        } else if (vars->type == EXPR_SYMBOL) {
            // Function[x, body]
            SymbolDef* def = symtab_get_def(vars->data.symbol);
            Rule* old_own = def->own_values;
            def->own_values = NULL;
            if (arg_count >= 1) {
                symtab_add_own_value(vars->data.symbol, vars, args[0]);
            }
            
            Expr* result = evaluate(body);
            
            if (def->own_values) {
                Rule* r = def->own_values;
                expr_free(r->pattern);
                expr_free(r->replacement);
                free(r);
            }
            def->attributes &= ~ATTR_HOLDALL; // should not be here but for safety
            def->own_values = old_own;
            return result;
        }
    }
    
    return NULL;
}
