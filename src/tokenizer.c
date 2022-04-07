#include "tokenizer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void start_token(struct command_tokens_t *tokens, char *ch) {
    tokens->token_count++;
    tokens->tokens = realloc(tokens->tokens, sizeof(char *) * tokens->token_count);
    tokens->tokens[tokens->token_count - 1] = ch;
}

void tokens_read(struct command_tokens_t *tokens, char *input, size_t maxlen) {
    // TODO: Error handling

    // Duplicate the string to avoid modifying it
    input = strdup(input);
    tokens->buf_start = input;

    size_t len = strlen(input);
    if (len > maxlen) {
        len = maxlen;
    }

    tokens->token_count = 0;
    tokens->tokens = NULL;

    bool prev_whitespace = true;
    bool escape = false;
    bool quotation = false;

    /*
    This method also supports quotation marks and escape character for spaces
    because it was fun to implement.
    */

    char ch;
    bool tmp;
    for (size_t i = 0; i < len; i++) {
        ch = input[i];
        if (ch == '\\') {
            escape = true;
            memmove(&input[i], &input[i + 1], len - i - 1);
            ch = input[i];
            len--;
        }

        if (IS_WHITESPACE(ch)) {
            // We are within quotation marks
            if (quotation) {
                continue;
            }

            if (escape) {
                escape = false;
                continue;
            }

            // Replace all whitespace with null pointer to separate
            // the string into separate strings without reallocating
            input[i] = '\0';
            prev_whitespace = true;
            continue;
        }

        tmp = prev_whitespace;
        prev_whitespace = false;

        if (ch == '"') {
            quotation = !quotation;
            input[i] = '\0';
            i++;
        }

        if (tmp) {
            start_token(tokens, &input[i]);
        }
    }
}

void tokens_finish(struct command_tokens_t *token) {
    token->token_count = 0;
    free(token->buf_start);
    free(token->tokens);
}