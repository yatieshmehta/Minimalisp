#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "lenv.h"
#include "mpc.h"
#include "arena.h"

enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR, LVAL_STR, LVAL_STRUCT, LVAL_INST, LVAL_MAT, LVAL_VEC };
int counter = 0;
typedef struct lval lval;
typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
    int type;

    long num;
    int rows;
    int cols;

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
    lval* v = arena_alloc(global_arena, sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

lval* lval_err(char* fmt, ...) {
    lval* v = arena_alloc(global_arena, sizeof(lval));
    v->type = LVAL_ERR;
    
    /* Create a va list and initialize it */
    va_list va;
    va_start(va, fmt);
    
    /* Allocate 512 bytes of space */
    char* err = malloc(512);
    
    /* printf the error string with a maximum of 511 characters */
    vsnprintf(err, 511, fmt, va);
    
    /* Reallocate to number of bytes actually used */
    // v->err = realloc(v->err, strlen(v->err)+1);
    v->err = arena_alloc(global_arena, strlen(err)+1);
    strcpy(v->err, err);
    free(err);    
    
    /* Cleanup our va list */
    va_end(va);
    
    return v;
}

lval* lval_sym(char* s) {
    lval* v = arena_alloc(global_arena, sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = arena_alloc(global_arena, strlen(s)+1);
    strcpy(v->sym, s);
    return v;
}

lval* lval_str(char* s) {
    lval* v = arena_alloc(global_arena, sizeof(lval));
    v->type = LVAL_STR;
    v->str = arena_alloc(global_arena, strlen(s)+1);
    strcpy(v->str, s);
    return v;
}

lval* lval_builtin(lbuiltin func) {
    lval* v = arena_alloc(global_arena, sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = func;
    return v;
}

lenv* lenv_new(void);

lval* lval_lambda(lval* formals, lval* body) {
    lval* v = arena_alloc(global_arena, sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = NULL;
    v->env = lenv_new();
    v->formals = formals;
    v->body = body;
    return v;
}

lval* lval_sexpr(void) {
    lval* v = arena_alloc(global_arena, sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_qexpr(void) {
    lval* v = arena_alloc(global_arena, sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_struct(void) {
    lval* v = arena_alloc(global_arena, sizeof(lval));
    v->type = LVAL_STRUCT;
    v->fields = NULL;
    return v;
}

lval* lval_instance(void) {
    lval* v = arena_alloc(global_arena, sizeof(lval));
    v->type = LVAL_INST;
    v->struc = NULL;
    v->fields = NULL;
    return v;
}

lval* lval_matrix(int cols, int rows, lval* data) {
    lval* v = arena_alloc(global_arena, sizeof(lval));
    v->type = LVAL_MAT;
    v->cols = cols;
    v->rows = rows;
    v->body = data;

    return v;
}

void lenv_del(lenv* e);

void lval_del(lval* v) {
    switch (v->type) {
        // case LVAL_NUM: break;
        // case LVAL_ERR: free(v->err); break;
        // case LVAL_SYM: free(v->sym); break;
        // case LVAL_STR: free(v->str); break;
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
            // free(v->struc);
            lval_del(v->fields);
    }
    
    // free(v);
}

lenv* lenv_copy(lenv* env);

lval* lval_copy(lval* v) {

    lval* x = arena_alloc(global_arena, sizeof(lval));
    x->type = v->type;
    
    switch (v->type) {
        
        /* Copy Functions and Numbers Directly */
        case LVAL_NUM: x->num = v->num; break;
        
        /* Copy Strings using malloc and strcpy */
        case LVAL_ERR:
            x->err = arena_alloc(global_arena, strlen(v->err) + 1);
            strcpy(x->err, v->err); break;
            
        case LVAL_SYM:
            x->sym = arena_alloc(global_arena, strlen(v->sym) + 1);
            strcpy(x->sym, v->sym); break;
        
        case LVAL_STR:
            x->str = arena_alloc(global_arena, strlen(v->str) + 1);
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
            x->struc = arena_alloc(global_arena, strlen(v->struc) + 1);
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
    // free(y);
    return x;
}
void lval_print(lval* v);
char* ltype_name(int t);

lval* lval_pop(lval* v, int i) {
    lval* x = v->cell[i];
    if (v->count == 1) {
        free(v->cell); // Might lead to double free, maybe remove this later?
        v->cell = NULL;
    } else {
        memmove(&v->cell[i], &v->cell[i+1],
            sizeof(lval*) * (v->count-i-1));    
        v->cell = realloc(v->cell, sizeof(lval*) * (v->count-1));
    }

    v->count--;
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
    char* escaped = arena_alloc(global_arena, strlen(v->str) + 1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("\"%s\"", escaped);
    // free(escaped);
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