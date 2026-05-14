#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

#define MAX_JOBS 64

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} JobStatus;

typedef struct {
    int       id;              // job number shown to user: [1], [2]...
    pid_t     pid;             // process ID
    JobStatus status;          // running, stopped, or done
    char      cmd[256];        // command string e.g. "sleep 10"
} Job;

// Add a new job to the table
void job_add(pid_t pid, const char *cmd);

// Remove a job by PID
void job_remove(pid_t pid);

// Find a job by job ID number
Job *job_find_by_id(int id);

// Find a job by PID
Job *job_find_by_pid(pid_t pid);

// Print all jobs (for jobs builtin)
void job_list(void);

// Check if any background jobs have finished (call this in main loop)
void job_reap(void);

#endif
