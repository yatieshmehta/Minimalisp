#ifndef EVAL_H
#define EVAL_H

#include "lval.h"
#include "lenv.h"

lval* lval_call(lenv* e, lval* f, lval* a);
lval* lval_eval_sexpr(lenv* e, lval* v);
lval* lval_eval(lenv* e, lval* v);

#endif
