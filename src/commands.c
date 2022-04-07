#include "commands.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void commands_make_exec(char *command_line, struct command_tokens_t *tokens, struct command_execution_t **execution) {
    // TODO: Error handling for allocations
    *execution = malloc(sizeof(struct command_execution_t));
    (*execution)->command_line = command_line;

    (*execution)->in = NULL;  // TODO: Allow for I/O redirection
    (*execution)->out = NULL;

    // Duplicate all strings so that they aren't lost when buffer
    // and tokens are destroyed. We only assume that execution pointer
    // is safe
    (*execution)->argc = tokens->token_count;

    (*execution)->argv = malloc(sizeof(char *) * ((*execution)->argc + 1));
    for (size_t i = 0; i < (*execution)->argc; i++) {
        (*execution)->argv[i] = strdup(tokens->tokens[i]);
    }

    (*execution)->executable = (*execution)->argv[0];

    // exec calls require NULL termination of the vector so
    // we handle it here for ease of use. The argc value shows
    // one less than what is allocated, so any iteration or similar
    // will not encounter any troubles
    (*execution)->argv[(*execution)->argc] = NULL;
}

void commands_execute(struct command_execution_t *execution) {
    pid_t pid = fork();

    // In child
    if (pid == 0) {
        // TODO: I/O redirection here

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
        fprintf(stderr, "Error while waiting for PID %d [%s]\n", pid, execution->command_line);
    } else if (!WIFEXITED(status)) {
        fprintf(stderr, "Process did not exit normally for PID %d [%s]\n", pid, execution->command_line);
    } else {
        fprintf(stdout, "Exit status [%s] = %d\n", execution->command_line, WEXITSTATUS(status));
    }

    // Free all the argument strings
    for (size_t i = 0; i < execution->argc; i++) {
        free(execution->argv[i]);
    }

    free(execution->argv);
    free(execution);
}