Project 1
Cole Bardin
14422557

How To Compile & Run
====================
To build the program, run:

$ make

To remove executables and object files, run:

$ make clean

The program can be run with:

$ ./pssh

Description
===========
Pretty Simple Shell is standalone shell that can perform the following operations:

1. Display the current working directory within the prompt

2. Run commands with optional input and output redirection as well as with command line arguments

3. Run multiple pipelined commands with optional intput and output redirection.

4. Implements builtin commands such as exit, which, and cd

This shell will fork and exec arbitrary builtin and arbitrary programs. With the exception of cd and exit, which must not fork from the parent process.
