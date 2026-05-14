#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

void execute(Command *cmd);
void execute_pipeline(Pipeline *pipeline);

#endif
