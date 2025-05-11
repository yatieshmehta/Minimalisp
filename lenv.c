#include "lval.h"

typedef struct lenv lenv;

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
        new->syms[i] = malloc(strlen(env->syms[i]) + 1); // Maybe have to change this back to malloc
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
        if (strcmp(e->syms[i], k->str) == 0) {
            return lval_copy(e->vals[i]);
        }
    }

    if (e->parent) {
        return lenv_get(e->parent, k);
    } else {
        return lval_err("Unbound Symbol '%s'", k->str);
    }

}

void lenv_put(lenv* e, lval* k, lval* v) {
    
    /* Iterate over all items in environment */
    /* This is to see if variable already exists */
    for (int i = 0; i < e->count; i++) {
        /* If variable is found delete item at that position */
        /* And replace with variable supplied by user */
        if (strcmp(e->syms[i], k->str) == 0) {
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
    e->syms[e->count-1] = malloc(strlen(k->str)+1);
    strcpy(e->syms[e->count-1], k->str);
}

void lenv_def(lenv* env, lval* sym, lval* val) {
    while(env->parent) {
        env = env->parent; 
    }

    lenv_put(env, sym, val);
}
