#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include "executor.h"
#include "builtins.h"
#include "io.h"
#include "jobs.h"

static void build_cmd_string(Pipeline *pipeline, char *buf, int bufsize) {
    buf[0] = '\0';
    for (int i = 0; i < pipeline->num_cmds; i++) {
        for (int j = 0; pipeline->cmds[i].args[j]; j++) {
            strncat(buf, pipeline->cmds[i].args[j], bufsize - strlen(buf) - 1);
            if (pipeline->cmds[i].args[j+1])
                strncat(buf, " ", bufsize - strlen(buf) - 1);
        }
        if (i < pipeline->num_cmds - 1)
            strncat(buf, " | ", bufsize - strlen(buf) - 1);
    }
}

void execute(Command *cmd) {
    if (!cmd || !cmd->args[0]) return;

    // Only run builtin directly if there is NO redirection
    // If there is redirection, fork first so dup2 works properly
    if (!cmd->infile && !cmd->outfile) {
        if (run_builtin(cmd->args)) return;
    }
    pid_t pid = fork();

    if (pid < 0) {
        perror("iosh: fork");

    } else if (pid == 0) {
        // CHILD
        // Put child in its own process group
        setpgid(0, 0);

        // Give terminal control TO the child
        tcsetpgrp(STDIN_FILENO, getpid());

        // Restore default signals so Ctrl+C kills child normally
        signal(SIGINT,  SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);

        if (apply_redirections(cmd) == -1)
            exit(1);

        execvp(cmd->args[0], cmd->args);
        fprintf(stderr, "iosh: %s: command not found\n", cmd->args[0]);
        exit(127);

    } else {
        // PARENT
        setpgid(pid, pid);

        // Give terminal control to child
        tcsetpgrp(STDIN_FILENO, pid);

        int status;
        waitpid(pid, &status, WUNTRACED);

        // Take terminal control BACK to shell
        tcsetpgrp(STDIN_FILENO, getpgrp());

        if (WIFSTOPPED(status)) {
            Pipeline tmp;
            memset(&tmp, 0, sizeof(tmp));
            tmp.cmds[0]  = *cmd;
            tmp.num_cmds = 1;
            char buf[512];
            build_cmd_string(&tmp, buf, sizeof(buf));
            job_add(pid, buf);
            Job *j = job_find_by_pid(pid);
            if (j) {
                j->status = JOB_STOPPED;
                printf("\n[%d] Stopped    %s\n", j->id, buf);
            }
        }
    }
}

void execute_pipeline(Pipeline *pipeline) {
    if (!pipeline || pipeline->num_cmds == 0) return;

    int n = pipeline->num_cmds;

    if (n == 1 && !pipeline->background) {
        execute(&pipeline->cmds[0]);
        return;
    }

    int pipes[MAX_COMMANDS - 1][2];
    for (int i = 0; i < n - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("iosh: pipe");
            return;
        }
    }

    pid_t pids[MAX_COMMANDS];
    pid_t pgid = 0;

    for (int i = 0; i < n; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("iosh: fork");
            return;
        }

        if (pids[i] == 0) {
            // CHILD
            if (pgid == 0) pgid = getpid();
            setpgid(0, pgid);

            signal(SIGINT,  SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);

            if (i > 0)
                dup2(pipes[i-1][0], STDIN_FILENO);
            if (i < n - 1)
                dup2(pipes[i][1], STDOUT_FILENO);

            for (int j = 0; j < n - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            if (apply_redirections(&pipeline->cmds[i]) == -1)
                exit(1);

            execvp(pipeline->cmds[i].args[0], pipeline->cmds[i].args);
            fprintf(stderr, "iosh: %s: command not found\n", pipeline->cmds[i].args[0]);
            exit(127);
        }

        if (pgid == 0) pgid = pids[i];
        setpgid(pids[i], pgid);
    }

    // Give terminal to the pipeline's process group
    tcsetpgrp(STDIN_FILENO, pgid);

    // PARENT — close all pipe ends
    for (int i = 0; i < n - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    if (pipeline->background) {
        char cmd_str[512];
        build_cmd_string(pipeline, cmd_str, sizeof(cmd_str));
        job_add(pids[0], cmd_str);
        // Don't give terminal to background job
        tcsetpgrp(STDIN_FILENO, getpgrp());
    } else {
        for (int i = 0; i < n; i++)
            waitpid(pids[i], NULL, WUNTRACED);
        // Take terminal back
        tcsetpgrp(STDIN_FILENO, getpgrp());
    }
}
