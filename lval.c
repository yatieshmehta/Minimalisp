#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "lenv.h"
#include "mpc.h"

enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR, LVAL_STR, LVAL_STRUCT, LVAL_INST, LVAL_MAT };
typedef struct lval lval;
typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
    int type;
    long num;
    long rows;
    long cols;
    long* data;
    // char* err;
    // char* sym;
    char* str;
    lbuiltin builtin;
    lenv* env;
    lval* formals;
    lval* body;
    
    /* Structs */
    // char* struc;
    // lval* fields;
    
    // int count;
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
    v->str = malloc(512);
    
    /* printf the error string with a maximum of 511 characters */
    vsnprintf(v->str, 511, fmt, va);
    
    /* Reallocate to number of bytes actually used */
    v->str = realloc(v->str, strlen(v->str)+1);
    
    /* Cleanup our va list */
    va_end(va);
    
    return v;
}

lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->str = malloc(strlen(s)+1);
    strcpy(v->str, s);
    return v;
}

lval* lval_str(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(s)+1);
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
    v->num = 0;
    v->cell = NULL;
    return v;
}

lval* lval_qexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->num = 0;
    v->cell = NULL;
    return v;
}

lval* lval_struct(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STRUCT;
    v->body = NULL;
    return v;
}

lval* lval_instance(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_INST;
    v->str = NULL;
    v->body = NULL;
    return v;
}

lval* lval_matrix(long rows, long cols, lval* data) {
    lval* v = malloc(sizeof(lval));
    v->data = malloc(sizeof(long) * data->num);
    // lval* v = calloc(1, sizeof(lval));
    // v->data = calloc(v->num, sizeof(long));

    v->type = LVAL_MAT;
    v->cols = cols;
    v->rows = rows;
    v->num = data->num;
    // v->body = data;
    for (int i=0; i<data->num; i++) {
        if (!data->cell[i]) {
            printf("data->cell[%d] is NULL!\n", i);
            continue;
        }
        printf("data->cell[%d]->num = %ld\n", i, data->cell[i]->num);
        v->data[i] = data->cell[i]->num;
    }
    // lval_del(data); May not need this
    printf("\nLVAL_MATRIX: %ldx%ld with count %li\n", v->rows, v->cols, v->num);
    printf("v (in matrix): ptr=%p rows=%ld cols=%ld num=%ld\n", (void*)v, v->rows, v->cols, v->num);
    return v;
}


void lenv_del(lenv* e);

void lval_del(lval* v) {
    // return;
    switch (v->type) {
        case LVAL_NUM: break;
        case LVAL_ERR: free(v->str); break;
        case LVAL_SYM: free(v->str); break;
        case LVAL_STR: free(v->str); break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for (int i = 0; i < v->num; i++) {
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
            lval_del(v->body);
            break; 
        case LVAL_INST:
            free(v->str);
            lval_del(v->body);
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
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str); break;
            
        case LVAL_SYM:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str); break;
        
        case LVAL_STR:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str); break;

        /* Copy Lists by copying each sub-expression */
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->num = v->num;
            x->cell = malloc(sizeof(lval*) * x->num);
            for (int i = 0; i < x->num; i++) {
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
             x->body = lval_copy(v->body);
             break;
        case LVAL_INST:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str);
            x->body = lval_copy(v->body);
    }
    
    return x;
}

lval* lval_add(lval* v, lval* x) {
    v->num++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->num);
    v->cell[v->num-1] = x;
    return v;
}

lval* lval_join(lval* x, lval* y) {    
    for (int i = 0; i < y->num; i++) {
        x = lval_add(x, y->cell[i]);
    }
    free(y->cell);
    free(y);    
    return x;
}
void lval_print(lval* v);
char* ltype_name(int t);

lval* lval_pop(lval* v, int i) {
    // counter++;
    // printf("\nHERE:\t");
    // printf("%d", counter);
    // if (!v) {
    //     printf("FATAL: lval_pop called with v == NULL\n");
    //     exit(1);
    // }
    // if (!v->cell) {
    //     printf("FATAL: lval_pop called with v->cell == NULL, count = %d\n", v->num);
    //     // printf("%s", ltype_name(v->cell[i]->type));
    //     lval_print(v);
    //     exit(1);
    // }
    // if (i < 0 || i >= v->num) {
    //     printf("FATAL: lval_pop index %d out of bounds (count = %d)\n", i, v->num);
    //     exit(1);
    // }
    // fflush(stdout);
    // lval* x = v->cell[i];
    // if (v->num == 1) {
    //     free(v->cell);
    //     v->cell = NULL;
    // } else {
    //     memmove(&v->cell[i], &v->cell[i+1],
    //         sizeof(lval*) * (v->num-i-1));    
    //     v->cell = realloc(v->cell, sizeof(lval*) * v->num);
    // }

    // v->num--;
    // return x;

    lval* x = v->cell[i];
    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->num-i-1));
    v->num--;
    v->cell = realloc(v->cell, sizeof(lval*) * v->num);
    
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
        case LVAL_SYM: return strcmp(x->str, y->str) == 0;
        case LVAL_ERR: return strcmp(x->str, y->str) == 0;
        case LVAL_STR: return strcmp(x->str, y->str) == 0;
        case LVAL_FUN:
            if (x->builtin || y->builtin) {
                return x->builtin == y->builtin;
            } else {
                return lval_eq(x->formals, y->formals) && lval_eq(x->body, y->body);
            }
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if (x->num != y->num) { return 0; }
            for (int i=0; i < x->num; i++) {
                if (!lval_eq(x->cell[i], y->cell[i])) { return 0; }
            }

            return 1;
        break;
    }

    return 0;
}



void lval_print_expr(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->num; i++) {
        lval_print(v->cell[i]);        
        if (i != (v->num-1)) {
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
    // free(escaped);
}

void lval_print(lval* v) {
    switch (v->type) {
        case LVAL_NUM:     printf("%li", v->num); break;
        case LVAL_ERR:     printf("Error: %s", v->str); break;
        case LVAL_SYM:     printf("%s", v->str); break;
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