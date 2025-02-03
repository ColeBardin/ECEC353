#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "parse.h"

static char *builtin[] = {
    "exit",   /* exits the shell */
    "which",  /* displays full path to command */
    "cd",     /* Changes process working directory */
    NULL
};


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
    if (!strcmp(T.cmd, "exit")) {
        exit(EXIT_SUCCESS);
    } else if(!strcmp(T.cmd, "which")){
        if(T.argv[1] == NULL){
            fprintf(stderr, "which: which what?\n");
            return;
        }
        for(int i=0; builtin[i]; i++){
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
    } else {
        printf("pssh: builtin command: %s (not implemented!)\n", T.cmd);
    }
}
