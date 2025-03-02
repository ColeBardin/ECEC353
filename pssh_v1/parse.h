#ifndef PARSE_H
#define PARSE_H

#include <limits.h>

typedef struct {
    char *cmd;
    char **argv;   /* NULL terminated array of strings */
    pid_t pid;     /* PID assigned to this task with execvp */
} Task;

typedef struct {
    Task *tasks;         /* ordered list of tasks to pipe */
    int   ntasks;        /* # of tasks in the parse */

    char *infile;        /* filename of 'infile'  */
    char *outfile;       /* filename of 'outfile' */

    int background;      /* run process in background? */
    int invalid_syntax;  /* parse failed */
} Parse;


Parse *parse_cmdline(char *cmdline);
void parse_destroy(Parse **P);
void parse_debug(Parse *P);

#endif
