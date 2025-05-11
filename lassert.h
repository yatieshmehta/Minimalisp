#ifndef LASSERT_H
#define LASSERT_H

#include "lval.h"
#include <stdio.h>

#define LASSERT(args, cond, fmt, ...) \
    if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
    LASSERT(args, args->cell[index]->type == expect, \
        "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
        func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, number) \
    LASSERT(args, args->num == number, \
        "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
        func, args->num, number)

#define LASSERT_NOT_EMPTY(func, args, index) \
    LASSERT(args, args->cell[index]->num != 0, \
        "Function '%s' passed {} for argument %i.", func, index);

#endif
