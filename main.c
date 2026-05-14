#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parser.h"
#include "executor.h"
#include "jobs.h"

#define MAX_INPUT    1024
#define HISTORY_FILE "/.ioshell_history"

// Returns current time in milliseconds
static long get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

int main() {
    signal(SIGINT,  SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    // Load history from previous sessions
    char history_path[512];
    snprintf(history_path, sizeof(history_path),
             "%s%s", getenv("HOME"), HISTORY_FILE);
    read_history(history_path);

    char *input;
    while (1) {
        job_reap();

        input = readline("\033[1;32miosh$\033[0m ");
        if (!input) {
            printf("\n");
            break;
        }

        if (strlen(input) == 0) {
            free(input);
            continue;
        }

        // Handle !! → repeat last command
        if (strcmp(input, "!!") == 0) {
            HIST_ENTRY *last = history_get(history_length);
            if (!last) {
                fprintf(stderr, "iosh: !!: no previous command\n");
                free(input);
                continue;
            }
            free(input);
            input = strdup(last->line);
            printf("%s\n", input);
        }

        // Handle !N → run command number N
        if (input[0] == '!' && input[1] != '!') {
            int n = atoi(input + 1);
            HIST_ENTRY *entry = history_get(n);
            if (!entry) {
                fprintf(stderr, "iosh: !%d: event not found\n", n);
                free(input);
                continue;
            }
            free(input);
            input = strdup(entry->line);
            printf("%s\n", input);
        }

        // Add to history (skip duplicates)
        HIST_ENTRY *last = history_get(history_length);
        if (!last || strcmp(last->line, input) != 0)
            add_history(input);
        write_history(history_path);

        Pipeline pipeline;
        if (parse_pipeline(input, &pipeline) == -1) {
            free(input);
            continue;
        }
        if (pipeline.num_cmds == 0) {
            free(input);
            continue;
        }

        // ⏱ START timer just before running
        long start = get_time_ms();

        execute_pipeline(&pipeline);

        // ⏱ STOP timer after command finishes
        long ms = get_time_ms() - start;

        // Show time — color changes based on how long it took
        // green = fast (<100ms), yellow = medium, red = slow (>2000ms)
        if (ms < 100)
            printf("\033[0;32m  took %ldms\033[0m\n", ms);
        else if (ms < 2000)
            printf("\033[0;33m  took %ldms\033[0m\n", ms);
        else
            printf("\033[0;31m  took %ldms\033[0m\n", ms);

        free(input);
    }

    return 0;
}
