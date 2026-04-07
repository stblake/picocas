#include "parse.h"
#include "expr.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

// Private parser state
typedef struct {
    const char* input;  // Original input string
    const char* pos;    // Current position
} ParserState;

// Forward declarations
static Expr* parse_expression_state(ParserState* s);
static void skip_whitespace(ParserState* s);

/* ------------------- Basic Token Parsers ------------------- */

// Skips whitespace and comments
static void skip_whitespace(ParserState* s) {
    while (1) {
        if (isspace(*s->pos)) {
            s->pos++;
        } else if (s->pos[0] == '(' && s->pos[1] == '*') {
            int depth = 1;
            s->pos += 2;
            while (*s->pos != '\0' && depth > 0) {
                if (s->pos[0] == '(' && s->pos[1] == '*') {
                    depth++;
                    s->pos += 2;
                } else if (s->pos[0] == '*' && s->pos[1] == ')') {
                    depth--;
                    s->pos += 2;
                } else {
                    s->pos++;
                }
            }
        } else {
            break;
        }
    }
}

// Parses a symbol (x, `name`, $var)
static Expr* parse_symbol(ParserState* s) {
    char buffer[256];
    size_t i = 0;
    
    while (isalnum(*s->pos) || *s->pos == '`' || *s->pos == '$') {
        if (i < sizeof(buffer)-1) buffer[i++] = *s->pos;
        s->pos++;
    }
    buffer[i] = '\0';
    return expr_new_symbol(buffer);
}

// Parses numbers (integers, reals, scientific notation)
static Expr* parse_number(ParserState* s) {
    char* end;
    
    // Check if it's potentially a real number
    int is_real = 0;
    const char* p = s->pos;
    if (*p == '-' || *p == '+') p++;
    while (isdigit(*p)) p++;
    if (*p == '.' || *p == 'e' || *p == 'E') {
        is_real = 1;
    }

    if (is_real) {
        double dval = strtod(s->pos, &end);
        if (end == s->pos) return NULL;
        s->pos = end;
        return expr_new_real(dval);
    } else {
        const char* start = s->pos;
        int negative = 0;
        if (*start == '-') {
            negative = 1;
            start++;
        } else if (*start == '+') {
            start++;
        }

        uint64_t ullval = 0;
        const char* current = start;
        while (isdigit(*current)) {
            ullval = ullval * 10 + (*current - '0');
            current++;
        }
        
        if (current == start) return NULL;
        s->pos = current;

        int64_t val = (int64_t)ullval;
        if (negative) val = -val;
        
        return expr_new_integer(val);
    }
}

// Parses quoted strings ("text")
static Expr* parse_string(ParserState* s) {
    s->pos++;  // Skip opening quote
    char buffer[256];
    size_t i = 0;
    
    while (*s->pos && *s->pos != '"') {
        if (*s->pos == '\\') s->pos++;  // Handle escapes
        if (i < sizeof(buffer)-1) buffer[i++] = *s->pos;
        s->pos++;
    }
    
    if (*s->pos != '"') {
        fprintf(stderr, "Unterminated string\n");
        return NULL;
    }
    s->pos++;  // Skip closing quote
    
    buffer[i] = '\0';
    return expr_new_string(buffer);
}

/* ------------------- Compound Expressions ------------------- */

// Parses lists: {1,2,3}
static Expr* parse_list(ParserState* s) {
    s->pos++;  // Skip '{'
    Expr* elements[64];  // Practical maximum
    size_t count = 0;
    
    while (*s->pos && *s->pos != '}') {
        skip_whitespace(s);
        if (*s->pos == ',') s->pos++;
        
        Expr* elem = parse_expression_state(s);
        if (!elem) {
            while (count--) expr_free(elements[count]);
            return NULL;
        }
        elements[count++] = elem;
    }
    
    if (*s->pos != '}') {
        fprintf(stderr, "Expected '}' to close list\n");
        while (count--) expr_free(elements[count]);
        return NULL;
    }
    s->pos++;  // Skip '}'
    
    // Create List[...] expression
    Expr* list = expr_new_function(expr_new_symbol("List"), elements, count);
    return list;
}

