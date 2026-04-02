#ifndef MATCH_H
#define MATCH_H

#include "expr.h"
#include <stdbool.h>
#include <stddef.h>

// Environment to store pattern bindings (e.g., x -> 4)
typedef struct {
    char** symbols;
    Expr** values;
    size_t count;
    size_t capacity;
} MatchEnv;

// Create a new empty match environment
MatchEnv* env_new(void);

// Free the match environment (does not free the Expr* values, they are typically shared or cloned by the user)
void env_free(MatchEnv* env);

// Add a binding to the environment
void env_set(MatchEnv* env, const char* symbol, Expr* value);

// Get a binding from the environment, returns NULL if not found
Expr* env_get(MatchEnv* env, const char* symbol);

// Match an expression against a pattern. Returns true if match succeeds,
// and populates the env with bindings.
bool match(Expr* expr, Expr* pattern, MatchEnv* env);

// Replace bound variables in expr with their values from env.
// Returns a new expression (caller must free).
Expr* replace_bindings(Expr* expr, MatchEnv* env);

Expr* builtin_matchq(Expr* res);

#endif // MATCH_H
