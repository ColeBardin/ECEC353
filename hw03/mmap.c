#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define MAP_SIZE 32

int main(){
    int fd;
    int *addr;
    pid_t chpid;

    fd = open("/dev/zero", O_RDWR);
    if(fd == -1){
        fputs("ERROR: Failed to open /dev/zero", stderr);
        exit(EXIT_FAILURE);
    }

    addr = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(addr == MAP_FAILED){
        fputs("ERROR: mmap failed to reserve memory", stderr);
        exit(EXIT_FAILURE);
    }
    if(close(fd) < 0){
        perror("Failed to close /dev/zero fd");
        if(munmap(addr, MAP_SIZE) < 0) perror("Failed to munmap");
        exit(EXIT_FAILURE);
    }
    
    chpid = fork();
    if(chpid < 0){
        fputs("ERROR: fork failed to create a new process", stderr);
        exit(EXIT_FAILURE);
    }else if(chpid > 0){
        int i;
        while(!addr[0]);
        printf("P: Found special number that child put in shared memory: %d\n", addr[2]);
        wait(NULL);
        if(munmap(addr, MAP_SIZE) < 0){
            perror("Child failed to munmap");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }else{
        int i;
        int special_number = 39;
        for(i = 1; i < MAP_SIZE; i++) addr[i] = special_number;
        addr[0] = 1;
        if(munmap(addr, MAP_SIZE) < 0){
            perror("Child failed to munmap");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
}

