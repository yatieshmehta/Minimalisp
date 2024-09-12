#include "mpc.h"

#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

/* Forward Declarations */

mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Lispy;

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

/* Lisp Value */

enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR, LVAL_STR, LVAL_STRUCT, LVAL_INST };

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
    int type;

    long num;
    char* err;
    char* sym;
    char* str;

    lbuiltin builtin;
    lenv* env;
    lval* formals;
    lval* body;

    /* Structs */
    char* struc;
    lval* fields;

    int count;
    lval** cell;
};

lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

lval* lval_err(char* fmt, ...) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    
    /* Create a va list and initialize it */
    va_list va;
    va_start(va, fmt);
    
    /* Allocate 512 bytes of space */
    v->err = malloc(512);
    
    /* printf the error string with a maximum of 511 characters */
    vsnprintf(v->err, 511, fmt, va);
    
    /* Reallocate to number of bytes actually used */
    v->err = realloc(v->err, strlen(v->err)+1);
    
    /* Cleanup our va list */
    va_end(va);
    
    return v;
}

lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

lval* lval_str(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}

lval* lval_builtin(lbuiltin func) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = func;
    return v;
}

lenv* lenv_new(void);

lval* lval_lambda(lval* formals, lval* body) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = NULL;
    v->env = lenv_new();
    v->formals = formals;
    v->body = body;
    return v;
}

lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_qexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_struct(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STRUCT;
    v->fields = NULL;
    return v;
}

lval* lval_instance(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_INST;
    v->struc = NULL;
    v->fields = NULL;
    return v;
}

void lenv_del(lenv* e);

void lval_del(lval* v) {

    switch (v->type) {
        case LVAL_NUM: break;
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;
        case LVAL_STR: free(v->str); break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
        break;
        case LVAL_FUN:
            if (!v->builtin) {
                lenv_del(v->env);
                lval_del(v->body);
                lval_del(v->formals);
            }
            break;
        case LVAL_STRUCT:
            lval_del(v->fields);
            break; 
        case LVAL_INST:
            free(v->struc);
            lval_del(v->fields);
    }
    
    free(v);
}

lenv* lenv_copy(lenv* env);

lval* lval_copy(lval* v) {

    lval* x = malloc(sizeof(lval));
    x->type = v->type;
    
    switch (v->type) {
        
        /* Copy Functions and Numbers Directly */
        case LVAL_NUM: x->num = v->num; break;
        
        /* Copy Strings using malloc and strcpy */
        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err); break;
            
        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->sym, v->sym); break;
        
        case LVAL_STR:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str); break;

        /* Copy Lists by copying each sub-expression */
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;
        case LVAL_FUN:
            if (v->builtin) {
                x->builtin = v->builtin;
            } else {
                x->builtin = NULL;
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;
        case LVAL_STRUCT:
             x->fields = lval_copy(v->fields);
             break;
        case LVAL_INST:
            x->struc = malloc(strlen(v->struc) + 1);
            strcpy(x->struc, v->struc);
            x->fields = lval_copy(v->fields);
    }
    
    return x;
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

lval* lval_join(lval* x, lval* y) {    
    for (int i = 0; i < y->count; i++) {
        x = lval_add(x, y->cell[i]);
    }
    free(y->cell);
    free(y);    
    return x;
}

