#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#include <string.h>

// Space and tab
#define IS_WHITESPACE(x) (x == 0x20 || x == 0x09)

struct command_tokens_t {
    size_t token_count;
    char **tokens;
};

void tokens_read(struct command_tokens_t *tokens, char *input, size_t maxlen);

void tokens_finish(struct command_tokens_t *token);

#endif