#include "tokenizer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static int start_token(struct command_tokens_t *tokens, char *ch) {
    tokens->token_count++;

    // We could preserve the previous value by not directly assigning
    // here, but if this fails the entire operation has also failed so
    // there is no point
    tokens->tokens = realloc(tokens->tokens, sizeof(char *) * tokens->token_count);
    if (tokens->tokens == NULL) {
        return 1;
    }

    tokens->tokens[tokens->token_count - 1] = ch;
    return 0;
}

int tokens_read(struct command_tokens_t *tokens, char *input, size_t maxlen) {
    // Duplicate the string to avoid modifying it
    input = strdup(input);
    if (input == NULL) {
        return 1;
    }

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
            if (start_token(tokens, &input[i])) {
                free(input);
                // Failed to allocate space for token pointers
                return 2;
            }
        }
    }

    return 0;
}

void tokens_finish(struct command_tokens_t *token) {
    token->token_count = 0;
    free(token->buf_start);
    free(token->tokens);
}

size_t tokens_search(struct command_tokens_t *tokens, char *target) {
    for (size_t i = 0; i < tokens->token_count; i++) {
        if (!strcmp(tokens->tokens[i], target)) {
            return i;
        }
    }

    return -1;
}

int tokens_remove(struct command_tokens_t *tokens, size_t index_start, size_t index_end) {
    if (index_end <= index_start) {
        return 1;
    }

    if (index_end > tokens->token_count) {
        return 2;
    }

    size_t to_remove = index_end - index_start;

    // Move any data that comes after the section we remove so that it is not also removed
    for (int i = 0; i < to_remove; i++) {
        // This line only copies one string, so move all strings to correct position
        // using for loop
        memmove(tokens->tokens + index_start + i, tokens->tokens + index_end + i, tokens->token_count - index_end);
    }

    tokens->token_count -= to_remove;
    char **reallocated = realloc(tokens->tokens, sizeof(char *) * tokens->token_count);
    if (reallocated == NULL) {
        return 3;
    }

    tokens->tokens = reallocated;
    return 0;
}