#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "builtin.h"
#include "parse.h"

/*******************************************
 * Set to 1 to view the command line parse *
 * Set to 0 before submitting!             *
 *******************************************/
#define DEBUG_PARSE 0


void print_banner()
{
    printf ("                    ________   \n");
    printf ("_________________________  /_  \n");
    printf ("___  __ \\_  ___/_  ___/_  __ \\ \n");
    printf ("__  /_/ /(__  )_(__  )_  / / / \n");
    printf ("_  .___//____/ /____/ /_/ /_/  \n");
    printf ("/_/ Type 'exit' or ctrl+c to quit\n\n");
}


/* **returns** a string used to build the prompt
 * (DO NOT JUST printf() IN HERE!)
 *
 * Note:
 *   If you modify this function to return a string on the heap,
 *   be sure to free() it later when appropirate!  */
static char *build_prompt()
{
    char *prompt = "$ ";
    char *ps1 = getcwd(NULL, 0);
    if(ps1 == NULL){
        perror("Failed to allocate memory and get cwd");
        exit(EXIT_FAILURE);
    }
    ps1 = realloc(ps1, strlen(ps1) + strlen(prompt) + 1);
    if(!strcat(ps1, prompt)){
        perror("Failed to concatenate the prompt");
        exit(EXIT_FAILURE);
    }
    return ps1;
}


/* return true if command is found, either:
 *   - a valid fully qualified path was supplied to an existing file
 *   - the executable file was found in the system's PATH
 * false is returned otherwise */
static int command_found(const char *cmd)
{
    char *dir;
    char *tmp;
    char *PATH;
    char *state;
    char probe[PATH_MAX];

    int ret = 0;

    if (access(cmd, X_OK) == 0)
        return 1;

    PATH = strdup(getenv("PATH"));

    for (tmp=PATH; ; tmp=NULL) {
        dir = strtok_r(tmp, ":", &state);
        if (!dir)
            break;

        strncpy(probe, dir, PATH_MAX-1);
        strncat(probe, "/", PATH_MAX-1);
        strncat(probe, cmd, PATH_MAX-1);

        if (access(probe, X_OK) == 0) {
            ret = 1;
            break;
        }
    }

    free(PATH);
    return ret;
}


/* Called upon receiving a successful parse.
 * This function is responsible for cycling through the
 * tasks, and forking, executing, etc as necessary to get
 * the job done! */
void execute_tasks(Parse *P)
{
    unsigned int t;
    pid_t chpid;
    int fdi;
    int fdo;
    int (*pipes)[2];

    pipes = malloc((P->ntasks - 1) * sizeof(int[2]));
    if(!pipes){
        perror("Failed to allocate memory for array of pipes");
        exit(EXIT_FAILURE);
    }
    for (t = 0; t < P->ntasks; t++) {
        if(t < P->ntasks - 1){
            if(pipe(pipes[t]) == -1){
                perror("Failed to create pipe");
                exit(EXIT_FAILURE);
            }
        }
        // Special exception for exit and cd which must not be executed in child process
        if(!strcmp(P->tasks[t].cmd, "cd") || !strcmp(P->tasks[t].cmd, "exit")){
            builtin_execute(P->tasks[t]);
            break;
        }
        chpid = fork();
        switch(chpid){
            case -1:
                perror("Failed to vfork");
                exit(EXIT_FAILURE);
                break;
            case 0:
                // in redir
                if(t == 0){
                    if(P->infile){
                        fdi = open(P->infile, O_RDONLY);
                        if(fdi == -1){
                            perror("Could not open input file for redirection");
                            exit(EXIT_FAILURE);
                        }
                        if(dup2(fdi, STDIN_FILENO) == -1){
                            perror("Failed to duplicate input file to STDIN");
                            exit(EXIT_FAILURE);
                        }
                        if(close(fdi) == -1){
                            perror("Failed to close old file descriptor for file input");
                            exit(EXIT_SUCCESS);
                        }
                    }
                }else{
                    if(dup2(pipes[t-1][0], STDIN_FILENO) == -1){
                        perror("Failed to duplicate pipe read end to STDIN");
                        exit(EXIT_FAILURE);
                    }
                    if(close(pipes[t-1][0]) == -1){
                        perror("Failed to close old file descriptor for pipe read end");
                        exit(EXIT_SUCCESS);
                    }
                    if(close(pipes[t-1][1]) == -1){
                        perror("Failed to close unused file descriptor for pipe write end");
                        exit(EXIT_SUCCESS);
                    }
                }
                // out redir
                if(t == P->ntasks - 1){
                    if(P->outfile){
                        fdo = open(P->outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        if(fdo < 0){
                            perror("Could not open output file for redirection");
                            exit(EXIT_FAILURE);
                        }
                        if(dup2(fdo, STDOUT_FILENO) == -1){
                            perror("Failed to duplicate output file to STDOUT\n");
                            exit(EXIT_FAILURE);
                        }
                        if(close(fdo) == -1){
                            perror("Failed to close old file descriptor for file output");
                            exit(EXIT_SUCCESS);
                        }
                    }
                }else{
                    if(dup2(pipes[t][1], STDOUT_FILENO) == -1){
                        perror("Could not duplicate pipe write end to STDOUT\n");
                        exit(EXIT_FAILURE);
                    }
                    if(close(pipes[t][1]) == -1){
                        perror("Failed to close old file descriptor for pipe write end");
                        exit(EXIT_SUCCESS);
                    }
                    if(close(pipes[t][0]) == -1){
                        perror("Failed to close unused file descriptor for pipe read end");
                        exit(EXIT_SUCCESS);
                    }
                }

                if (is_builtin(P->tasks[t].cmd)) {
                    builtin_execute(P->tasks[t]);
                    P->tasks[t].pid = -1;
                    exit(EXIT_SUCCESS);
                }
                else if (command_found(P->tasks[t].cmd)) {
                    if(execvp(P->tasks[t].cmd, P->tasks[t].argv) == -1) {
                        perror("Failed to execute");
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    printf("pssh: command not found: %s\n", P->tasks[t].cmd);
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                P->tasks[t].pid = chpid;
                if(t != 0){
                    close(pipes[t-1][0]);
                    close(pipes[t-1][1]);
                }
                break;
        } 
    }
    // Wait on all child processes
    if(!P->background){
        for(t = 0; t < P->ntasks; t++){
            if(P->tasks[t].pid != -1){
                waitpid(P->tasks[t].pid, NULL, 0);
            }
        }
    }
    free(pipes);
}


int main(int argc, char **argv)
{
    char *ps1;
    char *cmdline;
    Parse *P;

    print_banner();

    while (1) {
        ps1 = build_prompt();
        /* do NOT replace readline() with scanf() or anything else! */
        cmdline = readline(ps1);
        if (!cmdline)       /* EOF (ex: ctrl-d) */
            exit(EXIT_SUCCESS);

        P = parse_cmdline(cmdline);
        if (!P)
            goto next;

        if (P->invalid_syntax) {
            printf("pssh: invalid syntax\n");
            goto next;
        }

#if DEBUG_PARSE
        parse_debug(P);
#endif

        execute_tasks(P);

    next:
        parse_destroy(&P);
        free(cmdline);
        free(ps1);
    }
}
