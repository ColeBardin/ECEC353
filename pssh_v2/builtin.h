#ifndef BUILTIN_H
#define BUILTIN_H

//#include <unistd.h>
//#include <stdlib.h>
#include <signal.h>
#include "parse.h"

void set_fg_pgrp(pid_t pgrp);
int is_builtin(char *cmd);
void builtin_execute(Task T);
int builtin_which(Task T);

#endif
