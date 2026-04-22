#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// Print

#include "print.h"
#include "expr.h"
#include "arithmetic.h"
#include "context.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <inttypes.h>

static void print_standard(Expr* e, int parent_prec);
static void print_series_data(Expr* e, int parent_prec);

/*
 * When non-zero, print_standard renders heads like SeriesData using their
 * literal head[args...] form rather than the pretty mathematical form.
 * Toggled while printing inside an InputForm[...] wrapper.
 */
static int g_inputform_depth = 0;

static int get_expr_prec(Expr* e) {
    if (e->type != EXPR_FUNCTION) return 1000;
    if (e->data.function.head->type != EXPR_SYMBOL) return 1000;
    
    const char* head = e->data.function.head->data.symbol;

    if (strcmp(head, "Set") == 0 || strcmp(head, "SetDelayed") == 0) return 40;
    if (strcmp(head, "Rule") == 0 || strcmp(head, "RuleDelayed") == 0) return 120;
    if (strcmp(head, "Condition") == 0) return 130;
    if (strcmp(head, "Alternatives") == 0) return 160;
    if (strcmp(head, "Repeated") == 0 || strcmp(head, "RepeatedNull") == 0) return 170;
    if (strcmp(head, "And") == 0) return 215;
    if (strcmp(head, "Or") == 0) return 215;
    if (strcmp(head, "Equal") == 0 || strcmp(head, "Unequal") == 0 || strcmp(head, "Less") == 0 || strcmp(head, "Greater") == 0 || strcmp(head, "LessEqual") == 0 || strcmp(head, "GreaterEqual") == 0 || strcmp(head, "SameQ") == 0 || strcmp(head, "UnsameQ") == 0) return 290;
    if (strcmp(head, "Plus") == 0) return 310;

    if (strcmp(head, "Times") == 0) return 400;
    if (strcmp(head, "Divide") == 0) return 470;
    if (strcmp(head, "Power") == 0) {
        if (e->data.function.arg_count == 2) {
            Expr* exp = e->data.function.args[1];
            if (exp->type == EXPR_INTEGER && exp->data.integer < 0) return 470;
            int64_t n, d;
            if (is_rational(exp, &n, &d) && n < 0) return 470;
            if (exp->type == EXPR_REAL && exp->data.real < 0.0) return 470;
        }
        return 590;
    }
    if (strcmp(head, "Rational") == 0) return 470;
    if (strcmp(head, "Complex") == 0) return 310;
    return 1000;
}

static void print_function_fullform(Expr* e) {
    expr_print_fullform(e->data.function.head);
    printf("[");
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        if (i > 0) printf(", ");
        expr_print_fullform(e->data.function.args[i]);
    }    
    printf("]");
}

void expr_print_fullform(Expr* e) {
    if (!e) { printf("Null"); return; }
    switch (e->type) {
        case EXPR_INTEGER: printf("%" PRId64, e->data.integer); break;
        case EXPR_REAL: {
            char buf[64];
            snprintf(buf, sizeof(buf), "%g", e->data.real);
            if (strchr(buf, '.') == NULL && strchr(buf, 'e') == NULL && strchr(buf, 'E') == NULL) printf("%s.0", buf);
            else printf("%s", buf);
            break;
        }
        case EXPR_SYMBOL: printf("%s", context_display_name(e->data.symbol)); break;
        case EXPR_STRING: printf("\"%s\"", e->data.string); break;
        case EXPR_FUNCTION: print_function_fullform(e); break;
        case EXPR_BIGINT: {
            char* str = mpz_get_str(NULL, 10, e->data.bigint);
            printf("%s", str);
            free(str);
            break;
        }
#ifdef USE_MPFR
        case EXPR_MPFR: {
            /* Print with decimal-digit precision matching the value's
             * stored binary precision. `%.*Rg` uses the `g` style (drops
             * trailing zeros) so most common values print cleanly; we
             * append ".0" for integer-valued outputs for consistency
             * with EXPR_REAL formatting (e.g. 3. → 3.0). */
            long digits = (long)ceil((double)mpfr_get_prec(e->data.mpfr)
                                     / 3.3219280948873626);
            if (digits < 1) digits = 1;
            char* buf = NULL;
            int len = mpfr_asprintf(&buf, "%.*Rg", (int)digits, e->data.mpfr);
            if (len >= 0 && buf) {
                printf("%s", buf);
                if (strchr(buf, '.') == NULL && strchr(buf, 'e') == NULL
                    && strchr(buf, 'E') == NULL && strchr(buf, 'n') == NULL
                    && strchr(buf, 'i') == NULL /* nan / inf */) {
                    printf(".0");
                }
                mpfr_free_str(buf);
            } else {
                printf("<mpfr print error>");
            }
            break;
        }
#endif
    }
}

