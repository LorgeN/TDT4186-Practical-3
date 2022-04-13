#include "tokenizer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static int add_token(struct command_tokens_t *tokens, char *ch, size_t len) {
    if (len == 0) {
        return 0;  // Nothing to add
    }

    char **reallocated = realloc(tokens->tokens, sizeof(char *) * (tokens->token_count + 1));
    if (reallocated == NULL) {
        return 1;
    }

    tokens->tokens = reallocated;
    tokens->token_count++;
    char *dest = malloc(len + 1);
    if (dest == NULL) {
        return 2;
    }

    tokens->tokens[tokens->token_count - 1] = dest;
    memcpy(dest, ch, len);
    *(dest + len) = '\0';

    return 0;
}

int tokens_read(struct command_tokens_t *tokens, char *input, size_t maxlen) {
    // Duplicate the string to avoid modifying it
    input = strdup(input);
    if (input == NULL) {
        return 1;
    }

    size_t len = strlen(input);
    if (len > maxlen) {
        len = maxlen;
    }

    tokens->token_count = 0;
    tokens->tokens = NULL;

    bool escape = false;
    bool quotation = false;
    size_t start_index = 0;

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

            if (add_token(tokens, input + start_index, i - start_index)) {
                tokens_finish(tokens);
                free(input);
                return 1;
            }

            start_index = i + 1;
            continue;
        }

        // Special consideration to split even if there is no whitespace
        if (IS_IO_REDIRECT(ch)) {
            if (quotation) {
                continue;
            }

            // Complete previous token
            if (start_index != i) {
                if (add_token(tokens, input + start_index, i - start_index)) {
                    tokens_finish(tokens);
                    free(input);
                    return 1;
                }

                start_index = i;
            }

            // Allow ">>"
            if ((len - i) > 1 && input[i + 1] == '>') {
                i++;
            }

            if (add_token(tokens, input + start_index, i - start_index + 1)) {
                tokens_finish(tokens);
                free(input);
                return 1;
            }

            start_index = i + 1;
        }

        if (ch == '"') {
            quotation = !quotation;

            if (quotation) {
                start_index = i + 1;
            } else {
                if (add_token(tokens, input + start_index, i - start_index)) {
                    tokens_finish(tokens);
                    free(input);
                    return 1;
                }

                start_index = i + 1;
            }
        }
    }

    if ((len - start_index) > 0 && add_token(tokens, input + start_index, len - start_index)) {
        for (size_t i = 0; i < tokens->token_count; i++) {
            free(tokens->tokens[i]);
        }

        free(input);
        free(tokens->tokens);
        return 1;
    }

    free(input);
    return 0;
}

void tokens_finish(struct command_tokens_t *token) {
    for (size_t i = 0; i < token->token_count; i++) {
        free(token->tokens[i]);
    }

    token->token_count = 0;
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

    for (size_t i = index_start; i < index_end; i++) {
        free(tokens->tokens[i]);
    }

    size_t to_remove = index_end - index_start;
    // Amount of char * to the right of the data we need to remove
    size_t copy_count = tokens->token_count - index_end;

    // Move any data that comes after the section we remove so that it is not also removed
    memmove(tokens->tokens + index_start, tokens->tokens + index_end, sizeof(char *) * copy_count);

    tokens->token_count -= to_remove;
    char **reallocated = realloc(tokens->tokens, sizeof(char *) * tokens->token_count);
    if (reallocated == NULL) {
        return 3;
    }

    tokens->tokens = reallocated;
    return 0;
}