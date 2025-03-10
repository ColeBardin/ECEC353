#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "builtin.h"
#include "parse.h"
#include "job.h"

/*******************************************
 * Set to 1 to view the command line parse *
 * Set to 0 before submitting!             *
 *******************************************/
#define DEBUG_PARSE 0

void set_infile(int t, char *filename);
void set_outfile(int t, char *filename, int ntasks);
void handler(int sig);

int (*pipes)[2];

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
    int jobn;
    sigset_t no_sigchld, old_mask;
    sigemptyset(&no_sigchld);
    sigaddset(&no_sigchld, SIGCHLD);

    jobn = add_job(P, FG);
    if(jobn < 0) exit(EXIT_FAILURE);
    if(P->background) bg_job(jobn);

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
        if(!strcmp(P->tasks[t].cmd, "cd") 
           || !strcmp(P->tasks[t].cmd, "exit")
           || !strcmp(P->tasks[t].cmd, "fg")
          )
        {
            builtin_execute(P->tasks[t]);
            delete_job(jobn);
            continue;
        }


        sigprocmask(SIG_BLOCK, &no_sigchld, &old_mask);
        P->tasks[t].pid = fork();
        setpgid(P->tasks[t].pid, P->tasks[0].pid);
        if(!P->background) set_fg_pgrp(P->tasks[0].pid);

        switch(P->tasks[t].pid){
        case -1:
            perror("Failed to fork");
            exit(EXIT_FAILURE);
            break;
        case 0:
            set_infile(t, P->infile);
            set_outfile(t, P->outfile, P->ntasks);

            if (is_builtin(P->tasks[t].cmd)) {
                builtin_execute(P->tasks[t]);
                exit(EXIT_SUCCESS);
            }
            else if (command_found(P->tasks[t].cmd)) {
                if(execvp(P->tasks[t].cmd, P->tasks[t].argv) == -1) {
                    perror("Failed to execute program");
                    exit(EXIT_FAILURE);
                }
            }
            else {
                printf("pssh: command not found: %s\n", P->tasks[t].cmd);
                exit(EXIT_FAILURE);
            }
            break;
        default:
            add_pid_to_job(jobn, P->tasks[t].pid, t);
            if(t == 0){
                if(!P->background) set_fg_pgrp(P->tasks[t].pid);
            }else{
                close(pipes[t-1][0]);
                close(pipes[t-1][1]);
            }
            break;
        } 
        if(t == P->ntasks - 1 && P->background) print_bg_job(jobn);
        sigprocmask(SIG_SETMASK, &old_mask, NULL);
    }

    free(pipes);
}

int main(int argc, char **argv)
{
    char *ps1;
    char *cmdline;
    Parse *P;

    print_banner();
    signal(SIGTTOU, handler);
    signal(SIGCHLD, handler);

    while (1) {
        /*
        while(tcgetpgrp(STDOUT_FILENO) != getpgrp())
            pause();
            */
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

void set_infile(int t, char *filename)
{
    int fdi;
    if(t == 0){
        if(filename){
            fdi = open(filename, O_RDONLY);
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
}

void set_outfile(int t, char *filename, int ntasks)
{
    int fdo;
    if(t == ntasks - 1){
        if(filename){
            fdo = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
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
}

void handler(int sig)
{
    pid_t chld;
    int status;
    int jobn;

    switch(sig) {
    case SIGTTOU:
        while(tcgetpgrp(STDOUT_FILENO) != getpgrp())
            pause();
        break;
    case SIGCHLD:
        while( (chld = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0 ) {
            jobn = find_job(chld);
            if(jobn < 0)
            {
                fprintf(stderr, "pssh: SIGCHLD failed to find job (%d) for chld %d\n", jobn, chld);
                exit(EXIT_FAILURE);
            }

            if (WIFCONTINUED(status)) {
                //printf("CONTINUED jobn %d\n", jobn);
                continue_job(jobn);
            }else if (WIFSTOPPED(status)) {
                set_fg_pgrp(0);
                printf("STOPPED jobn %d\n", jobn);
                suspend_job(jobn);
            } else if (WIFEXITED(status)) {
                set_fg_pgrp(0);
                pid_term_job(chld, jobn);
                //printf("SIGCHLD: W IF EXITED\n");
            }else{
                if(tcgetpgrp(STDOUT_FILENO) != getpgrp()){
                    //printf("SIGCHLD: FG task exited\n");
                    if(pid_term_job(chld, jobn) == JOB_DONE) set_fg_pgrp(0);
                }else{
                    WTERMSIG(status);
                    //printf("SIGCHLD: signal stat %d\n", status);
                    switch(status)
                    {
                    case 0:
                        break;
                    case SIGKILL:
                        kill_job(jobn); 
                        break;
                    case SIGTERM:
                        //jobs[jobn].status = TERM;
                        break;
                    default:
                        printf("pssh: SIGCLD exited with some other signal status: %d\n", status);
                        break;
                    }
                    pid_term_job(chld, jobn);
                }
            }
        }
        break;
    default:
        break;
    }
}