static void print_standard(Expr* e, int parent_prec) {
    if (!e) { printf("Null"); return; }
    if (e->type != EXPR_FUNCTION) {
        expr_print_fullform(e);
        return;
    }

    int my_prec = get_expr_prec(e);
    bool need_parens = (my_prec < parent_prec);

    if (need_parens) printf("(");

    if (e->data.function.head->type == EXPR_SYMBOL) {
        const char* head = e->data.function.head->data.symbol;
        
        if (strcmp(head, "FullForm") == 0 && e->data.function.arg_count == 1) {
            expr_print_fullform(e->data.function.args[0]);
        }
        else if (strcmp(head, "InputForm") == 0 && e->data.function.arg_count == 1) {
            g_inputform_depth++;
            print_standard(e->data.function.args[0], parent_prec);
            g_inputform_depth--;
        }
        else if (strcmp(head, "SeriesData") == 0 && e->data.function.arg_count == 6 && g_inputform_depth == 0) {
            print_series_data(e, parent_prec);
        }
        else if (strcmp(head, "HoldForm") == 0 && e->data.function.arg_count == 1) {
            print_standard(e->data.function.args[0], parent_prec);
        }
        else if (strcmp(head, "Rational") == 0 && e->data.function.arg_count == 2) {
            print_standard(e->data.function.args[0], 470);
            printf("/");
            print_standard(e->data.function.args[1], 470);
        }
        else if (strcmp(head, "Complex") == 0 && e->data.function.arg_count == 2) {
            Expr* re = e->data.function.args[0];
            Expr* im = e->data.function.args[1];
            bool re_zero = (re->type == EXPR_INTEGER && re->data.integer == 0);
            bool im_one = (im->type == EXPR_INTEGER && im->data.integer == 1);
            bool im_minus_one = (im->type == EXPR_INTEGER && im->data.integer == -1);

            bool im_neg = im_minus_one;
            Expr* im_abs = NULL;
            int64_t rn, rd;
            if (!im_neg) {
                if (im->type == EXPR_INTEGER && im->data.integer < 0) {
                    im_neg = true;
                    im_abs = expr_new_integer(-im->data.integer);
                } else if (im->type == EXPR_REAL && im->data.real < 0.0) {
                    im_neg = true;
                    im_abs = expr_new_real(-im->data.real);
                } else if (is_rational(im, &rn, &rd) && rn < 0) {
                    im_neg = true;
                    im_abs = make_rational(-rn, rd);
                }
            }

            if (!re_zero) {
                print_standard(re, 310);
                printf(im_neg ? " - " : " + ");
            } else if (im_neg) {
                printf("-");
            }
            if (im_one || im_minus_one) printf("I");
            else if (im_abs) { print_standard(im_abs, 400); printf("*I"); expr_free(im_abs); }
            else { print_standard(im, 400); printf("*I"); }
        }
        else if (strcmp(head, "Power") == 0 && e->data.function.arg_count == 2) {
            Expr* exp = e->data.function.args[1];
            int64_t n, d;
            bool is_negative = false;
            Expr* pos_exp = NULL;
            if (exp->type == EXPR_INTEGER && exp->data.integer < 0) {
                is_negative = true;
                pos_exp = expr_new_integer(-exp->data.integer);
            } else if (is_rational(exp, &n, &d) && n < 0) {
                is_negative = true;
                pos_exp = make_rational(-n, d);
            } else if (exp->type == EXPR_REAL && exp->data.real < 0.0) {
                is_negative = true;
                pos_exp = expr_new_real(-exp->data.real);
            }

            if (is_negative) {
                printf("1/");
                if (pos_exp->type == EXPR_INTEGER && pos_exp->data.integer == 1) {
                    print_standard(e->data.function.args[0], 470);
                } else {
                    Expr* args[2] = { expr_copy(e->data.function.args[0]), expr_copy(pos_exp) };
                    Expr* tmp = expr_new_function(expr_new_symbol("Power"), args, 2);
                    print_standard(tmp, 470);
                    expr_free(tmp);
                }
                if (pos_exp) expr_free(pos_exp);
            } else if (is_rational(exp, &n, &d) && n == 1 && d == 2) {
                printf("Sqrt[");
                print_standard(e->data.function.args[0], 0);
                printf("]");
            } else {
                print_standard(e->data.function.args[0], 590);
                printf("^");
                print_standard(e->data.function.args[1], 590);
            }
        }
        else if (strcmp(head, "Times") == 0) {
            size_t count = e->data.function.arg_count;

            /* Pull a leading negative numeric factor out as unary minus so
             * Times[-1, Infinity] prints "-Infinity", Times[-1, a, b] prints
             * "-a b", and Times[-3, x] prints "-3 x" (we keep the magnitude
             * as a factor when it isn't 1). Without this the numerator loop
             * below would emit "-1 Infinity". */
            int64_t lead_start = 0;
            bool flipped_sign = false;
            Expr* flipped_head = NULL; /* only used when lead arg != -1 */
            if (count > 0) {
                Expr* a0 = e->data.function.args[0];
                if (a0->type == EXPR_INTEGER && a0->data.integer < 0) {
                    flipped_sign = true;
                    if (a0->data.integer == -1 && count > 1) {
                        lead_start = 1; /* drop the -1, emit only "-" */
                    } else {
                        flipped_head = expr_new_integer(-a0->data.integer);
                    }
                } else if (a0->type == EXPR_REAL && a0->data.real < 0.0) {
                    flipped_sign = true;
                    if (a0->data.real == -1.0 && count > 1) {
                        lead_start = 1;
                    } else {
                        flipped_head = expr_new_real(-a0->data.real);
                    }
                } else if (a0->type == EXPR_FUNCTION &&
                           a0->data.function.head->type == EXPR_SYMBOL &&
                           strcmp(a0->data.function.head->data.symbol, "Rational") == 0 &&
                           a0->data.function.arg_count == 2 &&
                           a0->data.function.args[0]->type == EXPR_INTEGER &&
                           a0->data.function.args[0]->data.integer < 0) {
                    flipped_sign = true;
                    Expr* num = a0->data.function.args[0];
                    Expr* den = a0->data.function.args[1];
                    Expr* rargs[2] = { expr_new_integer(-num->data.integer), expr_copy(den) };
                    flipped_head = expr_new_function(expr_new_symbol("Rational"), rargs, 2);
                }
            }
            if (flipped_sign) printf("-");

            Expr** num_args = malloc(sizeof(Expr*) * count);
            Expr** den_args = malloc(sizeof(Expr*) * count);
            size_t num_count = 0, den_count = 0;

            for (size_t i = lead_start; i < count; i++) {
                Expr* arg = (i == 0 && flipped_head) ? flipped_head : e->data.function.args[i];
                if (arg->type == EXPR_FUNCTION && arg->data.function.head->type == EXPR_SYMBOL && strcmp(arg->data.function.head->data.symbol, "Power") == 0 && arg->data.function.arg_count == 2) {
                    Expr* exp = arg->data.function.args[1];
                    bool is_neg = false;
                    int64_t n, d;
                    Expr* pos_exp = NULL;
                    if (exp->type == EXPR_INTEGER && exp->data.integer < 0) {
                        is_neg = true;
                        pos_exp = expr_new_integer(-exp->data.integer);
                    } else if (is_rational(exp, &n, &d) && n < 0) {
                        is_neg = true;
                        pos_exp = make_rational(-n, d);
                    } else if (exp->type == EXPR_REAL && exp->data.real < 0.0) {
                        is_neg = true;
                        pos_exp = expr_new_real(-exp->data.real);
                    }
                    if (is_neg) {
                        if (pos_exp->type == EXPR_INTEGER && pos_exp->data.integer == 1) {
                            den_args[den_count++] = expr_copy(arg->data.function.args[0]);
                        } else {
                            Expr* p_args[2] = { expr_copy(arg->data.function.args[0]), pos_exp };
                            den_args[den_count++] = expr_new_function(expr_new_symbol("Power"), p_args, 2);
                            pos_exp = NULL; 
                        }
                        if (pos_exp) expr_free(pos_exp);
                    } else {
                        num_args[num_count++] = expr_copy(arg);
                    }
                } else {
                    num_args[num_count++] = expr_copy(arg);
                }
            }
            
            if (den_count == 0) {
                for (size_t i = 0; i < num_count; i++) {
                    if (i > 0) printf(" ");
                    print_standard(num_args[i], 400);
                }
            } else {
                Expr* num = NULL;
                if (num_count == 0) num = expr_new_integer(1);
                else if (num_count == 1) num = expr_copy(num_args[0]);
                else {
                    Expr** nc = malloc(sizeof(Expr*) * num_count);
                    for(size_t i=0; i<num_count; i++) nc[i] = expr_copy(num_args[i]);
                    num = expr_new_function(expr_new_symbol("Times"), nc, num_count);
                    free(nc);
                }
                
                Expr* den = NULL;
                if (den_count == 1) den = expr_copy(den_args[0]);
                else {
                    Expr** dc = malloc(sizeof(Expr*) * den_count);
                    for(size_t i=0; i<den_count; i++) dc[i] = expr_copy(den_args[i]);
                    den = expr_new_function(expr_new_symbol("Times"), dc, den_count);
                    free(dc);
                }
                
                print_standard(num, 470);
                printf("/");
                print_standard(den, 470);
                
                expr_free(num);
                expr_free(den);
            }
            
            for (size_t i = 0; i < num_count; i++) expr_free(num_args[i]);
            free(num_args);
            for (size_t i = 0; i < den_count; i++) expr_free(den_args[i]);
            free(den_args);
            if (flipped_head) expr_free(flipped_head);
        }

        else if ((strcmp(head, "Equal") == 0 || strcmp(head, "Unequal") == 0 || strcmp(head, "Less") == 0 || strcmp(head, "Greater") == 0 || strcmp(head, "LessEqual") == 0 || strcmp(head, "GreaterEqual") == 0 || strcmp(head, "SameQ") == 0 || strcmp(head, "UnsameQ") == 0 || strcmp(head, "Set") == 0 || strcmp(head, "SetDelayed") == 0 || strcmp(head, "Rule") == 0 || strcmp(head, "RuleDelayed") == 0 || strcmp(head, "Condition") == 0 || strcmp(head, "And") == 0 || strcmp(head, "Or") == 0 || strcmp(head, "Alternatives") == 0) && e->data.function.arg_count >= 2) {
            const char* op = "";
            if (strcmp(head, "Equal") == 0) op = " == ";
            else if (strcmp(head, "Unequal") == 0) op = " != ";
            else if (strcmp(head, "Less") == 0) op = " < ";
            else if (strcmp(head, "Greater") == 0) op = " > ";
            else if (strcmp(head, "LessEqual") == 0) op = " <= ";
            else if (strcmp(head, "GreaterEqual") == 0) op = " >= ";
            else if (strcmp(head, "SameQ") == 0) op = " === ";
            else if (strcmp(head, "UnsameQ") == 0) op = " =!= ";
            else if (strcmp(head, "Set") == 0) op = " = ";
            else if (strcmp(head, "SetDelayed") == 0) op = " := ";
            else if (strcmp(head, "Rule") == 0) op = " -> ";
            else if (strcmp(head, "RuleDelayed") == 0) op = " :> ";
            else if (strcmp(head, "Condition") == 0) op = " /; ";
            else if (strcmp(head, "And") == 0) op = " && ";
            else if (strcmp(head, "Or") == 0) op = " || ";
            else if (strcmp(head, "Alternatives") == 0) op = " | ";

            for (size_t i = 0; i < e->data.function.arg_count; i++) {
                if (i > 0) printf("%s", op);
                print_standard(e->data.function.args[i], my_prec);
            }
        }
        else if (strcmp(head, "Plus") == 0) {
            for (size_t i = 0; i < e->data.function.arg_count; i++) {
                Expr* arg = e->data.function.args[i];
                bool is_negative = false;
                Expr* to_print = arg;
                Expr* t_copy = NULL;

                if (arg->type == EXPR_INTEGER && arg->data.integer < 0) {
                    is_negative = true;
                    t_copy = expr_new_integer(-arg->data.integer);
                    to_print = t_copy;
                } else if (arg->type == EXPR_REAL && arg->data.real < 0) {
                    is_negative = true;
                    t_copy = expr_new_real(-arg->data.real);
                    to_print = t_copy;
                } else if (arg->type == EXPR_FUNCTION && arg->data.function.head->type == EXPR_SYMBOL) {
                    const char* h = arg->data.function.head->data.symbol;
                    if (strcmp(h, "Times") == 0 && arg->data.function.arg_count > 0) {
                        Expr* f_arg = arg->data.function.args[0];
                        if (f_arg->type == EXPR_INTEGER && f_arg->data.integer < 0) {
                            is_negative = true;
                            t_copy = expr_copy(arg);
                            t_copy->data.function.args[0]->data.integer = -f_arg->data.integer;
                            
                            if (t_copy->data.function.args[0]->data.integer == 1 && t_copy->data.function.arg_count > 1) {
                                Expr** new_args = malloc(sizeof(Expr*) * (t_copy->data.function.arg_count - 1));
                                for (size_t k = 1; k < t_copy->data.function.arg_count; k++) {
                                    new_args[k-1] = expr_copy(t_copy->data.function.args[k]);
                                }
                                Expr* new_times = expr_new_function(expr_new_symbol("Times"), new_args, t_copy->data.function.arg_count - 1);
                                expr_free(t_copy);
                                t_copy = new_times;
                                free(new_args);
                            }
                            to_print = t_copy;
                        } else if (f_arg->type == EXPR_REAL && f_arg->data.real < 0) {
                            is_negative = true;
                            t_copy = expr_copy(arg);
                            t_copy->data.function.args[0]->data.real = -f_arg->data.real;
                            to_print = t_copy;
                        } else if (f_arg->type == EXPR_FUNCTION && f_arg->data.function.head->type == EXPR_SYMBOL && strcmp(f_arg->data.function.head->data.symbol, "Rational") == 0) {
                            Expr* num = f_arg->data.function.args[0];
                            if (num->type == EXPR_INTEGER && num->data.integer < 0) {
                                is_negative = true;
                                t_copy = expr_copy(arg);
                                t_copy->data.function.args[0]->data.function.args[0]->data.integer = -num->data.integer;
                                to_print = t_copy;
                            }
                        }
                    } else if (strcmp(h, "Rational") == 0 && arg->data.function.arg_count == 2) {
                        Expr* num = arg->data.function.args[0];
                        if (num->type == EXPR_INTEGER && num->data.integer < 0) {
                            is_negative = true;
                            t_copy = expr_copy(arg);
                            t_copy->data.function.args[0]->data.integer = -num->data.integer;
                            to_print = t_copy;
                        }
                    }
                }

                if (i > 0) {
                    if (is_negative) {
                        printf(" - ");
                    } else {
                        printf(" + ");
                    }
                } else {
                    if (is_negative) {
                        printf("-");
                    }
                }

                print_standard(to_print, 310);
                if (t_copy) expr_free(t_copy);
            }
        }
        else if (strcmp(head, "List") == 0) {
            printf("{");
            for (size_t i = 0; i < e->data.function.arg_count; i++) {
                if (i > 0) printf(", ");
                print_standard(e->data.function.args[i], 0);
            }
            printf("}");
        }
        else {
            print_standard(e->data.function.head, 1000);
            printf("[");
            for (size_t i = 0; i < e->data.function.arg_count; i++) {
                if (i > 0) printf(", ");
                print_standard(e->data.function.args[i], 0);
            }    
            printf("]");
        }
    } else {
        print_standard(e->data.function.head, 1000);
        printf("[");
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            if (i > 0) printf(", ");
            print_standard(e->data.function.args[i], 0);
        }    
        printf("]");
    }

    if (need_parens) printf(")");
}

