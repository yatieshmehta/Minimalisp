#include "lval.h"
#include "lenv.h"
#include "builtin.h"

lval* lval_eval(lenv* e, lval* v);

lval* lval_call(lenv* e, lval* f, lval* a) {
    if (f->builtin) { return f->builtin(e, a); }

    int given = a->num;
    int total = f->formals->num;

    while (a->num) {
        if (f->formals->num == 0) {
            lval_del(a);
            return lval_err("Function passed too many arguments. Got %i, expected %i.", given, total);
        }

        lval* sym = lval_pop(f->formals, 0);

        if (strcmp(sym->str, "&") == 0) {
            if (f->formals->num != 1) {
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

    if (f->formals->num > 0 && strcmp(f->formals->cell[0]->str, "&") == 0) {
        if (f->formals->num != 2) {
            return lval_err("Function format invalid. Symbol '&' not followed by a single symbol.");
        }
    
        lval_del(lval_pop(f->formals, 0));
        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_qexpr();

        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }

    if (f->formals->num == 0) {
        f->env->parent = e;
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    } else {
        return lval_copy(f);
    }
}

/* Evaluation */

lval* lval_eval_sexpr(lenv* e, lval* v) {
    
    for (int i = 0; i < v->num; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }
    
    for (int i = 0; i < v->num; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }
    
    if (v->num == 0) { return v; }    
    if (v->num == 1) { return lval_take(v, 0); }
    
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