lval* lval_pop(lval* v, int i) {
    lval* x = v->cell[i];    
    memmove(&v->cell[i], &v->cell[i+1],
        sizeof(lval*) * (v->count-i-1));    
    v->count--;    
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

int lval_eq(lval* x, lval* y) {
    if (x->type != y->type) { return 0; }

    switch (x->type) {
        case LVAL_NUM: return x->num == y->num;
        case LVAL_SYM: return strcmp(x->sym, y->sym) == 0;
        case LVAL_ERR: return strcmp(x->sym, y->sym) == 0;
        case LVAL_STR: return strcmp(x->str, y->str) == 0;
        case LVAL_FUN:
            if (x->builtin || y->builtin) {
                return x->builtin == y->builtin;
            } else {
                return lval_eq(x->formals, y->formals) && lval_eq(x->body, y->body);
            }
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if (x->count != y->count) { return 0; }
            for (int i=0; i < x->count; i++) {
                if (!lval_eq(x->cell[i], y->cell[i])) { return 0; }
            }

            return 1;
        break;
    }

    return 0;
}

void lval_print(lval* v);

void lval_print_expr(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);        
        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print_str(lval* v) {
    char* escaped = malloc(strlen(v->str) + 1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("\"%s\"", escaped);
    free(escaped);
}

void lval_print(lval* v) {
    switch (v->type) {
        case LVAL_NUM:     printf("%li", v->num); break;
        case LVAL_ERR:     printf("Error: %s", v->err); break;
        case LVAL_SYM:     printf("%s", v->sym); break;
        case LVAL_STR:     lval_print_str(v); break;
        case LVAL_SEXPR: lval_print_expr(v, '(', ')'); break;
        case LVAL_QEXPR: lval_print_expr(v, '{', '}'); break;
        case LVAL_FUN:
            if (v->builtin) {
                printf("<builtin>");
            } else {
                printf("(lambda ");
                lval_print(v->formals);
                putchar(' ');
                lval_print(v->body);
                putchar(')');
            }
            break;
        case LVAL_STRUCT:
            printf("<struct>");
            break;
        case LVAL_INST:
        printf("<instance>");
        break;
    }
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

char* ltype_name(int t) {
    switch(t) {
        case LVAL_FUN: return "Function";
        case LVAL_NUM: return "Number";
        case LVAL_ERR: return "Error";
        case LVAL_STR: return "String";
        case LVAL_SYM: return "Symbol";
        case LVAL_SEXPR: return "S-Expression";
        case LVAL_QEXPR: return "Q-Expression";
        case LVAL_STRUCT: return "Structure";
        case LVAL_INST: return "Instance";
        default: return "Unknown";
    }
}

/* Lisp Environment */

struct lenv {
    lenv* parent;
    int count;
    char** syms;
    lval** vals;
};

lenv* lenv_new(void) {

    /* Initialize struct */
    lenv* e = malloc(sizeof(lenv));
    e->parent = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;   
}

void lenv_del(lenv* e) {
    
    /* Iterate over all items in environment deleting them */
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    
    /* Free allocated memory for lists */
    free(e->syms);
    free(e->vals);
    free(e);
}

lenv* lenv_copy(lenv* env) {
    lenv *new = malloc(sizeof(lenv));
    new->parent = env->parent;
    new->count = env->count;
    new->syms = malloc(sizeof(char*) * new->count);
    new->vals = malloc(sizeof(lval*) * new->count);

    for (int i=0; i < new->count; i++) {
        new->syms[i] = malloc(strlen(env->syms[i]) + 1);
        strcpy(new->syms[i], env->syms[i]);
        new->vals[i] = lval_copy(env->vals[i]);
    }

    return new;
}

lval* lenv_get(lenv* e, lval* k) {
    
    /* Iterate over all items in environment */
    for (int i = 0; i < e->count; i++) {
        /* Check if the stored string matches the symbol string */
        /* If it does, return a copy of the value */
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }

    if (e->parent) {
        return lenv_get(e->parent, k);
    } else {
        return lval_err("Unbound Symbol '%s'", k->sym);
    }

}

void lenv_put(lenv* e, lval* k, lval* v) {
    
    /* Iterate over all items in environment */
    /* This is to see if variable already exists */
    for (int i = 0; i < e->count; i++) {
        /* If variable is found delete item at that position */
        /* And replace with variable supplied by user */
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }
    
    /* If no existing entry found allocate space for new entry */
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);
    
    /* Copy contents of lval and symbol string into new location */
    e->vals[e->count-1] = lval_copy(v);
    e->syms[e->count-1] = malloc(strlen(k->sym)+1);
    strcpy(e->syms[e->count-1], k->sym);
}

void lenv_def(lenv* env, lval* sym, lval* val) {
    while(env->parent) {
        env = env->parent; 
    }

    lenv_put(env, sym, val);
}

/* Builtins */

#define LASSERT(args, cond, fmt, ...) \
    if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
    LASSERT(args, args->cell[index]->type == expect, \
        "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
        func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
    LASSERT(args, args->count == num, \
        "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
        func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
    LASSERT(args, args->cell[index]->count != 0, \
        "Function '%s' passed {} for argument %i.", func, index);

lval* lval_eval(lenv* e, lval* v);

lval* builtin_lambda(lenv* e, lval* a) {
    LASSERT_NUM("lambda", a, 2);
    LASSERT_TYPE("lambda", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("lambda", a, 1, LVAL_QEXPR);

    for (int i=0; i < a->cell[0]->count; i++) {
        LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM), 
        "Cannot define a non-symbol. Got %s, expected %s",
        ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
    }
    
    lval* formals = lval_pop(a, 0);
    lval* body = lval_pop(a, 0);
    lval_del(a);

    return lval_lambda(formals, body);
}

lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}

lval* builtin_first(lenv* e, lval* a) {
    LASSERT_NUM("first", a, 1);
    LASSERT_TYPE("first", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("first", a, 0);
    
    lval* v = lval_take(a, 0);
    while (v->count > 1) { lval_del(lval_pop(v, 1)); }
    return v->cell[0];
}

lval* builtin_rest(lenv* e, lval* a) {
    LASSERT_NUM("rest", a, 1);
    LASSERT_TYPE("rest", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("rest", a, 0);

    lval* v = lval_take(a, 0);    
    lval_del(lval_pop(v, 0));
    return v;
}

lval* builtin_last(lenv* e, lval* a) {
    LASSERT_NUM("last", a, 1);
    LASSERT_TYPE("last", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("last", a, 0);

    lval* v = a->cell[0]->cell[a->cell[0]->count - 1];
    return v;
}

lval* builtin_eval(lenv* e, lval* a) {
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);
    
    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a) {
    
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }
    
    lval* x = lval_pop(a, 0);
    
    while (a->count) {
        lval* y = lval_pop(a, 0);
        x = lval_join(x, y);
    }
    
    lval_del(a);
    return x;
}

lval* builtin_op(lenv* e, lval* a, char* op) {
    
    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE(op, a, i, LVAL_NUM);
    }
    
    lval* x = lval_pop(a, 0);
    
    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;
    }
    
    while (a->count > 0) {    
        lval* y = lval_pop(a, 0);
        
        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_del(x); lval_del(y);
                x = lval_err("Division By Zero.");
                break;
            }
            x->num /= y->num;
        }
        
        lval_del(y);
    }
    
    lval_del(a);
    return x;
}