// Parses functions: f[x,y]
static Expr* parse_function(ParserState* s, Expr* head) {
    s->pos++;  // Skip '['
    Expr* args[64];  // Practical maximum
    size_t count = 0;
    
    while (*s->pos && *s->pos != ']') {
        skip_whitespace(s);
        if (*s->pos == ',') s->pos++;
        
        Expr* arg = parse_expression_state(s);
        if (!arg) {
            while (count--) expr_free(args[count]);
            return NULL;
        }
        args[count++] = arg;
    }
    
    if (*s->pos != ']') {
        fprintf(stderr, "Expected ']' to close function\n");
        while (count--) expr_free(args[count]);
        return NULL;
    }
    s->pos++;  // Skip ']'
    
    Expr* func = expr_new_function(head, args, count);
    return func;
}

/* ------------------- Main Parser ------------------- */

static Expr* parse_expression_prec(ParserState* s, int min_prec);

typedef enum {
    OP_NONE = 0,
    OP_PLUS,
    OP_MINUS,
    OP_TIMES,
    OP_DIVIDE,
    OP_DOT,
    OP_POWER,
    OP_SET,
    OP_SETDELAYED,
    OP_EQUAL,
    OP_UNEQUAL,
    OP_LESS,
    OP_GREATER,
    OP_LESSEQUAL,
    OP_GREATEREQUAL,
    OP_SAMEQ,
    OP_UNSAMEQ,
    OP_APPLY,
    OP_APPLY1,
    OP_RULE,
    OP_RULEDELAYED,
    OP_CONDITION,
    OP_ALTERNATIVES,
    OP_MAP,
    OP_MAPALL,
    OP_REPLACEALL,
    OP_REPLACEREPEATED,
    OP_SPAN,
    OP_PART,
    OP_CALL,
    OP_AND,
    OP_OR,
    OP_COMPOUND,
    OP_FUNCTION,
    OP_POSTFIX,
    OP_PREFIX,
    OP_PATTERNTEST,
    OP_COLON,
    OP_INFORMATION,
    OP_FACTORIAL,
    OP_REPEATED,
    OP_REPEATEDNULL
} OperatorType;

typedef struct {
    OperatorType type;
    int prec;
    int right_assoc;
    const char* head_name;
    int len;
} OperatorDef;

