#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

int parse_input(char *input, Command *cmd) {
    memset(cmd, 0, sizeof(Command));

    int   argc  = 0;
    char *token = strtok(input, " \t");

    while (token != NULL) {

        if (strcmp(token, ">>") == 0) {
            token = strtok(NULL, " \t");
            if (!token) {
                fprintf(stderr, "iosh: expected filename after >>\n");
                return -1;
            }
            cmd->outfile = token;
            cmd->append  = 1;

        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " \t");
            if (!token) {
                fprintf(stderr, "iosh: expected filename after >\n");
                return -1;
            }
            cmd->outfile = token;
            cmd->append  = 0;

        } else if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " \t");
            if (!token) {
                fprintf(stderr, "iosh: expected filename after <\n");
                return -1;
            }
            cmd->infile = token;

        } else {
            if (argc >= MAX_ARGS - 1) {
                fprintf(stderr, "iosh: too many arguments\n");
                return -1;
            }
            cmd->args[argc++] = token;
        }

        token = strtok(NULL, " \t");
    }

    cmd->args[argc] = NULL;
    return 0;
}

// NEW FUNCTION — splits "ls | grep txt | wc -l" into 3 separate commands
int parse_pipeline(char *input, Pipeline *pipeline) {
    memset(pipeline, 0, sizeof(Pipeline));

    // Check for background operator & at the end
    int len = strlen(input);
    if (len > 0 && input[len - 1] == '&') {
        pipeline->background = 1;
        input[len - 1] = '\0'; // remove the & from input
    }

    // Split the input by | character
    // strsep splits "ls | grep txt | wc -l" into ["ls ", " grep txt ", " wc -l"]
    char *segment;
    int   n = 0;

    while ((segment = strsep(&input, "|")) != NULL) {
        if (n >= MAX_COMMANDS) {
            fprintf(stderr, "iosh: too many commands in pipeline\n");
            return -1;
        }

        // trim leading/trailing spaces from each segment
        while (*segment == ' ' || *segment == '\t') segment++;
        int slen = strlen(segment);
        while (slen > 0 && (segment[slen-1] == ' ' || segment[slen-1] == '\t'))
            segment[--slen] = '\0';

        if (strlen(segment) == 0) continue;

        if (parse_input(segment, &pipeline->cmds[n]) == -1)
            return -1;

        n++;
    }

    pipeline->num_cmds = n;
    return 0;
}