lval* builtin_cmp(lenv* e, lval* a, char* op) {
    LASSERT_NUM(op, a, 2);
    int r;

    if (strcmp(op, "eq") == 0) {
        r = lval_eq(a->cell[0], a->cell[1]);
    } else if (strcmp(op, "neq") == 0) {
        r = !lval_eq(a->cell[0], a->cell[1]);
    }

    lval_del(a);
    return lval_num(r);
}

lval* builtin_eq(lenv* e, lval* a) {
  return builtin_cmp(e, a, "eq");
}

lval* builtin_ne(lenv* e, lval* a) {
  return builtin_cmp(e, a, "neq");
}

lval* builtin_ord(lenv* e, lval* a, char* op) {
    LASSERT_NUM(op, a, 2);
    LASSERT_TYPE(op, a, 0, LVAL_NUM);
    LASSERT_TYPE(op, a, 1, LVAL_NUM);

    int r;
    if (strcmp(op, ">") == 0) {
        r = a->cell[0]->num > a->cell[1]->num;
    } else if (strcmp(op, "<") == 0) {
        r = a->cell[0]->num < a->cell[1]->num;
    } else if (strcmp(op, ">=") == 0) {
        r = a->cell[0]->num >= a->cell[1]->num;
    } else if (strcmp(op, "<=") == 0) {
        r = a->cell[0]->num <= a->cell[1]->num;
    //} else if (strcmp(op, "==") == 0) {
    //    r = a->cell[0]->num == a->cell[1]->num;
    }

    lval_del(a);
    return lval_num(r);
}

//lval* builtin_eq(lenv* e, lval* a) {
//    return builtin_ord(e, a, "==");
//}

lval* builtin_gt(lenv* e, lval* a) {
    return builtin_ord(e, a, ">");
}

lval* builtin_lt(lenv* e, lval* a) {
  return builtin_ord(e, a, "<");
}

lval* builtin_ge(lenv* e, lval* a) {
  return builtin_ord(e, a, ">=");
}

lval* builtin_le(lenv* e, lval* a) {
  return builtin_ord(e, a, "<=");
}

lval* builtin_int(lenv* e, lval* a) {
    LASSERT_NUM("integer?", a, 1);
    if (a->cell[0]->type == LVAL_NUM) {
        return lval_num(1);
    } else {
        return lval_num(0);
    }
}

