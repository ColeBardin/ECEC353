#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "job.h"

int add_job(Job *jobs, Parse *P, JobStatus status)
{
    int job;
    Job *cur;
    int i;
    int len;

    if(!jobs) return BAD_JOB_LIST;

    for(job = 0; job < MAX_JOBS; job++)
    {
        cur = &jobs[job];
        if(cur->name == NULL)
        {
            len = strlen(P->tasks[0].argv[0]) + 1;
            cur->name = malloc(len);
            if(!cur->name)
            {
                perror("Failed to allocate array for job name");
                exit(EXIT_FAILURE);
            }
            strcpy(cur->name, P->tasks[0].argv[0]);
            cur->npids = P->ntasks;
            cur->pids = malloc(P->ntasks * sizeof(pid_t));
            if(!cur->pids)
            {
                perror("Failed to allocate array for job pids");
                exit(EXIT_FAILURE);
            }
            for(i = 0; i < P->ntasks; i++) cur->pids[i] = P->tasks[i].pid;
            cur->pgid = cur->pids[0];
            cur->status = status;

            printf("Created job for %s, job#%d (%d)\n", cur->name, job, cur->pgid);
            return job;
        }
    }
    fprintf(stderr, "add_job: out of available job numbers\n");
    return OUT_OF_JOBS;
}

int delete_job(Job * jobs, int jobn)
{
    if(!jobs) return BAD_JOB_LIST;
    Job *cur;

    cur = &jobs[jobn];
    if(cur->name == NULL) return JOB_NOT_FOUND;
    printf("DEBUG: deleted job %d for %d(%s)\n", jobn, cur->pgid, cur->name);
    free(cur->name);
    cur->name = NULL;
    free(cur->pids);
    cur->pids = NULL;
    cur->npids = 0;
    cur->pgid = 0;

    return 0;
}

int find_job(Job *jobs, pid_t pid)
{
    int i, j;
    if(!jobs) return BAD_JOB_LIST;
    for(i = 0; i < MAX_JOBS; i++)
    {
        for(j = 0; j < jobs[i].npids; j++)
        {
            if(jobs[i].pids[j] == pid) return i;
        }
    }
    return JOB_NOT_FOUND;
}

char *get_status(JobStatus status)
{
    static char *state_names[] = 
    {
        "stopped",
        "term",
        "running",
        "running"
    };
    return state_names[status];
}


