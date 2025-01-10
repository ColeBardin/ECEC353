/* EXECVP */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

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
        char *vargs[] = {"ls", "-lh", NULL};
        ret = execvp("ls", vargs);
        if(ret == -1){
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
}
