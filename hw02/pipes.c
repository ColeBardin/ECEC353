#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(){
    pid_t chpid;
    char c;
    ssize_t ret;
    int pico[2] = {-1};
    int poci[2] = {-1};

    if(pipe(pico) == -1){
        fputs("ERROR: failed to create PICO pipe", stderr);
        return EXIT_FAILURE;
    }
    if(pipe(poci) == -1){
        fputs("ERROR: failed to create POCI pipe", stderr);
        return EXIT_FAILURE;
    }

    chpid = fork();
    if(chpid < 0){
        fputs("ERROR: failed to fork", stderr);
        return EXIT_FAILURE;
    }else if(chpid > 0){
        // Parent
        // 1. STDIN -> c
        // 2. c -> POCI
        // 3. PICO -> c
        // 4. c -> STDOUT
        int child_stat;
        close(pico[1]);
        close(poci[0]);
        while((ret = read(STDIN_FILENO, &c, 1)) > 0){
            if(write(poci[1], &c, 1) < 1) break;
            if(read(pico[0], &c, 1) < 1) break;
            write(STDOUT_FILENO, &c, 1);
        }
        close(pico[0]);
        close(poci[1]);
        waitpid(chpid, &child_stat, 0);
        return EXIT_SUCCESS;
    }else{
        // Child
        // 1. POCI -> c
        // 2. c gets uppercase'd
        // 3. c -> PICO
        close(pico[0]);
        close(poci[1]);
        while((ret = read(poci[0], &c, 1)) > 0){
            // Uppercase
            if(c >= 'a' && c <= 'z') c -= 'a' - 'A';
            if(write(pico[1], &c, 1) < 1) break;
        }
        close(pico[1]);
        close(poci[0]);
        return EXIT_SUCCESS;
    }
}

