#ifndef PARSER_H
#define PARSER_H

#include "lval.h"
#include "mpc.h"

extern mpc_parser_t* Number;
extern mpc_parser_t* Symbol;
extern mpc_parser_t* String;
extern mpc_parser_t* Comment;
extern mpc_parser_t* Sexpr;
extern mpc_parser_t* Qexpr;
extern mpc_parser_t* Expr;
extern mpc_parser_t* Lispy;

lval* lval_read(mpc_ast_t* t);

#endif
