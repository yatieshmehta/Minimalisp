#ifndef LENV_H
#define LENV_H

typedef struct lenv lenv;
typedef struct lval lval;

struct lenv {
    lenv* parent;
    int count;
    char** syms;
    lval** vals;
};

lenv* lenv_new(void);
void lenv_del(lenv* e);
lenv* lenv_copy(lenv* env);
lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv* e, lval* k, lval* v);
void lenv_def(lenv* env, lval* sym, lval* val);

#endif
