Homework 3
Cole Bardin
14422557

How To Compile & Run
====================
To build the program, run:

$ cc -o mmap mmap.c

The program can be run with:

$ ./mmap

Description
===========
This program is very simple. It just demonstrates using mmap the System-V way to create shared anonymous memory. It reserves some memory with mmap, then forks. The child process puts a special number in the memory except for address 0. Once it is done modifying memory, it sets address 0 to a nonzero value. This signals to the parent process that the 'work' is done. The parent process reads the special value. Then the processes and program terminates.
