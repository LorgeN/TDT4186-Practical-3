#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#include <string.h>

// Space and tab
#define IS_WHITESPACE(x) (x == 0x20 || x == 0x09)
#define IS_IO_REDIRECT(x) (x == '>' || x == '<')

struct command_tokens_t {
    size_t token_count;
    char **tokens;
};

int tokens_read(struct command_tokens_t *tokens, char *input, size_t maxlen);

void tokens_finish(struct command_tokens_t *tokens);

size_t tokens_search(struct command_tokens_t *tokens, char* target);

int tokens_remove(struct command_tokens_t *tokens, size_t index_start, size_t index_end);

#endif