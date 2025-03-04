#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "parse.h"
#include "job.h"

extern Job jobs[MAX_JOBS];
extern Job *bg_jobs[MAX_JOBS];

static char *builtin[] = {
    "exit",   /* exits the shell */
    "which",  /* displays full path to command */
    "cd",     /* Changes process working directory */
    "jobs",
    "fg",
    "bg",
    NULL
};

void set_fg_pgrp(pid_t pgrp)
{
    void (*sav)(int sig);

    if (pgrp == 0)
        pgrp = getpgrp();

    sav = signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(STDOUT_FILENO, pgrp);
    signal(SIGTTOU, sav);
}

int is_builtin(char *cmd)
{
    int i;

    for (i=0; builtin[i]; i++) {
        if (!strcmp(cmd, builtin[i]))
            return 1;
    }

    return 0;
}


void builtin_execute(Task T)
{
    int i;
    if (!strcmp(T.cmd, "exit")) {
        exit(EXIT_SUCCESS);
    } else if(!strcmp(T.cmd, "which")){
        if(T.argv[1] == NULL){
            fprintf(stderr, "which: which what?\n");
            return;
        }
        for(i=0; builtin[i]; i++){
            if(!strcmp(T.argv[1], builtin[i])){
                printf("%s: shell built-in command\n", T.argv[1]);
                return;
            }
        }
        char *dir;
        char *tmp;
        char *PATH;
        char *state;
        char probe[PATH_MAX];

        PATH = strdup(getenv("PATH"));

        for (tmp=PATH; ; tmp=NULL) {
            dir = strtok_r(tmp, ":", &state);
            if (!dir)
                break;

            strncpy(probe, dir, PATH_MAX-1);
            strncat(probe, "/", PATH_MAX-1);
            strncat(probe, T.argv[1], PATH_MAX-1);

            if (access(probe, X_OK) == 0) {
                puts(probe);
            }
        }

        free(PATH);
    } else if(!strcmp(T.cmd, "cd")){
        char *p;
        if(T.argv[1] == NULL) p = getenv("HOME");
        else p = T.argv[1];

        if(chdir(p) < 0) perror("Failed to cd");
    } else if(!strcmp(T.cmd, "jobs")){
        Job *cur;
        for(i = 0; i < MAX_JOBS; i++)
        {
            cur = bg_jobs[i];
            if(cur->name != NULL) printf("[%d] + %s\t %s\n", i, get_status(cur->status), cur->name);
        }
    } else if(!strcmp(T.cmd, "fg")){
        int jobn;
        if(T.argv[1] == NULL || T.argv[1][0] != '%')
        {
            fprintf(stderr, "fg: invalid argument use\n");
            fprintf(stderr, "USAGE: fg %%<job_number>\n");
            return;
        }
        jobn = atoi(&T.argv[1][1]);
        if(jobn < 0 || jobs[jobn].name == NULL){
            fprintf(stderr, "fg: invalid job number: %d\n", jobn);
            return;
        }
        set_fg_pgrp(jobs[jobn].pgid);
    } else if(!strcmp(T.cmd, "bg")){

    } else {
        printf("pssh: builtin command: %s (not implemented!)\n", T.cmd);
    }
}

