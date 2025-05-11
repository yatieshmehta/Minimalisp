#ifndef LVAL_BUILTINS_H
#define LVAL_BUILTINS_H

#include "lval.h"
#include "lenv.h"
#include "mpc.h"

lval* builtin_lambda(lenv* e, lval* a);
lval* builtin_matrix(lenv* e, lval* a);

lval* builtin_list(lenv* e, lval* a);
lval* builtin_first(lenv* e, lval* a);
lval* builtin_rest(lenv* e, lval* a);
lval* builtin_last(lenv* e, lval* a);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_join(lenv* e, lval* a);
lval* builtin_op(lenv* e, lval* a, char* op);
lval* builtin_cmp(lenv* e, lval* a, char* op);
lval* builtin_eq(lenv* e, lval* a);
lval* builtin_ne(lenv* e, lval* a);
lval* builtin_ord(lenv* e, lval* a, char* op);
lval* builtin_gt(lenv* e, lval* a);
lval* builtin_lt(lenv* e, lval* a);
lval* builtin_ge(lenv* e, lval* a);
lval* builtin_le(lenv* e, lval* a);
lval* builtin_int(lenv* e, lval* a);
lval* builtin_qexpr(lenv* e, lval* a);
lval* builtin_str(lenv* e, lval* a);
lval* builtin_if(lenv* e, lval* a);
lval* builtin_and(lenv* e, lval* a);
lval* builtin_or(lenv* e, lval* a);
lval* builtin_not(lenv* e, lval* a);
lval* builtin_add(lenv* e, lval* a);
lval* builtin_sub(lenv* e, lval* a);
lval* builtin_mul(lenv* e, lval* a);
lval* builtin_div(lenv* e, lval* a);
lval* builtin_var(lenv* e, lval* a, char* func);
lval* builtin_def(lenv* e, lval* a);
lval* builtin_put(lenv* e, lval* a);
lval* builtin_empty(lenv* e, lval* a);
lval* builtin_cons(lenv* e, lval* a);
lval* builtin_load(lenv* e, lval* a);
lval* builtin_print(lenv* e, lval* a);
lval* builtin_error(lenv* e, lval* a);
lval* builtin_zero(lenv* e, lval* a);
lval* builtin_length(lenv* e, lval* a);
lval* builtin_get(lenv* e, lval* a);
lval* builtin_make(lenv* e, lval* a);
lval* builtin_struct(lenv* e, lval* a);
void lenv_add_builtin(lenv* e, char* name, lbuiltin func);
void lenv_add_builtins(lenv* e);

#endif
