#ifndef LVAL_H
#define LVAL_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR, LVAL_STR, LVAL_STRUCT, LVAL_INST, LVAL_MAT, LVAL_VEC };

typedef struct lenv lenv;
typedef struct lval lval;
typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
    int type;
    long num;
    long rows;
    long cols;
    long* data;
    char* str;
    lbuiltin builtin;
    lenv* env;
    lval* formals;
    lval* body;
    lval** cell;};


lval* lval_num(long x);
lval* lval_err(char* fmt, ...);
lval* lval_sym(char* s);
lval* lval_str(char* s);
lval* lval_builtin(lbuiltin func);
lval* lval_lambda(lval* formals, lval* body);
lval* lval_sexpr(void);
lval* lval_qexpr(void);
lval* lval_struct(void);
lval* lval_instance(void);
lval* lval_matrix(long rows, long cols, lval* data);
lval* lval_vector(int size, lval* data);

void lval_del(lval* v);
lval* lval_copy(lval* v);

lval* lval_add(lval* v, lval* x);
lval* lval_join(lval* x, lval* y);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
int lval_eq(lval* x, lval* y);

void lval_print(lval* v);
void lval_print_expr(lval* v, char open, char close);
void lval_print_str(lval* v);
void lval_println(lval* v);

char* ltype_name(int t);

#endif