static OperatorDef get_operator(const char* pos) {
    OperatorDef def = {OP_NONE, -1, 0, NULL, 0};
    
    if (strncmp(pos, "===", 3) == 0) {
        def.type = OP_SAMEQ; def.prec = 290; def.head_name = "SameQ"; def.len = 3;
    } else if (strncmp(pos, "=!=", 3) == 0) {
        def.type = OP_UNSAMEQ; def.prec = 290; def.head_name = "UnsameQ"; def.len = 3;
    } else if (strncmp(pos, "@@@", 3) == 0) {
        def.type = OP_APPLY1; def.prec = 620; def.right_assoc = 1; def.head_name = "Apply1"; def.len = 3;
    } else if (strncmp(pos, "//.", 3) == 0) {
        def.type = OP_REPLACEREPEATED; def.prec = 110; def.right_assoc = 0; def.head_name = "ReplaceRepeated"; def.len = 3;
    } else if (strncmp(pos, "//@", 3) == 0) {
        def.type = OP_MAPALL; def.prec = 620; def.right_assoc = 1; def.head_name = "MapAll"; def.len = 3;
    } else if (strncmp(pos, "//", 2) == 0) {
        def.type = OP_POSTFIX; def.prec = 70; def.head_name = "Postfix"; def.len = 2;
    } else if (strncmp(pos, "/.", 2) == 0) {
        def.type = OP_REPLACEALL; def.prec = 110; def.right_assoc = 0; def.head_name = "ReplaceAll"; def.len = 2;
    } else if (strncmp(pos, "@@", 2) == 0) {
        def.type = OP_APPLY; def.prec = 620; def.right_assoc = 1; def.head_name = "Apply"; def.len = 2;
    } else if (strncmp(pos, "/@", 2) == 0) {
        def.type = OP_MAP; def.prec = 620; def.right_assoc = 1; def.head_name = "Map"; def.len = 2;
    } else if (strncmp(pos, ":>", 2) == 0) {
        def.type = OP_RULEDELAYED; def.prec = 120; def.right_assoc = 1; def.head_name = "RuleDelayed"; def.len = 2;
    } else if (strncmp(pos, "->", 2) == 0) {
        def.type = OP_RULE; def.prec = 120; def.right_assoc = 1; def.head_name = "Rule"; def.len = 2;
    } else if (strncmp(pos, "/;", 2) == 0) {
        def.type = OP_CONDITION; def.prec = 130; def.right_assoc = 1; def.head_name = "Condition"; def.len = 2;
    } else if (strncmp(pos, ";;", 2) == 0) {
        def.type = OP_SPAN; def.prec = 290; def.right_assoc = 0; def.head_name = "Span"; def.len = 2;
    } else if (*pos == ';') {
        def.type = OP_COMPOUND; def.prec = 10; def.right_assoc = 1; def.head_name = "CompoundExpression"; def.len = 1;
    } else if (strncmp(pos, "||", 2) == 0) {
        def.type = OP_OR; def.prec = 215; def.head_name = "Or"; def.len = 2;
    } else if (*pos == '|') {
        def.type = OP_ALTERNATIVES; def.prec = 160; def.head_name = "Alternatives"; def.len = 1;
    } else if (strncmp(pos, "==", 2) == 0) {
        def.type = OP_EQUAL; def.prec = 290; def.head_name = "Equal"; def.len = 2;
    } else if (strncmp(pos, "&&", 2) == 0) {
        def.type = OP_AND; def.prec = 215; def.head_name = "And"; def.len = 2;
    } else if (*pos == '&') {
        def.type = OP_FUNCTION; def.prec = 90; def.head_name = "Function"; def.len = 1;
    } else if (strncmp(pos, "!=", 2) == 0) {
        def.type = OP_UNEQUAL; def.prec = 290; def.head_name = "Unequal"; def.len = 2;
    } else if (strncmp(pos, "<=", 2) == 0) {
        def.type = OP_LESSEQUAL; def.prec = 290; def.head_name = "LessEqual"; def.len = 2;
    } else if (strncmp(pos, ">=", 2) == 0) {
        def.type = OP_GREATEREQUAL; def.prec = 290; def.head_name = "GreaterEqual"; def.len = 2;
    } else if (*pos == '<') {
        def.type = OP_LESS; def.prec = 290; def.head_name = "Less"; def.len = 1;
    } else if (*pos == '>') {
        def.type = OP_GREATER; def.prec = 290; def.head_name = "Greater"; def.len = 1;
    } else if (strncmp(pos, ":=", 2) == 0) {
        def.type = OP_SETDELAYED; def.prec = 40; def.right_assoc = 1; def.head_name = "SetDelayed"; def.len = 2;
    } else if (strncmp(pos, "[[", 2) == 0) {
        def.type = OP_PART; def.prec = 1100; def.head_name = "Part"; def.len = 2;
    } else if (*pos == ':') {
        def.type = OP_COLON; def.prec = 140; def.right_assoc = 1; def.head_name = "Optional"; def.len = 1;
    } else if (*pos == '=') {
        def.type = OP_SET; def.prec = 40; def.right_assoc = 1; def.head_name = "Set"; def.len = 1;
    } else if (*pos == '+') {
        def.type = OP_PLUS; def.prec = 310; def.head_name = "Plus"; def.len = 1;
    } else if (*pos == '-') {
        def.type = OP_MINUS; def.prec = 310; def.head_name = "Plus"; def.len = 1;
    } else if (*pos == '*') {
        def.type = OP_TIMES; def.prec = 400; def.head_name = "Times"; def.len = 1;
    } else if (*pos == '/') {
        def.type = OP_DIVIDE; def.prec = 470; def.head_name = "Divide"; def.len = 1;
    } else if (strncmp(pos, "...", 3) == 0) {
        def.type = OP_REPEATEDNULL; def.prec = 170; def.head_name = "RepeatedNull"; def.len = 3;
    } else if (strncmp(pos, "..", 2) == 0) {
        def.type = OP_REPEATED; def.prec = 170; def.head_name = "Repeated"; def.len = 2;
    } else if (*pos == '.') {
        def.type = OP_DOT; def.prec = 490; def.head_name = "Dot"; def.len = 1;
    } else if (*pos == '^') {
        def.type = OP_POWER; def.prec = 590; def.right_assoc = 1; def.head_name = "Power"; def.len = 1;
    } else if (*pos == '?') {
        def.type = OP_PATTERNTEST; def.prec = 680; def.right_assoc = 0; def.head_name = "PatternTest"; def.len = 1;
    } else if (*pos == '@') {
        def.type = OP_PREFIX; def.prec = 620; def.right_assoc = 1; def.head_name = "Prefix"; def.len = 1;
    } else if (*pos == '[') {
        def.type = OP_CALL; def.prec = 1000; def.len = 1;
    } else if (*pos == '!' && pos[1] != '=') {
        def.type = OP_FACTORIAL; def.prec = 710; def.head_name = "Factorial"; def.len = 1;
    }
    
    return def;
}

