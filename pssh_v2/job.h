#ifndef _JOB_H
#define _JOB_H

#include <stdbool.h>
#include "parse.h"

#define MAX_JOBS 100
#define BAD_JOB_LIST -1
#define JOB_NOT_FOUND -2
#define OUT_OF_JOBS -3

typedef enum {
    STOPPED,
    TERM,
    BG,
    FG,
} JobStatus;

typedef struct {
    char* name;
    pid_t* pids;
    unsigned int npids;
    pid_t pgid;
    JobStatus status;
} Job;

int add_job(Job *, Parse *, JobStatus);
int delete_job(Job *, int);
int find_job(Job *, pid_t);
char *get_status(JobStatus);
int bg_job(Job *, Job **, int);
int suspend_job(Job **, int);
int terminate_job(Job *, int);
int bg_job_remove(Job **, pid_t);
int add_pid_to_job(Job *, int, pid_t, int);

#endif
