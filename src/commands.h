#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "tokenizer.h"

struct command_part_t {
    int out;
    int in;
    char *executable;
    int argc;
    char **argv;
    pid_t pid;
};

struct command_execution_t {
    char *command_line;
    struct command_part_t *parts;
    size_t part_count;
    bool background;
};

int commands_make_exec(char *command_line, struct command_tokens_t *tokens, struct command_execution_t **execution);

void commands_execute(struct command_execution_t *execution);

size_t commands_get_running_count();

struct command_execution_t *commands_get_running(size_t index);

void commands_cleanup_running();

#endif