lval* builtin_qexpr(lenv* e, lval* a) {
    LASSERT_NUM("qexpr?", a, 1);
    if (a->cell[0]->type == LVAL_QEXPR) {
        return lval_num(1);
    } else {
        return lval_num(0);
    }
}

lval* builtin_str(lenv* e, lval* a) {
    LASSERT_NUM("string?", a, 1);
    if (a->cell[0]->type == LVAL_STR) {
        return lval_num(1);
    } else {
        return lval_num(0);
    }
}

lval* builtin_if(lenv* e, lval* a) {

    for (int i=0; i < a->count; i++) {
        LASSERT_TYPE("if", a, i, LVAL_QEXPR);
        
        if (i != a->count - 1) {
            LASSERT(a, (a->cell[i]->cell[0]->type == LVAL_NUM || 
                a->cell[i]->cell[0]->type == LVAL_SEXPR),
            "Function 'if' condition passed incorrect type. Got %s, expected %s or %s.",
            ltype_name(a->cell[i]->cell[0]->type), ltype_name(LVAL_NUM), ltype_name(LVAL_SEXPR));
            
        } else {
            LASSERT(a, (a->cell[i]->cell[0]->type == LVAL_NUM || 
                (a->cell[i]->cell[0]->type == LVAL_SYM && strcmp(a->cell[i]->cell[0]->sym, "else") == 0) ||
                a->cell[i]->cell[0]->type == LVAL_SEXPR),
            "Function 'if' condition passed incorrect type. Got %s, expected %s, %s or 'else'.",
            ltype_name(a->cell[i]->cell[0]->type), ltype_name(LVAL_NUM), ltype_name(LVAL_SEXPR));
        }
    }

    lval* x = NULL;

    for (int i=0; i < a->count - 1; i++) {
        a->cell[i]->cell[0] = lval_eval(e, a->cell[i]->cell[0]);
        if (a->cell[i]->cell[0]->num) {
            x = lval_eval(e, lval_pop(a->cell[i], 1));
            break;
        }
    }

    if (!x && a->cell[a->count - 1]->cell[0]->type == LVAL_SYM && strcmp(a->cell[a->count - 1]->cell[0]->sym, "else") == 0) {
        x = lval_eval(e, lval_pop(a->cell[a->count - 1], 1));
    } else if (!x && a->cell[a->count - 1]->cell[0]->num) {
        if (a->cell[a->count - 1]->count > 1) {
            lval_print(a->cell[a->count - 1]->cell[2]);
        }
        x = lval_eval(e, lval_pop(a->cell[a->count - 1], 1)); 
    } else if (!x) {
        x = lval_sexpr();
    }

    lval_del(a);
    return x;
}

lval* builtin_and(lenv* e, lval* a) {
    for (int i=0; i < a->count; i++) {
        LASSERT_TYPE("and", a, i, LVAL_NUM);
    }

    for (int i=0; i < a->count; i++) {
        if (a->cell[i]->num == 0) {
            lval_del(a);
            return lval_num(0);
        }
    }
    
    lval_del(a);
    return lval_num(1);
}

lval* builtin_or(lenv* e, lval* a) {
    for (int i=0; i < a->count; i++) {
        LASSERT_TYPE("or", a, i, LVAL_NUM);
    }

    for (int i=0; i < a->count; i++) {
        if (a->cell[i]->num == 1) {
            lval_del(a);
            return lval_num(1);
        }
    }
    
    lval_del(a);
    return lval_num(0);
}

lval* builtin_not(lenv* e, lval* a) {
    LASSERT_NUM("not", a, 1);
    LASSERT_TYPE("not", a, 0, LVAL_NUM);

    if (a->cell[0]->num == 0) {
        return lval_num(1);
    } else {
        return lval_num(0);
    }
}

lval* builtin_add(lenv* e, lval* a) {
    return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a) {
    return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a) {
    return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a) {
    return builtin_op(e, a, "/");
}

