#include "lval.h"
#include "lenv.h"
#include "lassert.h"
#include "mpc.h"
#include "parser.h"
#include "arena.h"
#include "utils.h"
#include <string.h>

lval* lval_eval(lenv* e, lval* v);

lval* builtin_lambda(lenv* e, lval* a) {
    LASSERT_NUM("lambda", a, 2);
    LASSERT_TYPE("lambda", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("lambda", a, 1, LVAL_QEXPR);

    for (int i=0; i < a->cell[0]->num; i++) {
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
    return lval_take(v, 0);     
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

    lval* v = a->cell[0]->cell[a->cell[0]->num - 1];
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
    
    for (int i = 0; i < a->num; i++) {
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }
    
    lval* x = lval_pop(a, 0);
    
    while (a->num) {
        lval* y = lval_pop(a, 0);
        x = lval_join(x, y);
    }
    
    lval_del(a);
    return x;
}

lval* builtin_op(lenv* e, lval* a, char* op) {
    
    for (int i = 0; i < a->num; i++) {
        LASSERT_TYPE(op, a, i, LVAL_NUM);
    }
    
    lval* x = lval_pop(a, 0);
    
    if ((strcmp(op, "-") == 0) && a->num == 0) {
        x->num = -x->num;
    }
    
    while (a->num > 0) {    
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

    for (int i=0; i < a->num; i++) {
        LASSERT_TYPE("if", a, i, LVAL_QEXPR);
        
        if (i != a->num - 1) {
            LASSERT(a, (a->cell[i]->cell[0]->type == LVAL_NUM || 
                a->cell[i]->cell[0]->type == LVAL_SEXPR),
            "Function 'if' condition passed incorrect type. Got %s, expected %s or %s.",
            ltype_name(a->cell[i]->cell[0]->type), ltype_name(LVAL_NUM), ltype_name(LVAL_SEXPR));
            
        } else {
            LASSERT(a, (a->cell[i]->cell[0]->type == LVAL_NUM || 
                (a->cell[i]->cell[0]->type == LVAL_SYM && strcmp(a->cell[i]->cell[0]->str, "else") == 0) ||
                a->cell[i]->cell[0]->type == LVAL_SEXPR),
            "Function 'if' condition passed incorrect type. Got %s, expected %s, %s or 'else'.",
            ltype_name(a->cell[i]->cell[0]->type), ltype_name(LVAL_NUM), ltype_name(LVAL_SEXPR));
        }
    }

    lval* x = NULL;

    for (int i=0; i < a->num - 1; i++) {
        a->cell[i]->cell[0] = lval_eval(e, a->cell[i]->cell[0]);
        // LASSERT_TYPE("idk", a->cell[i], 0, LVAL_NUM);
        // if (a->cell[i]->cell[0]->type == LVAL_ERR) {printf("%s", a->cell[i]->cell[0]->str);}
        if (a->cell[i]->cell[0]->num) {
            x = lval_eval(e, lval_pop(a->cell[i], 1));
            break;
        }
    }

    if (!x && a->cell[a->num - 1]->cell[0]->type == LVAL_SYM && strcmp(a->cell[a->num - 1]->cell[0]->str, "else") == 0) {
        x = lval_eval(e, lval_pop(a->cell[a->num - 1], 1));
    } else if (!x && a->cell[a->num - 1]->cell[0]->num) {
        if (a->cell[a->num - 1]->num > 1) {
            lval_print(a->cell[a->num - 1]->cell[2]);
        }
        x = lval_eval(e, lval_pop(a->cell[a->num - 1], 1)); 
    } else if (!x) {
        x = lval_sexpr();
    }

    lval_del(a);
    return x;
}

lval* builtin_and(lenv* e, lval* a) {
    for (int i=0; i < a->num; i++) {
        LASSERT_TYPE("and", a, i, LVAL_NUM);
    }

    for (int i=0; i < a->num; i++) {
        if (a->cell[i]->num == 0) {
            lval_del(a);
            return lval_num(0);
        }
    }
    
    lval_del(a);
    return lval_num(1);
}

lval* builtin_or(lenv* e, lval* a) {
    for (int i=0; i < a->num; i++) {
        LASSERT_TYPE("or", a, i, LVAL_NUM);
    }

    for (int i=0; i < a->num; i++) {
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

    for (int i=0; i < syms->num; i++) {
        LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
        "Function '%s' cannot define non-symbol. "
        "Got %s, Expected %s.", func,
        ltype_name(syms->cell[i]->type),
        ltype_name(LVAL_SYM));
    }

    LASSERT(a, (syms->num == a->num-1),
    "Function '%s' passed too many arguments for symbols. \
    Got %i, Expected %i.", func, syms->num, a->num-1);

    for (int i=0; i < syms->num; i++) {
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
    if (a->cell[0]->num == 0) {
        return lval_num(1);
    } else {
        return lval_num(0);
    }
}

lval* builtin_cons(lenv* e, lval* a){

    LASSERT_NUM("cons", a, 2);
    LASSERT_TYPE("cons", a, 1, LVAL_QEXPR);

    a->type = LVAL_QEXPR;
    if (a->cell[1]->type == LVAL_QEXPR && a->cell[1]->num == 0) {
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

        while(expr->num) {
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
  for (int i = 0; i < a->num; i++) {
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
    
    return lval_num(a->cell[0]->num);
}

lval* builtin_get(lenv*e, lval* a) {
    LASSERT_NUM("get", a, 1);
    LASSERT_TYPE("get", a, 0, LVAL_QEXPR);
    LASSERT_NUM("get", a->cell[0], 2);

    lval* inst = lenv_get(e, a->cell[0]->cell[0]);
    lval* struc = lenv_get(e, inst);

    for (int i=0; i < struc->body->num; i++) {
        if (strcmp(struc->body->cell[i]->str, a->cell[0]->cell[1]->str) == 0) {
            return inst->body->cell[i];
        }
    }
    return lval_err("Struct '%s' has no attribute '%s'.", inst->str, a->cell[0]->cell[1]->str);
}

lval* builtin_make(lenv* e, lval* a) {
    LASSERT_NUM("make", a, 2);
    LASSERT_TYPE("make", a, 0, LVAL_SYM);
    LASSERT_TYPE("make", a, 1, LVAL_QEXPR);

    lval* struc = lenv_get(e, a->cell[0]);

    LASSERT(a, (a->cell[1]->num - 1 == struc->body->num),
    "Struct '%s' passed incorrect number of arguments. \
     Got %i, Expected %i.", a->cell[0], a->num-1, struc->body->num);

    lval* name = lval_pop(a->cell[1], 0);
    lval* inst = lval_instance();
    inst->str = a->cell[0]->str;
    inst->body = a->cell[1];
    lenv_put(e, name, inst);

    return lval_sexpr();
}

lval* builtin_struct(lenv* e, lval* a) {
    lval* struc = lval_struct();
    lval* body = lval_pop(a, 0);
    lval* name = lval_pop(body, 0);

    struc->body = body;
    lenv_put(e, name, struc);

    //char* make = malloc(strlen(a->cell[0]->cell[0]->str) + 6);
    //strcpy(make, "make-"); 
    //strcat(make, a->cell[0]->cell[0]->str);
    //free(make);

    return lval_sexpr();
}

lval* builtin_matrix(lenv* e, lval* a) {
    LASSERT_NUM("matrix", a, 4);
    LASSERT_TYPE("matrix", a, 0, LVAL_QEXPR);
    LASSERT_NUM("matrix symbol", a->cell[0], 1);
    LASSERT_TYPE("matrix symbol", a->cell[0], 0, LVAL_SYM);
    LASSERT_TYPE("matrix", a, 1, LVAL_NUM);
    LASSERT_TYPE("matrix", a, 2, LVAL_NUM);
    LASSERT_TYPE("matrix", a, 3, LVAL_QEXPR);

    for (int i=0; i<a->cell[3]->num; i++) {
        LASSERT_TYPE("matrix", a->cell[3], i, LVAL_NUM);
    }

    LASSERT(a, (a->cell[1]->num * a->cell[2]->num == a->cell[3]->num), "Data size should match matrix shape.");
    lval* v = lval_matrix(a->cell[1]->num, a->cell[2]->num, a->cell[3]);
    lenv_put(e, lval_take(lval_take(a, 0), 0), v); // Do I need to remove the lvaltake and just del a after? Thats what I do in add builtin
    return lval_sexpr();
}

lval* builtin_size(lenv* e, lval* a) {
    LASSERT_NUM("size?", a, 1);
    LASSERT_TYPE("size?", a, 0, LVAL_MAT);
    printf("Matrix of size %ldx%ld\n", a->cell[0]->rows, a->cell[0]->cols);
    return lval_str("");
}

lval* builtin_vector(lenv* e, lval* a);

lval* builtin_matmult(lenv* e, lval* a) {
    LASSERT_NUM("mat-mult", a, 2);
    LASSERT_TYPE("mat-mult", a, 0, LVAL_MAT);
    LASSERT_TYPE("mat-mult", a, 1, LVAL_MAT);
    LASSERT(a, (a->cell[0]->cols == a->cell[1]->rows), "Matrix shapes should align.");

    // lval_print(a);
    lval* A = lval_pop(a, 0);
    lval* B = lval_take(a, 0);

    long* result = arena_alloc(global_arena, sizeof(long) * A->rows * B->cols);

    for (int i=0; i<A->rows; i++) {
        for (int j=0; j<B->cols; j++) {
            result[i * B->cols + j] = 0;
            for (int k = 0; k < A->cols; k++) {
                result[i * B->cols + j] += A->data[i * A->cols + k] * B->data[k * B->cols + j];
            }
        }
    }

    lval* mult = arena_alloc(global_arena, sizeof(lval));
    mult->type = LVAL_MAT;
    mult->rows = A->rows;
    mult->cols = B->cols;
    mult->num = mult->rows * mult->cols;
    mult->data = result;

    return mult;
}

lval* builtin_dotproduct(lenv* e, lval* a) {
    LASSERT_NUM("dot-product", a, 2);
    LASSERT_TYPE("dot-product", a, 0, LVAL_VEC);
    LASSERT_TYPE("dot-product", a, 1, LVAL_VEC);
    LASSERT(a, (a->cell[0]->num == a->cell[1]->num), "Vector 1 and 2 sizes should match.");

    return lval_sexpr();

}

// lval* builtin_time(lenv* e, lval* a) {
    
// }

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
    lenv_add_builtin(e, "matrix", builtin_matrix);
    lenv_add_builtin(e, "size?", builtin_size);
    lenv_add_builtin(e, "mat-mult", builtin_matmult);
    lenv_add_builtin(e, "get", builtin_get);
    lenv_add_builtin(e, "length", builtin_length);
    lenv_add_builtin(e, "string?", builtin_str);
    lenv_add_builtin(e, "integer?", builtin_int);
    lenv_add_builtin(e, "qexpr?", builtin_qexpr);
    // lenv_add_builtin(e, "time", builtin_time);
    
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
