#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "job.h"

Job jobs[MAX_JOBS];
Job *bg_jobs[MAX_JOBS];

int add_job(Parse *P, JobStatus status)
{
    int job;
    Job *cur;
    int len;

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
            cur->status = status;
            cur->bg_id = -1;

            //printf("DEBUG: Created main job for %s, job#%d (%d)\n", cur->name, job, cur->pgid);
            return job;
        }
    }
    fprintf(stderr, "add_job: out of available job numbers\n");
    return OUT_OF_JOBS;
}

int delete_job(int jobn)
{
    Job *cur;

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

    //printf("DEBUG: deleted main job %d for %d(%s)\n", jobn, cur->pgid, cur->name);
    //if(bg_jobs[cur->bg_id]->pgid == cur->pgid) bg_jobs[cur->bg_id] = NULL;
    free(cur->name);
    cur->name = NULL;
    free(cur->pids);
    cur->pids = NULL;
    cur->npids = 0;
    cur->pgid = 0;

    return 0;
}

int find_job(pid_t pid)
{
    int i, j;

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

JobStatus get_status_e(int jobn)
{
    Job *cur;

    if(jobn < 0 || jobn >= MAX_JOBS)
    {
        fprintf(stderr, "get_stats_e: job index out of range\n");
        return JOB_NOT_FOUND;
    }
    cur = &jobs[jobn];
    if(cur->name == NULL)
    {
        fprintf(stderr, "get_status_e: job index is not a job\n");
        return JOB_NOT_FOUND;
    }
    return cur->status;
}

int bg_job(int jobn)
{
    Job *cur;
    int i;

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
            cur->bg_id = i;
            return i;
        }
    }
    return OUT_OF_JOBS;
}

int suspend_job(int jobn)
{
    Job *cur;
    int bgid;

    if(jobn < 0 || jobn >= MAX_JOBS)
    {
        fprintf(stderr, "suspend_job: job index out of range\n");
        return JOB_NOT_FOUND;
    }
    cur = &jobs[jobn];
    if(cur->name == NULL)
    {
        fprintf(stderr, "suspend_job: job index is not a job\n");
        return JOB_NOT_FOUND;
    }
    bgid = cur->bg_id;
    if(bgid < 0)
    {
        if(bg_job(jobn) < 0)
        {
            fprintf(stderr, "suspend_job: failed to bg job for suspension\n");
            return JOB_ERROR;
        }
        bgid = cur->bg_id;
    }
    if(cur->status != STOPPED)
    {
        cur->status = STOPPED;
        printf("[%d] + suspended\t %s\n", bgid, cur->name);
    }

    return 0;
}

int terminate_job(int jobn)
{
    Job *cur;

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
    
    //printf("DEBUG: removed main job %d (%s)\n", jobn, cur->name);
    cur->status = TERM;
    return 0;
}

int bg_job_remove(int bgid)
{
    Job *cur;

    if(bgid < 0 || bgid >= MAX_JOBS)
    {
        fprintf(stderr, "bg_job_remove: bgid is not valid\n");
        return JOB_NOT_FOUND;
    }
    cur = bg_jobs[bgid];
    if(cur->name == NULL)
    {
        fprintf(stderr, "bg_job_remove: bgid %d is not valid\n", bgid);
        return JOB_NOT_FOUND;
    }

    if(cur->status != TERM) printf("[%d] done\t %s\n", bgid, cur->name);
    bg_jobs[bgid] = NULL;
    return 0;
}

int add_pid_to_job(int jobn, pid_t pid, int task)
{
    Job *cur;

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
    //printf("DEBUG: set pid[%d] to %d for job %d\n", task, pid, jobn);

    return 0;
}

int kill_job(int jobn)
{
    Job *cur;

    if(jobn < 0 || jobn >= MAX_JOBS)
    {
        fprintf(stderr, "kill_job: job index out of range\n");
        return JOB_NOT_FOUND;
    }
    cur = &jobs[jobn];
    if(cur->name == NULL)
    {
        fprintf(stderr, "kill_job: job index is not a job\n");
        return JOB_NOT_FOUND;
    }
    cur->status = TERM;
    printf("[%d] + killed\t %s\n", cur->bg_id, cur->name);

    return 0;
}

