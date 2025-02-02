#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define USAGE fprintf(stderr, "%s [option] <pid>\n", argv[0]); \
fprintf(stderr, "\nOptions:\n"); \
fprintf(stderr, "\t-s <signal>  Sends <signal> to <pid>\n"); \
fprintf(stderr, "\t-l           Lists all signal numbers with their names\n");

const char *sigabbrev(unsigned int sig);

int main(int argc, char *argv[]){
    int pid;
    int signum = SIGTERM;
    int ret;

    if(argc == 1){
        USAGE
        exit(EXIT_FAILURE);
    }

    if(!strcmp(argv[1], "-l")){
        for(int i = 1; i < 32; i++) printf("%2d) SIG%-15s %s\n", i, sigabbrev(i), strsignal(i));
        exit(EXIT_SUCCESS);
    }else if(!strcmp(argv[1], "-s")){
        if(argc != 4){
            fprintf(stderr, "ERROR: incorrect number of arguments with -s flag\n");
            USAGE
            exit(EXIT_FAILURE);
        }  
        signum = atoi(argv[2]);
        if(signum < 0 || signum > 31){
            fprintf(stderr, "ERROR: failed to parse '%s' into valid signal number\n", argv[2]);
            exit(EXIT_FAILURE);
        }
        pid = atoi(argv[3]);
        if(pid < 1){
            fprintf(stderr, "ERROR: failed to parse '%s' into valid PID number>\n", argv[3]);
            exit(EXIT_FAILURE);
        }
        
        if(signum == 0){
            ret = kill(pid, signum);
            if(ret == 0){
                printf("PID %d exists and is able to receive signals\n", pid);
                exit(EXIT_SUCCESS);
            }else if(ret == -1){
                if(errno == EPERM){
                    printf("PID %d exists, but we cannot send it signals\n", pid);
                }else if(errno == ESRCH){
                    printf("PID %d does not exist\n", pid);
                }
                exit(EXIT_SUCCESS);
            }else{
                fprintf(stderr, "ERROR: Unexpected return value from kill: %d\n", ret);
                exit(EXIT_FAILURE);
            }
        }
    }else{
        if(argc != 2){
            fprintf(stderr, "ERROR: Incorrect nuber of arguments\n");
            USAGE
            exit(EXIT_FAILURE);
        }
        pid = atoi(argv[1]);
        if(pid < 1){
            fprintf(stderr, "ERROR: failed to parse '%s' into valid PID\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    ret = kill(pid, signum);
    if(ret < 0){
        perror("Kill returned an error"); 
        exit(EXIT_FAILURE);
    }
}

const char *sigabbrev(unsigned int sig){
    const char *sigs[31] = { "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT",
    "BUS", "FPE", "KILL", "USR1", "SEGV", "USR2", "PIPE", "ALRM",
    "TERM", "STKFLT", "CHLD", "CONT", "STOP", "TSTP", "TTIN",
    "TTOU", "URG", "XCPU", "XFSZ", "VTALRM", "PROF", "WINCH", "IO",
    "PWR", "SYS" };
    if (sig == 0 || sig > 31) return NULL;
    return sigs[sig-1];
}

