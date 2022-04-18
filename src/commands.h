#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "tokenizer.h"

/**
 * Data for executing a part of a command, e.g. "ls -l > file.txt".
 * Multiple parts may be required for commands that use pipes, e.g.
 * "ls -l | grep something > file.txt"
 */
struct command_part_t {
    /**
     * The file descriptor for stdout, -1 if not specified.
     * Use for I/O redirection and pipes.
     */
    int out;
    /**
     * The file descriptor for stdin, -1 if not specified.
     */
    int in;
    /**
     * The name of the executable. If the command is e.g. "ls -l"
     * this would be "ls"
     */
    char *executable;
    /**
     * The amount of provided arguments, including the executable
     */
    int argc;
    /**
     * @brief Argument values. 
     *
     * The argument values, for direct use with exec calls. Should
     * therefore have NULL as the last element as index argc, meaning
     * this should have a size of sizeof(char *) * (argc + 1)
     */
    char **argv;
    /**
     * @brief The PID of the process executing this part. 
     * 
     * Each part of a command should be executed as a forked process.
     * This is the PID of that process for this specific part of the
     * command.
     */
    pid_t pid;
};

/**
 * All data necessary to execute a command
 */
struct command_execution_t {
    /**
     * Command line as text. This is used for front-end purposes.
     */
    char *command_line;
    /**
     * Array of different parts of this command
     */
    struct command_part_t *parts;
    /**
     * The amount of parts contained in the parts array
     */
    size_t part_count;
    /**
     * If this command execution should run as a background process.
     */
    bool background;
};

/**
 * @brief Parses the given tokens to plan a command execution
 *
 * @param command_line The command line as a string
 * @param tokens The parsed tokens based on the command line string
 * @param execution Pointer to execution variable. Used to output the result of this call.
 * @return int - 0 if success, non-zero otherwise
 */
int commands_make_exec(char *command_line, struct command_tokens_t *tokens, struct command_execution_t **execution);

/**
 * @brief Execute the given command
 *
 * @param execution The command to execute
 */
void commands_execute(struct command_execution_t *execution);

/**
 * @brief The amount of processes running in the background
 *
 * @return size_t - The amount of processes running in the background
 */
size_t commands_get_running_count();

/**
 * @brief Get information about a running command
 *
 * @param index The index of the running command
 * @return struct command_execution_t* - Information about running command
 */
struct command_execution_t *commands_get_running(size_t index);

/**
 * @brief Look for any running background jobs, and clean up zombie processes
 */
void commands_cleanup_running();

#endif