void expr_print(Expr* e) {
    print_standard(e, 0);
    printf("\n");
}

char* expr_to_string(Expr* e) {
    // A bit of a hack: print to a memory buffer
    char* buffer = NULL;
    size_t len;
    FILE* stream = open_memstream(&buffer, &len);
    if (!stream) return NULL;

    // Temporarily redirect stdout
    FILE* old_stdout = stdout;
    stdout = stream;
    
    print_standard(e, 0);
    
    // Restore stdout and close stream
    stdout = old_stdout;
    fclose(stream);
    
    return buffer;
}

char* expr_to_string_fullform(Expr* e) {
    char* buffer = NULL;
    size_t len;
    FILE* stream = open_memstream(&buffer, &len);
    if (!stream) return NULL;

    FILE* old_stdout = stdout;
    stdout = stream;
    
    expr_print_fullform(e);
    
    stdout = old_stdout;
    fclose(stream);
    
    return buffer;
}

#include "symtab.h"

Expr* builtin_print(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    for (size_t i = 0; i < res->data.function.arg_count; i++) {
        print_standard(res->data.function.args[i], 0);
    }
    printf("\n");
    return expr_new_symbol("Null");
}

Expr* builtin_fullform(Expr* res) {
    (void)res;
    return NULL; // Unevaluated wrapper
}

