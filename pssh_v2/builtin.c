#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

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
    "kill",
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
        if(jobn < 0 || bg_jobs[jobn]->name == NULL){
            fprintf(stderr, "fg: invalid job number: %d\n", jobn);
            return;
        }
        set_fg_pgrp(bg_jobs[jobn]->pgid);
    } else if(!strcmp(T.cmd, "bg")){

    } else if(!strcmp(T.cmd, "kill")){
        int jobn;
        pid_t pid;
        int signum;

        jobn = -1;
        signum = SIGTERM;

        if(T.argv[1] == NULL)
        {
            // no arguments
            fprintf(stderr, "kill: not enough arguments\n");
            fprintf(stderr, "USAGE: kill (<pid> | %%<jobn>) [-s <sig>]\n");
            return;
        }
        // 2 arguments
        if(T.argv[2] != NULL && T.argv[3] == NULL)
        {
            fprintf(stderr, "kill: invalid number of arguments\n");
            fprintf(stderr, "USAGE: kill (<pid> | %%<jobn>) [-s <sig>]\n");
            return;
        }
        // parse second arg as pid or jobnum
        if(T.argv[1][0] == '%')
        {
            jobn = atoi(&T.argv[1][1]);
            if(jobn < 0)
            {
                fprintf(stderr, "kill: failed to parse job number: %s\n", T.argv[1]);
                return;
            }
            if(bg_jobs[jobn]->name == NULL)
            {
                fprintf(stderr, "kill: job number %d is not a valid job\n", jobn);
                return;
            }
        }
        else
        {
            pid = atoi(T.argv[1]);
            if(pid < 0)
            {
                fprintf(stderr, "kill: failed to parse pid: %s\n", T.argv[1]);
                return;
            }
        }
        // 3 arguments
        if(T.argv[2] != NULL && T.argv[3] != NULL && T.argv[4] == NULL)
        {
            if(T.argv[2][0] == '-' && T.argv[2][1] == 's')
            {
                signum = atoi(T.argv[3]);
                if(signum < 1 || signum > 31)
                {
                    fprintf(stderr, "kill: invalid signum: %s\n", T.argv[3]);
                    return;
                }
            }
            else
            {
                fprintf(stderr, "kill: unable to parse -s flag from second arg: %s\n", T.argv[2]);
                fprintf(stderr, "USAGE: kill (<pid> | %%<jobn>) [-s <sig>]\n");
                return;
            }
        }
        if(jobn != -1)
        {
            printf("DEBUG: sending signal %d to job#%d\n", signum, jobn);
            for(i = 0; i < bg_jobs[jobn]->npids; i++) kill(bg_jobs[jobn]->pids[i], signum);
        }
        else kill(pid, signum);
        return;
    } else {
        printf("pssh: builtin command: %s (not implemented!)\n", T.cmd);
    }
}



