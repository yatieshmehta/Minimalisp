#ifndef UTILS_H
#define UTILS_H

#ifdef _WIN32
char* readline(char* prompt);
void add_history(char* unused);
#endif

double now_sec();

#endif