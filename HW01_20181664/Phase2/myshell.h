/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void echo_parseline(char **argv);
char bin_path[MAXLINE];
void pipe_parseline(char *buf, int *comm_cnt, char **command);

void sigchld_handler(int sig);

sigset_t mask, prev_mask;

