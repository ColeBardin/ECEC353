#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "builtin.h"
#include "parse.h"
#include "job.h"

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

        if(chdir(p) < 0) perror("pssh: failed to cd");
    } else if(!strcmp(T.cmd, "jobs")){
        print_all_bg_jobs();
        return;
    } else if(!strcmp(T.cmd, "fg")){
        int bgid;
        int pgid;

        if(T.argv[1] == NULL)
        {
            printf("Usage: fg %%<jobnumber>\n");
            return;
        }
        bgid = atoi(&T.argv[1][1]);
        if(T.argv[1][0] != '%' || bgid < 0 || (pgid = get_job_pgid(bgid)) < 0)
        {
            printf("pssh: invalid job number: %s\n", T.argv[1]);
            return;
        }
        set_job_stat(bgid, FG);

        set_fg_pgrp(pgid);
        kill(-1 * pgid, SIGCONT);
        return;
    } else if(!strcmp(T.cmd, "bg")){
        int bgid;
        int pgid;

        if(T.argv[1] == NULL)
        {
            printf("Usage: bg %%<jobnumber>\n");
            return;
        }
        bgid = atoi(&T.argv[1][1]);
        if(T.argv[1][0] != '%' || bgid < 0 || (pgid = get_job_pgid(bgid)) < 0)
        {
            printf("pssh: invalid job number: %s\n", T.argv[1]);
            return;
        }

        set_job_stat(bgid, BG);
        kill(-1 * pgid, SIGCONT);
        return;
    } else if(!strcmp(T.cmd, "kill")){
        int jobn;
        pid_t pid;
        int signum;
        int i;

        i = 1;
        jobn = -1;
        signum = SIGTERM;

        // No args
        if(T.argv[1] == NULL)
        {
            printf("Usage: kill [-s <signal>] <pid> | %%<job> ...\n");
            return;
        }
        // first arg is -s
        if(T.argv[1][0] == '-' && T.argv[1][1] == 's')
        {
            // missing <signal>
            if(T.argv[2] == NULL)
            {
                printf("Usage: kill [-s <signal>] <pid> | %%<job> ...\n");
                return;
            }
            signum = atoi(T.argv[2]);
            if(signum < 1 || signum > 31)
            {
                printf("pssh: invalid signal number: %s\n", T.argv[2]);
                return;
            }
            // if missing PIDs/jobns after -s <signal>
            if(T.argv[3] == NULL)
            {
                printf("Usage: kill [-s <signal>] <pid> | %%<job> ...\n");
                return;
            }
            i = 3;
        }

        while(1)
        {
            if(T.argv[i] == NULL) return;
            if(T.argv[i][0] == '%')
            {
                jobn = atoi(&T.argv[i][1]);
                int pgid;
                pgid = get_job_pgid(jobn);
                if(pgid < 0)
                {
                    printf("pssh: invalid job number: %s\n", T.argv[i]);
                    return;
                }
                kill(-1 * pgid, signum);
            }
            else
            {
                pid = atoi(T.argv[i]);
                if(pid < 0)
                {
                    printf("pssh: invalid pid: %s\n", T.argv[i]);
                    return;
                }
                kill(pid, signum);
            }
            i++;
        }
        return;
    } else {
        printf("pssh: builtin command: %s (not implemented!)\n", T.cmd);
    }
}



