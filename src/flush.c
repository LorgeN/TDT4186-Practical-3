#include <stdio.h>

#include "tokenizer.h"

int main(int argc, char **argv) {
    char test_string[512] = "this is a test\\ of \"everything wohooo\"";

    printf("Tokens:\n");

    struct command_tokens_t tokens;
    tokenize(&tokens, test_string, 512);

    for (size_t i = 0; i < tokens.token_count; i++) {
        printf(" - Token %ld: %s\n", i, tokens.tokens[i]);
    }
}