// Parses primary expressions (atoms, lists, functions, parens)
static Expr* parse_primary(ParserState* s) {
    skip_whitespace(s);
    if (!*s->pos) return NULL;
    
    switch (*s->pos) {
        case '"': return parse_string(s);
        case '{': return parse_list(s);
        case '[': return parse_function(s, NULL);  // Anonymous function
        case '(': {
            s->pos++;
            Expr* inner = parse_expression_prec(s, 0);
            skip_whitespace(s);
            if (*s->pos == ')') {
                s->pos++;
            } else {
                fprintf(stderr, "Expected ')'\n");
            }
            return inner;
        }

        case '%': {
            s->pos++;
            int count = 1;
            while (*s->pos == '%') {
                count++;
                s->pos++;
            }
            if (isdigit(*s->pos)) {
                char* end;
                int64_t n = strtoll(s->pos, &end, 10);
                s->pos = end;
                Expr* out_args[1] = { expr_new_integer(n) };
                return expr_new_function(expr_new_symbol("Out"), out_args, 1);
            } else {
                Expr* out_args[1] = { expr_new_integer(-count) };
                return expr_new_function(expr_new_symbol("Out"), out_args, 1);
            }
        }
        
        case '#': {
            s->pos++;
            int count = 1;
            while (*s->pos == '#') {
                count++;
                s->pos++;
            }
            int64_t n = 1; // Default
            if (isdigit(*s->pos)) {
                char* end;
                n = strtoll(s->pos, &end, 10);
                s->pos = end;
            }
            if (count == 1) {
                Expr* args[1] = { expr_new_integer(n) };
                return expr_new_function(expr_new_symbol("Slot"), args, 1);
            } else {
                Expr* args[1] = { expr_new_integer(n) };
                return expr_new_function(expr_new_symbol("SlotSequence"), args, 1);
            }
        }
        
        default:
            if (isalpha(*s->pos) || *s->pos == '`' || *s->pos == '$' || *s->pos == '_') {
                Expr* head = NULL;
                if (*s->pos != '_') {
                    head = parse_symbol(s);
                }
                
                int underscores = 0;
                while (*s->pos == '_') {
                    underscores++;
                    s->pos++;
                }
                
                if (underscores > 0) {
                    Expr* blank = NULL;
                    Expr* blank_head = NULL;
                    if (isalpha(*s->pos) || *s->pos == '`' || *s->pos == '$') {
                        blank_head = parse_symbol(s);
                    }
                    
                    int n_args = blank_head ? 1 : 0;
                    Expr* args_b[1];
                    if (blank_head) args_b[0] = blank_head;
                    
                    if (underscores == 1) {
                        blank = expr_new_function(expr_new_symbol("Blank"), blank_head ? args_b : NULL, n_args);
                    } else if (underscores == 2) {
                        blank = expr_new_function(expr_new_symbol("BlankSequence"), blank_head ? args_b : NULL, n_args);
                    } else {
                        blank = expr_new_function(expr_new_symbol("BlankNullSequence"), blank_head ? args_b : NULL, n_args);
                    }
                    
                    if (head) {
                        Expr* args[2] = { head, blank };
                        head = expr_new_function(expr_new_symbol("Pattern"), args, 2);
                    } else {
                        head = blank;
                    }
                    
                    if (*s->pos == '.') {
                        s->pos++;
                        Expr* opt_args[1] = { head };
                        head = expr_new_function(expr_new_symbol("Optional"), opt_args, 1);
                    }
                }
                
                return head;

            }
            if (isdigit(*s->pos) || (*s->pos == '-' && isdigit(s->pos[1]))) {
                return parse_number(s);
            }
            fprintf(stderr, "Unexpected character: '%c'\n", *s->pos);
            return NULL;
    }
}

