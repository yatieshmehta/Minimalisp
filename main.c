#include "mpc.h"
#include "lval.h"
#include "lenv.h"
#include "lassert.h"
#include "builtin.h"
#include "eval.h"
#include "parser.h"
#include "arena.h"
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>

static char buffer[2048];

char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

void add_history(char* unused) {}

double now_sec() {
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);

    return (double)counter.QuadPart / frequency.QuadPart;
}

#else
#include <editline/readline.h>
#include <editline/history.h>

// Add code for Ubuntu now_sec()

#endif

arena_t* global_arena;
arena_t* temp_arena;

int main(int argc, char** argv) {
    
    Number = mpc_new("number");
    Symbol = mpc_new("symbol");
    String = mpc_new("string");
    Comment = mpc_new("comment");
    Sexpr = mpc_new("sexpr");
    Qexpr = mpc_new("qexpr");
    Expr = mpc_new("expr");
    Lispy = mpc_new("lispy");
    
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                              \
            number  : /-?[0-9]+/ ;                                                     \
            symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&?]+/ ;                              \
            string  : /'\\b[\\w-]+/ ;                                                  \
            comment : /;[^\\r\\n]*/ ;                                                  \
            sexpr   : '(' <expr>* ')' ;                                                \
            qexpr   : '{' <expr>* '}' ;                                                \
            expr    : <number> | <symbol> | <string> | <comment> | <sexpr> | <qexpr> ; \
            lispy   : /^/ <expr>* /$/ ;                                                \
        ",
        Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

    global_arena = arena_create(1024 * 64);

    lenv* e = lenv_new();
    lenv_add_builtins(e);

    size_t size = sizeof(lval);
    printf("Size: %zu \n", size);

    mpc_result_t first;
    mpc_parse("<stdin>", "(func {def} (lambda {args body} {func (list (first args)) (lambda (rest args) body)}))", Lispy, &first);
    lval* func = lval_eval(e, lval_read(first.output));
    lval_del(func);
    mpc_ast_delete(first.output);
    
    if (argc == 1) {

        puts("Lispy Version 0.0.0.0.7");
        puts("Press Ctrl+c to Exit\n");    

        while (1) {
            char* input = readline("lispy> ");
            add_history(input);
            
            mpc_result_t r;
            if (mpc_parse("<stdin>", input, Lispy, &r)) {
                lval* x = lval_eval(e, lval_read(r.output));
                lval_println(x);
                lval_del(x);
                mpc_ast_delete(r.output);
            } else {        
                mpc_err_print(r.error);
                mpc_err_delete(r.error);
            }
            
            free(input);
            
        }
    } 
    if (argc >= 2) {

        bool extension = false;

        for (int i=1; i < argc; i++) {
            extension = false;
            for (int j=0; j < strlen(argv[i]); j++) {
                if (strcmp((argv[i] + j), ".minlsp") == 0) {
                    lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));
                    lval* x = builtin_load(e, args);
                    if (x->type == LVAL_ERR) { lval_println(x); }
                    lval_del(x);
                    extension = true;
                    break;
                }
            }
            if (!extension) {
                printf("Error: file '%s' is not a .minlsp file.", argv[i]);
            }
        }
    }

    lenv_del(e);
    arena_destroy(global_arena);
    printf("ARENA DESTROYED!");
    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);
    
    return 0;
}