lval* builtin_var(lenv* e, lval* a, char* func) {
    LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

    lval* syms = a->cell[0];

    for (int i=0; i < syms->count; i++) {
        LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
        "Function '%s' cannot define non-symbol. "
        "Got %s, Expected %s.", func,
        ltype_name(syms->cell[i]->type),
        ltype_name(LVAL_SYM));
    }

    LASSERT(a, (syms->count == a->count-1),
    "Function '%s' passed too many arguments for symbols. \
    Got %i, Expected %i.", func, syms->count, a->count-1);

    for (int i=0; i < syms->count; i++) {
        if (strcmp(func, "def") == 0) {
            lenv_def(e, syms->cell[i], a->cell[i + 1]);
        }

        if (strcmp(func, "=") == 0) {
            lenv_put(e, syms->cell[i], a->cell[i + 1]);
        }
    }
    
    lval_del(a);
    return lval_sexpr(); 
}

lval* builtin_def(lenv* e, lval* a) {
    return builtin_var(e, a, "def");
}

lval *builtin_put(lenv* e, lval* a) {
    return builtin_var(e, a, "=");
}

lval *builtin_empty(lenv* e, lval* a) {
    LASSERT_NUM("empty?", a, 1);
    LASSERT_TYPE("empty?", a, 0, LVAL_QEXPR);
    if (a->cell[0]->count == 0) {
        return lval_num(1);
    } else {
        return lval_num(0);
    }
}

lval* builtin_cons(lenv* e, lval* a){

    LASSERT_NUM("cons", a, 2);
    LASSERT_TYPE("cons", a, 1, LVAL_QEXPR);

    a->type = LVAL_QEXPR;
    if (a->cell[1]->type == LVAL_QEXPR && a->cell[1]->count == 0) {
        lval_pop(a, 1);
        return a;
    } else {
        lval* second = lval_pop(a, 1);
        return lval_join(a, second);
    }
}

lval* lval_read(mpc_ast_t* t);

lval* builtin_load(lenv* e, lval* a) {
    LASSERT_NUM("load", a, 1);
    LASSERT_TYPE("load", a, 0, LVAL_STR);

    mpc_result_t r;
    if (mpc_parse_contents(a->cell[0]->str, Lispy, &r)) {
        lval* expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        while(expr->count) {
            lval* x = lval_eval(e, lval_pop(expr, 0));
            if (x->type == LVAL_ERR) { lval_println(x); }
            lval_del(x);
        }

        lval_del(expr);
        lval_del(a);

        return lval_sexpr();

    } else {
        char* err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        lval* err = lval_err("Could not load Library %s", err_msg);
        free(err_msg);
        lval_del(a);

        return err;
    }
}

lval* builtin_print(lenv* e, lval* a) {

  /* Print each argument followed by a space */
  for (int i = 0; i < a->count; i++) {
    lval_print(a->cell[i]); putchar(' ');
  }

  /* Print a newline and delete arguments */
  putchar('\n');
  lval_del(a);

  return lval_sexpr();
}

lval* builtin_error(lenv* e, lval* a) {
  LASSERT_NUM("error", a, 1);
  LASSERT_TYPE("error", a, 0, LVAL_STR);

  /* Construct Error from first argument */
  lval* err = lval_err(a->cell[0]->str);

  /* Delete arguments and return */
  lval_del(a);
  return err;
}

lval* builtin_zero(lenv* e, lval* a) {
    LASSERT_NUM("zero?", a, 1);
    LASSERT_TYPE("zero?", a, 0, LVAL_NUM);

    return builtin_not(e, a);
}