Expr* builtin_inputform(Expr* res) {
    (void)res;
    return NULL; // Unevaluated wrapper
}

/*
 * Pretty-prints SeriesData[x, x0, {a0, ..., a_{k-1}}, nmin, nmax, den] as
 *     a0 + a1 (x-x0) + ... + a_{k-1} (x-x0)^((nmin+k-1)/den) + O[x-x0]^(nmax/den).
 * The rendering is built by constructing an equivalent Plus-expression of
 * Times/Power nodes and delegating to print_standard so that sign handling,
 * precedence, and fractional-exponent formatting are shared with ordinary
 * expressions. If the arguments are not in a shape we can render (for
 * example non-literal nmin/nmax/den or a non-List coefficient container),
 * we fall back to the generic head[args...] form.
 */

static int64_t abs_i64(int64_t v) { return v < 0 ? -v : v; }

/* Builds (x - x0), or just x when x0 is a literal zero. The result is a
 * fresh expression owned by the caller. Common negative-number cases for
 * x0 are normalised (e.g. x - (-2) becomes x + 2) so that the Plus printer
 * produces a clean subtraction. */
static Expr* series_build_base(Expr* x, Expr* x0) {
    if (x0->type == EXPR_INTEGER && x0->data.integer == 0) return expr_copy(x);
    if (x0->type == EXPR_REAL && x0->data.real == 0.0) return expr_copy(x);

    if (x0->type == EXPR_INTEGER && x0->data.integer < 0) {
        Expr* args[2] = { expr_copy(x), expr_new_integer(-x0->data.integer) };
        return expr_new_function(expr_new_symbol("Plus"), args, 2);
    }
    if (x0->type == EXPR_REAL && x0->data.real < 0.0) {
        Expr* args[2] = { expr_copy(x), expr_new_real(-x0->data.real) };
        return expr_new_function(expr_new_symbol("Plus"), args, 2);
    }

    Expr* neg_args[2] = { expr_new_integer(-1), expr_copy(x0) };
    Expr* neg = expr_new_function(expr_new_symbol("Times"), neg_args, 2);
    Expr* plus_args[2] = { expr_copy(x), neg };
    return expr_new_function(expr_new_symbol("Plus"), plus_args, 2);
}

