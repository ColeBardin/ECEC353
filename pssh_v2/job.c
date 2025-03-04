#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "job.h"

int add_job(Job *jobs, Parse *P, JobStatus status)
{
    int job;
    Job *cur;
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
            /*
            for(i = 0; i < P->ntasks; i++) cur->pids[i] = P->tasks[i].pid;
            cur->pgid = cur->pids[0];
            */
            cur->status = status;

            printf("DEBUG: Created main job for %s, job#%d (%d)\n", cur->name, job, cur->pgid);
            return job;
        }
    }
    fprintf(stderr, "add_job: out of available job numbers\n");
    return OUT_OF_JOBS;
}

int delete_job(Job * jobs, int jobn)
{
    Job *cur;

    if(!jobs)
    {
        fprintf(stderr, "delete_job: bad job array\n");
        return BAD_JOB_LIST;
    }
    if(jobn < 0 || jobn >= MAX_JOBS)
    {
        fprintf(stderr, "delete_job: job index out of range\n");
        return JOB_NOT_FOUND;
    }

    cur = &jobs[jobn];
    if(cur->name == NULL)
    {
        fprintf(stderr, "delete_job: job index is not a job\n");
        return JOB_NOT_FOUND;
    }

    printf("DEBUG: deleted main job %d for %d(%s)\n", jobn, cur->pgid, cur->name);
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
    if(!jobs)
    {
        fprintf(stderr, "find_job: bad job array\n");
        return BAD_JOB_LIST;
    }
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

int bg_job(Job *jobs, Job **bg_jobs, int jobn)
{
    Job *cur;
    int i, t;

    if(!jobs)
    {
        fprintf(stderr, "bg_job: bad jobs array\n");
        return BAD_JOB_LIST;
    }
    if(!bg_jobs)
    {
        fprintf(stderr, "bg_job: bad bg_jobs array\n");
        return BAD_JOB_LIST;
    }
    if(jobn < 0 || jobn >= MAX_JOBS)
    {
        fprintf(stderr, "bg_job: job index out of range\n");
        return JOB_NOT_FOUND;
    }
    cur = &jobs[jobn];
    if(cur->name == NULL)
    {
        fprintf(stderr, "bg_job: job index is not a job\n");
        return JOB_NOT_FOUND;
    }
    for(i = 0; i < MAX_JOBS; i++)
    {
        if(bg_jobs[i] == NULL)
        {
            bg_jobs[i] = cur;
            cur->status = BG;
            printf("[%d]", i);
            for(t = 0; t < cur->npids; t++) printf(" %d", cur->pids[t]);
            printf("\n");
            return i;
        }
    }
    return OUT_OF_JOBS;
}

int suspend_job(Job **bg_jobs, int jobn)
{
    Job *cur;

    if(!bg_jobs)
    {
        fprintf(stderr, "suspend_job: bad bg_jobs array\n");
        return BAD_JOB_LIST;
    }
    if(jobn < 0 || jobn >= MAX_JOBS)
    {
        fprintf(stderr, "suspend_job: job index out of range\n");
        return JOB_NOT_FOUND;
    }
    cur = bg_jobs[jobn];
    if(cur->name == NULL)
    {
        fprintf(stderr, "suspend_job: job index is not a job\n");
        return JOB_NOT_FOUND;
    }
    cur->status = STOPPED;
    printf("[%d] + suspended\t %s\n", jobn, cur->name);

    return 0;
}

int terminate_job(Job *jobs, int jobn)
{
    Job *cur;

    if(!jobs)
    {
        fprintf(stderr, "terminate_job: bad jobs array\n");
        return BAD_JOB_LIST;
    }
    if(jobn < 0 || jobn >= MAX_JOBS)
    {
        fprintf(stderr, "terminate_job: job index out of range\n");
        return JOB_NOT_FOUND;
    }
    cur = &jobs[jobn];
    if(cur->name == NULL)
    {
        fprintf(stderr, "terminate_job: job index is not a job\n");
        return JOB_NOT_FOUND;
    }
    
    printf("DEBUG: removed main job %d (%s)\n", jobn, cur->name);
    cur->status = TERM;
    return 0;
}

int bg_job_remove(Job **bg_jobs, pid_t pgid)
{
    int i;
    Job *cur;
    if(!bg_jobs)
    {
        fprintf(stderr, "suspend_job: bad bg_jobs array\n");
        return BAD_JOB_LIST;
    }
    for(i = 0; i < MAX_JOBS; i++)
    {
        cur = bg_jobs[i];
        if(cur->pgid == pgid)
        {
            bg_jobs[i] = NULL;
            printf("[%d] Done\t %s\n", i, cur->name);
            return i;
        }
    }
    return JOB_NOT_FOUND;
}

int add_pid_to_job(Job *jobs, int jobn, pid_t pid, int task)
{
    Job *cur;
    if(!jobs)
    {
        fprintf(stderr, "add_pid_to_job: bad jobs array\n");
        return BAD_JOB_LIST;
    }
    if(jobn < 0 || jobn >= MAX_JOBS)
    {
        fprintf(stderr, "add_pid_to_job: job index out of range\n");
        return JOB_NOT_FOUND;
    }
    cur = &jobs[jobn];
    if(cur->name == NULL)
    {
        fprintf(stderr, "add_pid_to_job: job index is not a job\n");
        return JOB_NOT_FOUND;
    }
    cur->pids[task] = pid;
    if(task == 0) cur->pgid = pid;
    printf("DEBUG: set pid[%d] to %d for job %d\n", task, pid, jobn);

    return 0;
}