int pid_term_job(pid_t pid, int jobn)
{
    int i;
    Job *cur;

    if(jobn < 0 || jobn >= MAX_JOBS)
    {
        fprintf(stderr, "pid_term_job: job index out of range\n");
        return JOB_NOT_FOUND;
    }
    cur = &jobs[jobn];
    if(cur->name == NULL)
    {
        fprintf(stderr, "pid_term_job: job index is not a job\n");
        return JOB_NOT_FOUND;
    }
    for(i = 0; i < cur->npids; i++)
    {
        if(cur->pids[i] == pid)
        {
            cur->pids[i] = -1;
            break;
        }
    }
    if(i == cur->npids)
    {
        fprintf(stderr, "pid_term_job: pid %d is not found in job %d\n", pid, jobn); 
        return PID_NOT_FOUND;
    }
    // Check for still running pids
    for(i = 0; i < cur->npids; i++) if(cur->pids[i] != -1) return 0;
    //printf("DEBUG: no more PIDs running for main job %d (bg %d)\n", jobn, cur->bg_id);
    // No running PIDs
    if(cur->bg_id > -1) bg_job_remove(cur->bg_id);
    delete_job(jobn);
    return JOB_DONE;
}

int print_bg_job(int jobn)
{
    int i;
    Job *cur;

    if(jobn < 0 || jobn >= MAX_JOBS)
    {
        fprintf(stderr, "print_bg_job: job index out of range\n");
        return JOB_NOT_FOUND;
    }
    cur = &jobs[jobn];
    if(cur->name == NULL)
    {
        fprintf(stderr, "print_bg_job: job index is not a job\n");
        return JOB_NOT_FOUND;
    }

    printf("[%d]", cur->bg_id);
    for(i = 0; i < cur->npids; i++) printf(" %d", cur->pids[i]);
    printf("\n");
    return 0;
}

int continue_job(int jobn)
{
    Job *cur;

    if(jobn < 0 || jobn >= MAX_JOBS)
    {
        fprintf(stderr, "continue_job: job index out of range\n");
        return JOB_NOT_FOUND;
    }
    cur = &jobs[jobn];
    if(cur->name == NULL)
    {
        fprintf(stderr, "continue_job: job index is not a job\n");
        return JOB_NOT_FOUND;
    }

    if(cur->status != BG)
    {
        cur->status = BG;
        printf("[%d] + continued\t %s\n", cur->bg_id, cur->name);
    }

    return 0;
}

void print_all_bg_jobs()
{
    Job *cur;
    int i;

    for(i = 0; i < MAX_JOBS; i++)
    {
        cur = bg_jobs[i];
        if(cur->name != NULL) printf("[%d] + %s\t %s\n", i, get_status(cur->status), cur->name);
    }

    return;
}

int get_job_pgid(int jobn)
{
    if(jobn < 0 || jobn >= MAX_JOBS || bg_jobs[jobn]->name == NULL) return JOB_NOT_FOUND;
    else return bg_jobs[jobn]->pgid;
}

int get_bgid(int jobn)
{
    Job *cur;

    if(jobn < 0 || jobn >= MAX_JOBS)
    {
        fprintf(stderr, "get_bgid: job index out of range\n");
        return JOB_NOT_FOUND;
    }
    cur = &jobs[jobn];
    if(cur->name == NULL)
    {
        fprintf(stderr, "get_bgid: job index is not a job\n");
        return JOB_NOT_FOUND;
    }
    return cur->bg_id;
}

pid_t *get_pids(int jobn, int *npids)
{
    Job *cur;

    if(jobn < 0 || jobn >= MAX_JOBS)
    {
        fprintf(stderr, "get_pids: job index out of range\n");
        return NULL;
    }
    cur = &jobs[jobn];
    if(cur->name == NULL)
    {
        fprintf(stderr, "get_pids: job index is not a job\n");
        return NULL;
    }
    if(npids) *npids = cur->npids;
    return cur->pids;
}