static bool can_start_primary(char c) {
    return isalpha(c) || isdigit(c) || c == '{' || c == '(' || c == '"' || c == '_' || c == '`' || c == '$' || c == '#' || c == '%';
}

// Pratt parser for operator precedence
static Expr* parse_expression_prec(ParserState* s, int min_prec) {
    skip_whitespace(s);
    if (!*s->pos) return NULL;
    
    Expr* left = NULL;
    
    if (*s->pos == '?') {
        s->pos++;
        Expr* sym = parse_symbol(s);
        if (!sym) return NULL;
        Expr* args[1] = { sym };
        left = expr_new_function(expr_new_symbol("Information"), args, 1);
        return left; // ?symbol is top-level only usually
    }

    if (strncmp(s->pos, ";;", 2) == 0) {
        left = expr_new_integer(1);
    } else if (*s->pos == '-' && !isdigit(s->pos[1])) {
        s->pos++;
        // Use a precedence higher than Plus (310) and Times (400)
        Expr* right = parse_expression_prec(s, 480);
        if (!right) return NULL;
        Expr* minus_one = expr_new_integer(-1);
        Expr* args[2] = { minus_one, right };
        left = expr_new_function(expr_new_symbol("Times"), args, 2);
    } else if (*s->pos == '!' && s->pos[1] != '=') {
        s->pos++;
        Expr* right = parse_expression_prec(s, 230); // Not precedence is 230
        if (!right) return NULL;
        Expr* args[1] = { right };
        left = expr_new_function(expr_new_symbol("Not"), args, 1);
    } else {
        left = parse_primary(s);
    }
    
    if (!left) return NULL;
    
    while (1) {
        skip_whitespace(s);
        OperatorDef op_def = get_operator(s->pos);
        
        // Handle implicit multiplication: if no explicit operator but another expression follows
        if (op_def.type == OP_NONE && can_start_primary(*s->pos)) {
            // Implicit Times has same precedence as explicit Times (400)
            if (400 < min_prec) break;
            op_def.type = OP_TIMES;
            op_def.prec = 400;
            op_def.head_name = "Times";
            op_def.len = 0; // Don't advance pos
        }

        if (op_def.type == OP_NONE || op_def.prec < min_prec) break;
        
        s->pos += op_def.len;
        
        if (op_def.type == OP_PART) {
            Expr* args[64];
            size_t count = 0;
            args[count++] = left;
            
            while (*s->pos && strncmp(s->pos, "]]", 2) != 0) {
                skip_whitespace(s);
                if (count > 1 && *s->pos == ',') s->pos++;
                
                Expr* arg = parse_expression_state(s);
                if (!arg) {
                    while (count--) expr_free(args[count]);
                    return NULL;
                }
                args[count++] = arg;
                skip_whitespace(s);
            }
            if (strncmp(s->pos, "]]", 2) != 0) {
                fprintf(stderr, "Expected ']]'\n");
                while (count--) expr_free(args[count]);
                return NULL;
            }
            s->pos += 2; // skip ']]'
            left = expr_new_function(expr_new_symbol("Part"), args, count);
            continue;
        } else if (op_def.type == OP_CALL) {
            s->pos--; // parse_function expects '[' to be there
            left = parse_function(s, left);
            if (!left) return NULL;
            continue;
        } else if (op_def.type == OP_FUNCTION) {
            Expr* args[1] = { left };
            left = expr_new_function(expr_new_symbol("Function"), args, 1);
            continue;
        } else if (op_def.type == OP_FACTORIAL) {
            Expr* args[1] = { left };
            left = expr_new_function(expr_new_symbol("Factorial"), args, 1);
            continue;
        } else if (op_def.type == OP_REPEATED) {
            Expr* args[1] = { left };
            left = expr_new_function(expr_new_symbol("Repeated"), args, 1);
            continue;
        } else if (op_def.type == OP_REPEATEDNULL) {
            Expr* args[1] = { left };
            left = expr_new_function(expr_new_symbol("RepeatedNull"), args, 1);
            continue;
        } else if (op_def.type == OP_SPAN) {
            Expr* span_args[3];
            span_args[0] = left;
            int next_prec = op_def.prec + 1; // 291
            skip_whitespace(s);
            Expr* right = NULL;
            if (*s->pos == ']' || *s->pos == ',' || *s->pos == '}' || *s->pos == '\0' || *s->pos == ';' || strncmp(s->pos, ";;", 2) == 0) {
                right = expr_new_symbol("All");
            } else {
                right = parse_expression_prec(s, next_prec);
                if (!right) right = expr_new_symbol("All");
            }
            span_args[1] = right;
            
            skip_whitespace(s);
            if (strncmp(s->pos, ";;", 2) == 0) {
                s->pos += 2;
                skip_whitespace(s);
                Expr* step = NULL;
                if (*s->pos == ']' || *s->pos == ',' || *s->pos == '}' || *s->pos == '\0' || *s->pos == ';') {
                    step = expr_new_symbol("All");
                } else {
                    step = parse_expression_prec(s, next_prec);
                    if (!step) step = expr_new_symbol("All");
                }
                span_args[2] = step;
                left = expr_new_function(expr_new_symbol("Span"), span_args, 3);
            } else {
                left = expr_new_function(expr_new_symbol("Span"), span_args, 2);
            }
            continue;
        }
        
        int next_min_prec = op_def.right_assoc ? op_def.prec : op_def.prec + 1;
        
        Expr* right = parse_expression_prec(s, next_min_prec);
        if (!right) {
            if (op_def.type == OP_COMPOUND) {
                right = expr_new_symbol("Null");
            } else {
                expr_free(left);
                return NULL;
            }
        }
        
        if (op_def.type == OP_MINUS) {
            Expr* minus_one = expr_new_integer(-1);
            Expr* args_times[2] = { minus_one, right };
            Expr* neg_right = expr_new_function(expr_new_symbol("Times"), args_times, 2);
            Expr* args_plus[2] = { left, neg_right };
            left = expr_new_function(expr_new_symbol("Plus"), args_plus, 2);
        } else if (op_def.type == OP_DIVIDE) {
            Expr* minus_one = expr_new_integer(-1);
            Expr* args_power[2] = { right, minus_one };
            Expr* inv_right = expr_new_function(expr_new_symbol("Power"), args_power, 2);
            
            if (left->type == EXPR_INTEGER && left->data.integer == 1) {
                expr_free(left);
                left = inv_right;
            } else {
                Expr* args_times[2] = { left, inv_right };
                left = expr_new_function(expr_new_symbol("Times"), args_times, 2);
            }
        } else if (op_def.type == OP_APPLY) {
            Expr* args[2] = { left, right };
            left = expr_new_function(expr_new_symbol("Apply"), args, 2);
        } else if (op_def.type == OP_APPLY1) {
            Expr* level_args[1] = { expr_new_integer(1) };
            Expr* level = expr_new_function(expr_new_symbol("List"), level_args, 1);
            Expr* args[3] = { left, right, level };
            left = expr_new_function(expr_new_symbol("Apply"), args, 3);
        } else if (op_def.type == OP_MAP) {
            Expr* args[2] = { left, right };
            left = expr_new_function(expr_new_symbol("Map"), args, 2);
        } else if (op_def.type == OP_MAPALL) {
            Expr* args[2] = { left, right };
            left = expr_new_function(expr_new_symbol("MapAll"), args, 2);
        } else if (op_def.type == OP_REPLACEALL) {
            Expr* args[2] = { left, right };
            left = expr_new_function(expr_new_symbol("ReplaceAll"), args, 2);
        } else if (op_def.type == OP_REPLACEREPEATED) {
            Expr* args[2] = { left, right };
            left = expr_new_function(expr_new_symbol("ReplaceRepeated"), args, 2);
        } else if (op_def.type == OP_ALTERNATIVES) {
            Expr* args[2] = { left, right };
            left = expr_new_function(expr_new_symbol("Alternatives"), args, 2);
        } else if (op_def.type == OP_CONDITION) {
            Expr* args[2] = { left, right };
            left = expr_new_function(expr_new_symbol("Condition"), args, 2);
        } else if (op_def.type == OP_POSTFIX) {
            Expr* args[1] = { left };
            left = expr_new_function(right, args, 1);
        } else if (op_def.type == OP_PREFIX) {
            Expr* args[1] = { right };
            left = expr_new_function(left, args, 1);
        } else if (op_def.type == OP_COLON) {
            if (left->type == EXPR_SYMBOL) {
                Expr* args[2] = { left, right };
                left = expr_new_function(expr_new_symbol("Pattern"), args, 2);
            } else {
                Expr* args[2] = { left, right };
                left = expr_new_function(expr_new_symbol("Optional"), args, 2);
            }
        } else {
            Expr* args[2] = { left, right };
            left = expr_new_function(expr_new_symbol(op_def.head_name), args, 2);
        }
    }
    
    return left;
}

static Expr* parse_expression_state(ParserState* s) {
    return parse_expression_prec(s, 0);
}

// Public interface
Expr* parse_expression(const char* input) {
    ParserState state = {input, input};
    Expr* result = parse_expression_state(&state);
    
    // Check for trailing garbage
    skip_whitespace(&state);
    if (*state.pos != '\0') {
        fprintf(stderr, "Extra characters after expression: %s\n", state.pos);
        expr_free(result);
        return NULL;
    }
    
    return result;
}

Expr* parse_next_expression(const char** input_ptr) {
    if (!input_ptr || !*input_ptr) return NULL;
    ParserState state = {*input_ptr, *input_ptr};
    skip_whitespace(&state);
    if (*state.pos == '\0') return NULL;
    
    Expr* result = parse_expression_state(&state);
    
    skip_whitespace(&state);
    *input_ptr = state.pos;
    
    return result;
}