/* Builds base^(num/den) as an Expr, reducing the exponent to lowest terms.
 * Returns NULL to signal "exponent is zero" (in which case the caller
 * should use the coefficient alone) and a fresh base-copy when the
 * reduced exponent is 1. */
static Expr* series_build_power(Expr* base, int64_t num, int64_t den) {
    if (num == 0) return NULL;
    int64_t g = gcd(abs_i64(num), abs_i64(den));
    if (g == 0) g = 1;
    int64_t n = num / g;
    int64_t d = den / g;
    if (d < 0) { d = -d; n = -n; }

    if (d == 1 && n == 1) return expr_copy(base);

    Expr* exp;
    if (d == 1) exp = expr_new_integer(n);
    else exp = make_rational(n, d);

    Expr* pargs[2] = { expr_copy(base), exp };
    return expr_new_function(expr_new_symbol("Power"), pargs, 2);
}

/* Combines a coefficient with an optional power term into a single summand.
 * Returns NULL when the coefficient is a literal zero (i.e. the term is
 * omitted from the displayed sum). A NULL power corresponds to (x-x0)^0
 * and yields just the coefficient. */
static Expr* series_build_term(Expr* coef, Expr* power /* borrowed, may be NULL */) {
    if (coef->type == EXPR_INTEGER && coef->data.integer == 0) return NULL;
    if (coef->type == EXPR_REAL && coef->data.real == 0.0) return NULL;

    if (power == NULL) return expr_copy(coef);

    bool coef_is_one = (coef->type == EXPR_INTEGER && coef->data.integer == 1);
    if (coef_is_one) {
        /* A bare Plus would merge visually with the surrounding series
         * Plus (equal precedence -> no parens), so wrap it in a 1-arg
         * Times to force the Times printer to add parentheses. */
        if (power->type == EXPR_FUNCTION &&
            power->data.function.head->type == EXPR_SYMBOL &&
            strcmp(power->data.function.head->data.symbol, "Plus") == 0) {
            Expr* targs[1] = { expr_copy(power) };
            return expr_new_function(expr_new_symbol("Times"), targs, 1);
        }
        return expr_copy(power);
    }

    Expr* args[2] = { expr_copy(coef), expr_copy(power) };
    return expr_new_function(expr_new_symbol("Times"), args, 2);
}

