#include <stdio.h>
#include <unistd.h>

int main(){
    char c;
    while(read(STDIN_FILENO, &c, 1) > 0) printf("Read: %c\n", c);
    return 0;
}
