#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#include <string.h>

// Checks if the character is considered whitespace
#define IS_WHITESPACE(x) (x == 0x20 || x == 0x09)
// Checks if the character is I/O redirect
#define IS_IO_REDIRECT(x) (x == '>' || x == '<')
// Checks if the character is a pipe split character
#define IS_PIPE_SPLIT(x) (x == '|')

/**
 * @brief Tokens of a command, e.g. "ls -l | grep something" results
 * in {"ls", "-l", "|", "grep", "something"}
 */
struct command_tokens_t {
    /**
     * @brief Amount of tokens
     */
    size_t token_count;
    /**
     * @brief The tokens as strings
     */
    char **tokens;
};

/**
 * @brief Parses the given tokens
 *
 * @param tokens The output location for parsed result
 * @param input Input string
 * @param maxlen The maximum parsed length
 * @return int - 0 if success, non-zero otherwise
 */
int tokens_read(struct command_tokens_t *tokens, char *input, size_t maxlen);

/**
 * @brief Frees any allocated memory
 *
 * @param tokens The tokens
 */
void tokens_finish(struct command_tokens_t *tokens);

/**
 * @brief Split the given tokens into multiple lists of tokens on the
 * given token characters. The token characters will be removed.
 *
 * E.g. {"ls", "-l", "|", "grep", "something"} split on "|" gives
 * {{"ls", "-l"}, {"grep", "something"}}.
 *
 * The input tokens are automatically free'd via equivalent to calling
 * the tokens_finish method.
 *
 * @param token The token to split on
 * @param input The input tokens
 * @param count Output pointer for post split count
 * @param output Output pointer for list of split tokens. This is malloc'd
 * automatically, and will need to be free'd once used. Each created token
 * will also need to be free'd via the tokens_finish method.
 * @return int - 0 if success, non-zero otherwise
 */
int tokens_split(char *token, struct command_tokens_t *input, size_t *count, struct command_tokens_t **output);

/**
 * @brief Searches for a token equal to the given target
 *
 * @param tokens The input tokens
 * @param target The target token string
 * @return size_t - The index of a token equivalent to the given target. If
 * there are multiple equivalent tokens this will be the first (lowest) index.
 */
size_t tokens_search(struct command_tokens_t *tokens, char *target);

/**
 * @brief Remove the tokens at the given indices from the token list.
 *
 * @param tokens The tokens
 * @param index_start The start index (inclusive)
 * @param index_end The end index (exclusive)
 * @return int - 0 if success, non-zero otherwise
 */
int tokens_remove(struct command_tokens_t *tokens, size_t index_start, size_t index_end);

#endif