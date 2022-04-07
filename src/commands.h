#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <stdio.h>

#include "tokenizer.h"

struct command_execution_t {
    FILE *out;
    FILE *in;
    char *command_line;
    char *executable;
    int argc;
    char **argv;
};

void commands_make_exec(char *command_line, struct command_tokens_t *tokens, struct command_execution_t **execution);

void commands_execute(struct command_execution_t *execution);

#endif