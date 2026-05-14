#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "parser.h"
#include "executor.h"
#include "jobs.h"

#define MAX_INPUT 1024

int main() {
    // Shell ignores these signals permanently
    // Children restore them to default via executor.c
    signal(SIGINT,  SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    char input[MAX_INPUT];

    while (1) {
        // Check if any background jobs finished
        job_reap();

        printf("\033[1;36miosh$\033[0m ");
        fflush(stdout);

        if (!fgets(input, MAX_INPUT, stdin)) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';
        if (strlen(input) == 0) continue;

        Pipeline pipeline;
        if (parse_pipeline(input, &pipeline) == -1) continue;
        if (pipeline.num_cmds == 0) continue;

        execute_pipeline(&pipeline);
    }
    return 0;
}