lval* builtin_length(lenv* e, lval* a) {
    LASSERT_NUM("length", a, 1);
    LASSERT_TYPE("length", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("length", a, 0);
    
    return lval_num(a->cell[0]->count);
}

lval* builtin_get(lenv*e, lval* a) {
    LASSERT_NUM("get", a, 1);
    LASSERT_TYPE("get", a, 0, LVAL_QEXPR);
    LASSERT_NUM("get", a->cell[0], 2);

    lval* inst = lenv_get(e, a->cell[0]->cell[0]);
    lval* struc = lenv_get(e, inst);

    for (int i=0; i < struc->fields->count; i++) {
        if (strcmp(struc->fields->cell[i]->sym, a->cell[0]->cell[1]->sym) == 0) {
            return inst->fields->cell[i];
        }
    }
    return lval_err("Struct '%s' has no attribute '%s'.", inst->struc, a->cell[0]->cell[1]->sym);
}

lval* builtin_make(lenv* e, lval* a) {
    LASSERT_NUM("make", a, 2);
    LASSERT_TYPE("make", a, 0, LVAL_SYM);
    LASSERT_TYPE("make", a, 1, LVAL_QEXPR);

    lval* struc = lenv_get(e, a->cell[0]);

    LASSERT(a, (a->cell[1]->count - 1 == struc->fields->count),
    "Struct '%s' passed incorrect number of arguments. \
     Got %i, Expected %i.", a->cell[0], a->count-1, struc->fields->count);

    lval* name = lval_pop(a->cell[1], 0);
    lval* inst = lval_instance();
    inst->struc = a->cell[0]->sym;
    inst->fields = a->cell[1];
    lenv_put(e, name, inst);

    return lval_num(0);
}

lval* builtin_struct(lenv* e, lval* a) {
    lval* struc = lval_struct();
    lval* body = lval_pop(a, 0);
    lval* name = lval_pop(body, 0);

    struc->fields = body;
    lenv_put(e, name, struc);

    //char* make = malloc(strlen(a->cell[0]->cell[0]->sym) + 6);
    //strcpy(make, "make-"); 
    //strcat(make, a->cell[0]->cell[0]->sym);
    //free(make);

    return lval_num(0);
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
    lval* k = lval_sym(name);
    lval* v = lval_builtin(func);
    lenv_put(e, k, v);
    lval_del(k); lval_del(v);
}

void lenv_add_builtins(lenv* e) {   
    /* Variable Functions */
    lval* sym = lval_sym("empty");
    lval* empty = lval_qexpr();

    lenv_add_builtin(e, "last", builtin_last);
    lenv_add_builtin(e, "make", builtin_make);
    lenv_add_builtin(e, "struct", builtin_struct);
    lenv_add_builtin(e, "get", builtin_get);
    lenv_add_builtin(e, "length", builtin_length);
    lenv_add_builtin(e, "string?", builtin_str);
    lenv_add_builtin(e, "integer?", builtin_int);
    lenv_add_builtin(e, "qexpr?", builtin_qexpr);
    lenv_put(e, sym, empty);
    lenv_add_builtin(e, "zero?", builtin_zero);
    lenv_add_builtin(e, "cons", builtin_cons);
    lenv_add_builtin(e, "empty?", builtin_empty);
    lenv_add_builtin(e, "func", builtin_def);
    lenv_add_builtin(e, "=",   builtin_put);
    
    /* List Functions */
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "first", builtin_first);
    lenv_add_builtin(e, "rest", builtin_rest);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "lambda", builtin_lambda);
    
    /* Mathematical Functions */
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);

    /* Comparison Functions */
    lenv_add_builtin(e, "eq?", builtin_eq);
    lenv_add_builtin(e, "!=", builtin_ne);
    lenv_add_builtin(e, ">",  builtin_gt);
    lenv_add_builtin(e, "<",  builtin_lt);
    lenv_add_builtin(e, ">=", builtin_ge);
    lenv_add_builtin(e, "<=", builtin_le);

    /* Boolean Logic Functions */
    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, "and", builtin_and);
    lenv_add_builtin(e, "or", builtin_or);
    lenv_add_builtin(e, "int", builtin_int);
    lenv_add_builtin(e, "not", builtin_not);

    /* String Functions */
    lenv_add_builtin(e, "load",  builtin_load);
    lenv_add_builtin(e, "error", builtin_error);
    lenv_add_builtin(e, "print", builtin_print);

}

lval* lval_call(lenv* e, lval* f, lval* a) {
    if (f->builtin) { return f->builtin(e, a); }

    int given = a->count;
    int total = f->formals->count;

    while (a->count) {
        if (f->formals->count == 0) {
            lval_del(a);
            return lval_err("Function passed too many arguments. Got %i, expected %i.", given, total);
        }

        lval* sym = lval_pop(f->formals, 0);

        if (strcmp(sym->sym, "&") == 0) {
            if (f->formals->count != 1) {
                lval_del(a);
                return lval_err("Function invalid format. Symbol '&' not followed by single symbol.");
            }

            lval* nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a));
            lval_del(sym);
            lval_del(nsym);
            break;
        }

        lval* val = lval_pop(a, 0);
        lenv_put(f->env, sym, val);

        lval_del(sym);
        lval_del(val);
    }

    lval_del(a);

    if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) {
        if (f->formals->count != 2) {
            return lval_err("Function format invalid. Symbol '&' not followed by a single symbol.");
        }
    
        lval_del(lval_pop(f->formals, 0));
        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_qexpr();

        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }

    if (f->formals->count == 0) {
        f->env->parent = e;
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    } else {
        return lval_copy(f);
    }
}

