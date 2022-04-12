#include "commands.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int get_file_output_from_command_line(struct command_tokens_t *tokens, struct command_execution_t *execution) {
    size_t index = tokens_search(tokens, ">");

    if (index == -1) {
        return 0;
    }

    char *filename_to_write = (*tokens).tokens[index + 1];

    tokens_remove(tokens, index, (*tokens).token_count);

    int fd = open(filename_to_write, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    execution->out = fd;

    return 0;
}

int get_file_input_from_command_line(char *command_line) {
    return -1;
}

int commands_make_exec(char *command_line, struct command_tokens_t *tokens,
                       struct command_execution_t **execution) {
    *execution = malloc(sizeof(struct command_execution_t));
    if (*execution == NULL) {
        return 1;
    }

    (*execution)->command_line = command_line;

    get_file_input_from_command_line((*execution)->command_line);

    get_file_output_from_command_line(tokens, (*execution));

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
        // TODO: I/O redirection here

        if (execution->out >= 0) {
            dup2(execution->out, 1);
            close(execution->out);
        }

        if (execution->in >= 0) {
            printf("Has input");
        }

        printf("THIS WAS EXECUTED");

        // https://man7.org/linux/man-pages/man2/dup.2.html
        // Combined with STDIN_FILENO/STDOUT_FILENO etc.
        // https://stackoverflow.com/questions/2605130/redirecting-exec-output-to-a-buffer-or-file

        // argv is already null terminated
        execvp(execution->executable, execution->argv);
        exit(EXIT_FAILURE);
        return;
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