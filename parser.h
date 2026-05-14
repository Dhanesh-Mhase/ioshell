#ifndef PARSER_H
#define PARSER_H

#define MAX_ARGS     64
#define MAX_COMMANDS 32

typedef struct {
    char *args[MAX_ARGS]; // NULL-terminated argument list for execvp
    char *infile;         // filename after <  (NULL if none)
    char *outfile;        // filename after > or >> (NULL if none)
    int   append;         // 1 if >>, 0 if >
} Command;

// NEW: holds an entire pipeline like: ls | grep txt | wc -l
typedef struct {
    Command cmds[MAX_COMMANDS]; // each command in the pipeline
    int     num_cmds;           // how many commands
    int     background;         // 1 if trailing &, 0 if not
} Pipeline;

int parse_input(char *input, Command *cmd);
int parse_pipeline(char *input, Pipeline *pipeline);

#endif
