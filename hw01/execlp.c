/* EXECLP */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>

int main(){
    pid_t chpid;

    chpid = vfork();
    if(chpid < 0){
        fputs("ERROR: vfork() call failed to create child process", stderr);
        return EXIT_FAILURE;
    }else if(chpid > 0){
        // Parent
        int child_stat;
        waitpid(chpid, &child_stat, 0);
        printf("Child exited with return code %d\n", child_stat);
        return EXIT_SUCCESS;
    }else{
        // Child
        int ret;
        ret = execlp("ls", "ls", "-lh", NULL);
        if(ret == -1){
            perror("execlp failed");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
}
