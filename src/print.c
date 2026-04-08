
// Print

#include "print.h"
#include "expr.h"
#include "arithmetic.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <inttypes.h>

static void print_standard(Expr* e, int parent_prec);

static int get_expr_prec(Expr* e) {
    if (e->type != EXPR_FUNCTION) return 1000;
    if (e->data.function.head->type != EXPR_SYMBOL) return 1000;
    
    const char* head = e->data.function.head->data.symbol;
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
        case EXPR_SYMBOL: printf("%s", e->data.symbol); break;
        case EXPR_STRING: printf("\"%s\"", e->data.string); break;
        case EXPR_FUNCTION: print_function_fullform(e); break;
        case EXPR_BIGINT: {
            char* str = mpz_get_str(NULL, 10, e->data.bigint);
            printf("%s", str);
            free(str);
            break;
        }
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
        
        if (strcmp(head, "Rational") == 0 && e->data.function.arg_count == 2) {
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
            if (!re_zero) { 
                print_standard(re, 310); 
                if (im_minus_one) printf(" - ");
                else printf(" + "); 
            } else if (im_minus_one) {
                printf("-");
            }
            if (im_one || im_minus_one) printf("I");
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
            Expr** num_args = malloc(sizeof(Expr*) * count);
            Expr** den_args = malloc(sizeof(Expr*) * count);
            size_t num_count = 0, den_count = 0;
            
            for (size_t i = 0; i < count; i++) {
                Expr* arg = e->data.function.args[i];
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
                } else if (arg->type == EXPR_FUNCTION && arg->data.function.head->type == EXPR_SYMBOL && strcmp(arg->data.function.head->data.symbol, "Rational") == 0 && arg->data.function.arg_count == 2) {
                    Expr* n = arg->data.function.args[0];
                    Expr* d = arg->data.function.args[1];
                    if (!(n->type == EXPR_INTEGER && n->data.integer == 1)) {
                        num_args[num_count++] = expr_copy(n);
                    }
                    den_args[den_count++] = expr_copy(d);
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