/* Renders e as the generic head[arg1, arg2, ...] form. Used as a fallback
 * when SeriesData has unexpected argument shapes. */
static void print_series_generic(Expr* e, int parent_prec) {
    (void)parent_prec;
    print_standard(e->data.function.head, 1000);
    printf("[");
    for (size_t i = 0; i < e->data.function.arg_count; i++) {
        if (i > 0) printf(", ");
        print_standard(e->data.function.args[i], 0);
    }
    printf("]");
}

static void print_series_data(Expr* e, int parent_prec) {
    Expr* x        = e->data.function.args[0];
    Expr* x0       = e->data.function.args[1];
    Expr* coefs    = e->data.function.args[2];
    Expr* nmin_e   = e->data.function.args[3];
    Expr* nmax_e   = e->data.function.args[4];
    Expr* den_e    = e->data.function.args[5];

    bool coefs_ok = (coefs->type == EXPR_FUNCTION &&
                     coefs->data.function.head->type == EXPR_SYMBOL &&
                     strcmp(coefs->data.function.head->data.symbol, "List") == 0);
    bool ints_ok  = (nmin_e->type == EXPR_INTEGER &&
                     nmax_e->type == EXPR_INTEGER &&
                     den_e->type  == EXPR_INTEGER &&
                     den_e->data.integer != 0);
    if (!coefs_ok || !ints_ok) {
        print_series_generic(e, parent_prec);
        return;
    }

    int64_t nmin = nmin_e->data.integer;
    int64_t nmax = nmax_e->data.integer;
    int64_t den  = den_e->data.integer;
    size_t  k    = coefs->data.function.arg_count;

    Expr* base = series_build_base(x, x0);

    /* Collect non-zero summands plus the trailing O-term. */
    Expr** terms = malloc(sizeof(Expr*) * (k + 1));
    size_t tcount = 0;

    for (size_t i = 0; i < k; i++) {
        int64_t num = nmin + (int64_t)i;
        Expr* power = series_build_power(base, num, den);
        Expr* term  = series_build_term(coefs->data.function.args[i], power);
        if (power) expr_free(power);
        if (term) terms[tcount++] = term;
    }

    /* Trailing O[base]^(nmax/den) term. Always emitted, even when all
     * coefficients are zero, because it conveys the truncation order. */
    {
        Expr* oargs[1] = { expr_copy(base) };
        Expr* o_call   = expr_new_function(expr_new_symbol("O"), oargs, 1);
        int64_t g  = gcd(abs_i64(nmax), abs_i64(den));
        if (g == 0) g = 1;
        int64_t n  = nmax / g;
        int64_t d  = den  / g;
        if (d < 0) { d = -d; n = -n; }
        Expr* exp_expr = (d == 1) ? expr_new_integer(n) : make_rational(n, d);
        Expr* pargs[2] = { o_call, exp_expr };
        terms[tcount++] = expr_new_function(expr_new_symbol("Power"), pargs, 2);
    }

    Expr* sum;
    if (tcount == 1) {
        sum = terms[0];
    } else {
        sum = expr_new_function(expr_new_symbol("Plus"), terms, tcount);
    }
    free(terms);

    print_standard(sum, parent_prec);
    expr_free(sum);
    expr_free(base);
}
