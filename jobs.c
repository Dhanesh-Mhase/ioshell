#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "jobs.h"

// The job table — a fixed array of job slots
static Job job_table[MAX_JOBS];
static int job_count = 0;

void job_add(pid_t pid, const char *cmd) {
    if (job_count >= MAX_JOBS) {
        fprintf(stderr, "iosh: too many jobs\n");
        return;
    }

    // Find the next available job ID
    int new_id = 1;
    for (int i = 0; i < job_count; i++)
        if (job_table[i].id >= new_id)
            new_id = job_table[i].id + 1;

    job_table[job_count].id     = new_id;
    job_table[job_count].pid    = pid;
    job_table[job_count].status = JOB_RUNNING;
    strncpy(job_table[job_count].cmd, cmd, 255);
    job_table[job_count].cmd[255] = '\0';

    printf("[%d] %d\n", new_id, pid);
    job_count++;
}

void job_remove(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (job_table[i].pid == pid) {
            // Shift remaining jobs down
            for (int j = i; j < job_count - 1; j++)
                job_table[j] = job_table[j + 1];
            job_count--;
            return;
        }
    }
}

Job *job_find_by_id(int id) {
    for (int i = 0; i < job_count; i++)
        if (job_table[i].id == id)
            return &job_table[i];
    return NULL;
}

Job *job_find_by_pid(pid_t pid) {
    for (int i = 0; i < job_count; i++)
        if (job_table[i].pid == pid)
            return &job_table[i];
    return NULL;
}

void job_list(void) {
    if (job_count == 0) return;

    for (int i = 0; i < job_count; i++) {
        const char *status_str;
        switch (job_table[i].status) {
            case JOB_RUNNING: status_str = "Running";  break;
            case JOB_STOPPED: status_str = "Stopped";  break;
            case JOB_DONE:    status_str = "Done";     break;
            default:          status_str = "Unknown";  break;
        }
        printf("[%d] %-10s %s\n",
               job_table[i].id,
               status_str,
               job_table[i].cmd);
    }
}

// Called every prompt cycle — cleans up any finished background jobs
void job_reap(void) {
    int status;
    pid_t pid;

    // WNOHANG = don't block, just check if any child finished
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        Job *j = job_find_by_pid(pid);
        if (!j) continue;

        if (WIFSTOPPED(status)) {
            j->status = JOB_STOPPED;
            printf("\n[%d] Stopped    %s\n", j->id, j->cmd);
        } else {
            printf("[%d] Done       %s\n", j->id, j->cmd);
            job_remove(pid);
        }
    }
}