/* Evaluation */

lval* lval_eval_sexpr(lenv* e, lval* v) {
    
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }
    
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }
    
    if (v->count == 0) { return v; }    
    if (v->count == 1) { return lval_take(v, 0); }
    
    /* Ensure first element is a function after evaluation */
    lval* f = lval_pop(v, 0);

    if (f->type == LVAL_SYM) {
            lval* x = lenv_get(e, f);
            lval_del(f);
            f = x;
            if (f->type == LVAL_ERR) { return f; }
        }

    if (f->type != LVAL_FUN) {
        lval* err = lval_err(
            "S-Expression starts with incorrect type. "
            "Got %s, Expected %s.",
            ltype_name(f->type), ltype_name(LVAL_FUN));
        lval_del(f);
        lval_del(v);
        return err;
    }
    
    /* If so call function to get result */
    lval* result = lval_call(e, f, v);
    lval_del(f);
    return result;
}   

lval* lval_eval(lenv* e, lval* v) {

    if (v->type == LVAL_SYM) {
        lval* x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
    return v;
}

/* Reading */

lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("Invalid Number.");
}

lval* lval_read_str(mpc_ast_t* t) {
  t->contents[strlen(t->contents)] = '\0';
  char* unescaped = malloc(strlen(t->contents)); // Used to be malloc(strlen(t->contents+1)+1)
  strcpy(unescaped, t->contents+1);
  unescaped = mpcf_unescape(unescaped);
  lval* str = lval_str(unescaped);
  free(unescaped);
  return str;
}


lval* lval_read(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "string")) { return lval_read_str(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
    
    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); } 
    if (strstr(t->tag, "sexpr"))    { x = lval_sexpr(); }
    if (strstr(t->tag, "qexpr"))    { x = lval_qexpr(); }
    
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        if (strstr(t->children[i]->tag,"comment")) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }
    
    return x;
}

/* Main */

int main(int argc, char** argv) {
    
    Number = mpc_new("number");
    Symbol = mpc_new("symbol");
    String = mpc_new("string");
    Comment = mpc_new("comment");
    Sexpr    = mpc_new("sexpr");
    Qexpr    = mpc_new("qexpr");
    Expr     = mpc_new("expr");
    Lispy    = mpc_new("lispy");
    
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                              \
            number  : /-?[0-9]+/ ;                                                     \
            symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&?]+/ ;                              \
            string  : /'\\b[\\w-]+/ ;                                                       \
            comment : /;[^\\r\\n]*/ ;                                                  \
            sexpr   : '(' <expr>* ')' ;                                                \
            qexpr   : '{' <expr>* '}' ;                                                \
            expr    : <number> | <symbol> | <string> | <comment> | <sexpr> | <qexpr> ; \
            lispy   : /^/ <expr>* /$/ ;                                                \
        ",
        Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

    lenv* e = lenv_new();
    lenv_add_builtins(e);

    mpc_result_t first;
    mpc_parse("<stdin>", "(func {def} (lambda {args body} {func (list (first args)) (lambda (rest args) body)}))", Lispy, &first);
    lval* func = lval_eval(e, lval_read(first.output));
    lval_del(func);
    mpc_ast_delete(first.output);
    
    if (argc == 1) {

        puts("Lispy Version 0.0.0.0.7");
        puts("Press Ctrl+c to Exit\n");    

        while (1) {
            char* input = readline("lispy> ");
            add_history(input);
            
            mpc_result_t r;
            if (mpc_parse("<stdin>", input, Lispy, &r)) {
                lval* x = lval_eval(e, lval_read(r.output));
                lval_println(x);
                lval_del(x);
                mpc_ast_delete(r.output);
            } else {        
                mpc_err_print(r.error);
                mpc_err_delete(r.error);
            }
            
            free(input);
            
        }
    } 
    if (argc >= 2) {
        for (int i=1; i < argc; i++) {
            lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));
            lval* x = builtin_load(e, args);
            if (x->type == LVAL_ERR) { lval_println(x); }
            lval_del(x);
        }
    }

    lenv_del(e);
    
    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);
    
    return 0;
}