#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include "builtins.h"
#include "jobs.h"

int run_builtin(char **args) {
    if (!args || !args[0]) return 0;

    /* exit */
    if (strcmp(args[0], "exit") == 0) {
        int code = args[1] ? atoi(args[1]) : 0;
        printf("exit\n");
        exit(code);
    }

    /* cd */
    if (strcmp(args[0], "cd") == 0) {
        char *path = args[1];
        if (!path) {
            path = getenv("HOME");
            if (!path) {
                fprintf(stderr, "iosh: cd: HOME not set\n");
                return 1;
            }
        }
        if (chdir(path) != 0)
            fprintf(stderr, "iosh: cd: %s: %s\n", path, strerror(errno));
        return 1;
    }

    /* pwd */
    if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)))
            printf("%s\n", cwd);
        else
            perror("iosh: pwd");
        return 1;
    }

    /* echo */
    if (strcmp(args[0], "echo") == 0) {
        for (int i = 1; args[i]; i++) {
            printf("%s", args[i]);
            if (args[i + 1]) printf(" ");
        }
        printf("\n");
        return 1;
    }

    /* history — show command history */
    if (strcmp(args[0], "history") == 0) {
        HIST_ENTRY **list = history_list();
        if (list) {
            for (int i = 0; list[i]; i++)
                printf("  %d  %s\n", i + history_base, list[i]->line);
        }
        return 1;
    }

    /* jobs — list all background/stopped jobs */
    if (strcmp(args[0], "jobs") == 0) {
        job_list();
        return 1;
    }

    /* fg %1 — bring job to foreground */
    if (strcmp(args[0], "fg") == 0) {
        int id = args[1] ? atoi(args[1] + (args[1][0] == '%' ? 1 : 0)) : 1;
        Job *j = job_find_by_id(id);
        if (!j) {
            fprintf(stderr, "iosh: fg: %d: no such job\n", id);
            return 1;
        }
        // Resume it if it was stopped
        kill(j->pid, SIGCONT);
        j->status = JOB_RUNNING;
        printf("%s\n", j->cmd);
        // Wait for it in foreground
        int status;
        waitpid(j->pid, &status, WUNTRACED);
        if (WIFSTOPPED(status)) {
            j->status = JOB_STOPPED;
            printf("\n[%d] Stopped    %s\n", j->id, j->cmd);
        } else {
            job_remove(j->pid);
        }
        return 1;
    }

    /* bg %1 — resume stopped job in background */
    if (strcmp(args[0], "bg") == 0) {
        int id = args[1] ? atoi(args[1] + (args[1][0] == '%' ? 1 : 0)) : 1;
        Job *j = job_find_by_id(id);
        if (!j) {
            fprintf(stderr, "iosh: bg: %d: no such job\n", id);
            return 1;
        }
        kill(j->pid, SIGCONT);
        j->status = JOB_RUNNING;
        printf("[%d] %s &\n", j->id, j->cmd);
        return 1;
    }

    return 0;
}
