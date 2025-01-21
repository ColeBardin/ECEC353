Homework 2
Cole Bardin
14422557

How To Compile & Run
====================
To build the program, run:

$ cc -o pipes pipes.c

The program can be run with:

$ ./pipes

Description
===========
This program demonstrates how to use pipes to send data back and forth between a parent and child process. It creates two pipes, Parent In, Child Out (PICO) and Parent Out, Child In (POCI).

The parent process reads from STDIN and sends the characters to the child process via the POCI pipe. The child process reads from the POCI pipe. If the child receives a letter, it will uppercase it. Then it sends the character back to the parent via the PICO pipe. The parent reads the character from the PICO pipe and then writes the returned character to STDOUT.

All command line arguments will be ignored.
