#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "io.h"

int apply_redirections(Command *cmd) {

    if (cmd->infile) {
        int fd = open(cmd->infile, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "iosh: %s: %s\n", cmd->infile, strerror(errno));
            return -1;
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("iosh: dup2");
            return -1;
        }
        close(fd);
    }

    if (cmd->outfile) {
        int flags = O_WRONLY | O_CREAT;
        if (cmd->append)
            flags |= O_APPEND;
        else
            flags |= O_TRUNC;

        int fd = open(cmd->outfile, flags, 0644);
        if (fd == -1) {
            fprintf(stderr, "iosh: %s: %s\n", cmd->outfile, strerror(errno));
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("iosh: dup2");
            return -1;
        }
        close(fd);
    }

    return 0;
}
