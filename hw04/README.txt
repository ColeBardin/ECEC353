Homework 4
Cole Bardin
14422557

How To Compile & Run
====================
To build the program, run:

$ cc -o signal signal.c

The program can be run with:

$ ./signal [option] <pid>

Options:
-s < signal>    sends <signal> to <pid>
-l              lists all signal numbers with their names

Description
===========
This program performs the function of the standard UNIX kill command. It will attempt to send a user defined signal to a given PID. The program will send SIGTERM by default when only a PID is passed.

To send a signal other than SIGTERM, the user can do so with the -s flag and pass the signal number. If the user attempts to send the signal 0, the program will check if the PID exists and the user has permission to send it signals. The user can attempt to send any signal to any process. 

If the -l flag is specified, the program will list out all the signals and their descriptions. The program exists after printing the signal list.
