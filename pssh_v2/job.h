#ifndef _JOB_H
#define _JOB_H

#include <stdbool.h>
#include "parse.h"

#define MAX_JOBS 100
#define JOB_DONE 1
#define JOB_ERROR -1
#define BAD_JOB_LIST -2
#define JOB_NOT_FOUND -3
#define OUT_OF_JOBS -4
#define PID_NOT_FOUND -5

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
    int bg_id;
} Job;

int add_job(Parse *, JobStatus);
int delete_job(int);
int find_job(pid_t);
int find_bg_job(pid_t);
char *get_status(JobStatus);
JobStatus get_status_e(int);
int bg_job(int);
int suspend_job(int);
int terminate_job(int);
int bg_job_remove(int);
int add_pid_to_job(int, pid_t, int);
int kill_job(int);
int pid_term_job(pid_t, int);
int print_bg_job(int);
int continue_job(int);
void print_all_bg_jobs();
int get_job_pgid(int);
int get_bgid(int);
pid_t *get_pids(int, int *);

#endif
