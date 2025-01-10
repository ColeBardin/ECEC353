Homework 1
Cole Bardin
14422557

How To Compile & Run
====================
$ gcc -o execlp execlp.c
$ ./execlp

$ gcc -o execvp execvp.c
$ ./execvp


Description
===========
This assignment creates the same program with two versions of the exec function. One uses execlp and the other uses execvp. Both programs accomplish the same task. 

The parent process creates a child process and waits for it to return. The child process will call its respective form of exec to execute 'ls -lh'. The child process will then return.

Once the child returns, the parent awakes and prints out the return value of the child.

Then the program is over.

