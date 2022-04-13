#include "commands.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int get_file_output_from_command_line(struct command_tokens_t *tokens, struct command_execution_t *execution) {
    size_t index = tokens_search(tokens, ">");

    int append_mode = 0;

    if (index == -1) {
        index = tokens_search(tokens, ">>");
        if (index == -1) {
            // No error occurred, but there was no output redirection tokens present
            execution->out = -1;
            return 0;
        } else {
            append_mode = 1;
        }
    }

    char *filename_to_write = strdup(tokens->tokens[index + 1]);
    if (filename_to_write == NULL) {
        return 1;
    }

    // This call will free the above string if we do not duplicate it first
    tokens_remove(tokens, index, index + 2);

    int fd;
    if (append_mode == 1) {
        // Difference between trunc and append:
        // https://stackoverflow.com/questions/59886546/default-write-behaviour-o-trunc-or-o-append
        fd = open(filename_to_write, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    } else {
        fd = open(filename_to_write, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    }

    free(filename_to_write);

    if (fd == -1) {
        return 2;
    }

    execution->out = fd;
    return 0;
}

int get_file_input_from_command_line(struct command_tokens_t *tokens, struct command_execution_t *execution) {
    size_t index = tokens_search(tokens, "<");

    if (index == -1) {
        execution->in = -1;
        return 0;
    }

    char *filename_to_read = strdup(tokens->tokens[index + 1]);
    if (filename_to_read == NULL) {
        return 1;
    }

    // This call will free the above string if we do not duplicate it first
    tokens_remove(tokens, index, index + 2);

    int fd = open(filename_to_read, O_RDONLY);
    free(filename_to_read);

    if (fd == -1) {
        return 2;
    }

    execution->in = fd;
    return 0;
}

int commands_make_exec(char *command_line, struct command_tokens_t *tokens,
                       struct command_execution_t **execution) {
    *execution = malloc(sizeof(struct command_execution_t));
    if (*execution == NULL) {
        return 1;
    }

    (*execution)->command_line = command_line;

    if (get_file_input_from_command_line(tokens, *execution)) {
        free(*execution);
        return 1;
    }

    if (get_file_output_from_command_line(tokens, *execution)) {
        free(*execution);
        return 1;
    }

    // Duplicate all strings so that they aren't lost when buffer
    // and tokens are destroyed. We only assume that execution pointer
    // is safe
    (*execution)->argc = tokens->token_count;

    (*execution)->argv = malloc(sizeof(char *) * ((*execution)->argc + 1));
    if ((*execution)->argv == NULL) {
        free(*execution);
        return 1;
    }

    for (size_t i = 0; i < (*execution)->argc; i++) {
        (*execution)->argv[i] = strdup(tokens->tokens[i]);

        // Unallocate everything we allocated if this fails
        if ((*execution)->argv[i] == NULL) {
            for (size_t j = 0; j < i; j++) {
                free((*execution)->argv[j]);
            }

            free((*execution)->argv);
            free(*execution);
            return 2;
        }
    }

    (*execution)->executable = (*execution)->argv[0];

    // exec calls require NULL termination of the vector so
    // we handle it here for ease of use. The argc value shows
    // one less than what is allocated, so any iteration or similar
    // will not encounter any troubles
    (*execution)->argv[(*execution)->argc] = NULL;
    return 0;
}

void commands_execute(struct command_execution_t *execution) {
    pid_t pid = fork();

    // In child
    if (pid == 0) {
        if (execution->out >= 0) {
            dup2(execution->out, STDOUT_FILENO);
            close(execution->out);
        }

        if (execution->in >= 0) {
            dup2(execution->in, STDIN_FILENO);
            close(execution->in);
        }

        // execution->argv is already null terminated
        execvp(execution->executable, execution->argv);
        // Should never reach this point
        exit(EXIT_FAILURE);
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        fprintf(stderr, "Error while waiting for PID %d [%s]\n", pid,
                execution->command_line);
    } else if (!WIFEXITED(status)) {
        fprintf(stderr, "Process did not exit normally for PID %d [%s]\n", pid,
                execution->command_line);
    } else {
        fprintf(stdout, "Exit status [%s] = %d\n", execution->command_line,
                WEXITSTATUS(status));
    }

    // Free all the argument strings
    for (size_t i = 0; i < execution->argc; i++) {
        free(execution->argv[i]);
    }

    free(execution->argv);
    free(execution);
}