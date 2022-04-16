#include "commands.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "llist.h"

static struct list_t RUNNING_JOBS = {
    .head = NULL,
    .tail = NULL,
    .size = 0};

static int check_if_background(struct command_tokens_t *tokens, struct command_execution_t *execution) {
    char *last_token = tokens->tokens[tokens->token_count - 1];
    if (strcmp(last_token, "&")) {
        execution->background = false;
        return 0;
    }

    execution->background = true;
    if (tokens_remove(tokens, tokens->token_count - 1, tokens->token_count)) {
        return 1;
    }

    return 0;
}

static int get_file_output_from_command_line(struct command_tokens_t *tokens, struct command_part_t *execution) {
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
    if (tokens_remove(tokens, index, index + 2)) {
        free(filename_to_write);
        return 1;
    }

    int fd;
    if (append_mode == 1) {
        // Difference between trunc and append:
        // https://man7.org/linux/man-pages/man2/open.2.html
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

static int get_file_input_from_command_line(struct command_tokens_t *tokens, struct command_part_t *execution) {
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
    if (tokens_remove(tokens, index, index + 2)) {
        free(filename_to_read);
        return 1;
    }

    int fd = open(filename_to_read, O_RDONLY);
    free(filename_to_read);

    if (fd == -1) {
        return 2;
    }

    execution->in = fd;
    return 0;
}

static void free_exec(struct command_execution_t *execution) {
    struct command_part_t *part;
    for (size_t i = 0; i < execution->part_count; i++) {
        part = &execution->parts[i];
        // Free all the argument strings
        for (size_t j = 0; j < part->argc; j++) {
            free(part->argv[j]);
        }

        free(part->argv);
    }

    free(execution->parts);
    free(execution->command_line);
    free(execution);
}

int commands_make_exec(char *command_line, struct command_tokens_t *tokens,
                       struct command_execution_t **execution) {
    *execution = malloc(sizeof(struct command_execution_t));
    if (*execution == NULL) {
        return 1;
    }

    // Do this check early so we can use original tokens, which
    // is freed at a later point
    if (check_if_background(tokens, *execution)) {
        free(*execution);
        return 1;
    }

    struct command_tokens_t *parts;
    size_t part_count;

    if (tokens_split("|", tokens, &part_count, &parts)) {
        free(*execution);
        return 1;
    }

    (*execution)->part_count = part_count;
    (*execution)->parts = malloc(sizeof(struct command_part_t) * part_count);
    if ((*execution)->parts == NULL) {
        free(*execution);
        free(parts);
        return 1;
    }

    (*execution)->command_line = strdup(command_line);
    if ((*execution)->command_line == NULL) {
        free((*execution)->parts);
        free(*execution);
        free(parts);
        return 1;
    }

    struct command_tokens_t *part_tokens;
    struct command_part_t *part;
    for (size_t i = 0; i < part_count; i++) {
        part_tokens = &parts[i];
        part = &((*execution)->parts[i]);
        if (get_file_input_from_command_line(part_tokens, part) || get_file_output_from_command_line(part_tokens, part)) {
            free((*execution)->parts);
            free((*execution)->command_line);
            free(*execution);
            free(parts);
            return 2;
        }

        part->argc = part_tokens->token_count;
        part->argv = malloc(sizeof(char *) * (part->argc + 1));
        if (part->argv == NULL) {
            // Free everything previous to this iteration
            (*execution)->part_count = i - 1;
            free_exec(*execution);
            free(parts);
            return 2;
        }

        for (size_t j = 0; j < part->argc; j++) {
            part->argv[j] = strdup(part_tokens->tokens[j]);

            // Unallocate everything we allocated if this fails
            if (part->argv[j] == NULL) {
                // Set argc and part_count to i so we can use free_exec
                part->argc = j;
                (*execution)->part_count = i;
                free_exec(*execution);
                free(parts);
                return 2;
            }
        }

        part->executable = part->argv[0];

        // exec calls require NULL termination of the vector so
        // we handle it here for ease of use. The argc value shows
        // one less than what is allocated, so any iteration or similar
        // will not encounter any troubles
        part->argv[part->argc] = NULL;

        tokens_finish(part_tokens);  // We are done with these now
    }

    free(parts);
    return 0;
}

static void execute_part(struct command_part_t *part, bool pipe) {
    pid_t pid = fork();

    // In child
    if (pid == 0) {
        if (part->out >= 0) {
            dup2(part->out, STDOUT_FILENO);
            close(part->out);
        }

        if (part->in >= 0) {
            dup2(part->in, STDIN_FILENO);
            close(part->in);
        }

        // execution->argv is already null terminated
        execvp(part->executable, part->argv);
        // Should never reach this point
        exit(EXIT_FAILURE);
    }

    // Make sure we close fds in this process aswell
    if (part->out >= 0) {
        close(part->out);
    }

    if (!pipe && part->in >= 0) {
        close(part->in);
    }

    part->pid = pid;
}

void commands_execute(struct command_execution_t *execution) {
    struct command_part_t *part;
    int in = -1, fd[2];

    // If there is no piping going on, this for loop will not run since
    // part_count will be 1. This means we don't need any special
    // handling for pipes vs no pipes
    for (size_t i = 0; i < (execution->part_count - 1); i++) {
        part = &execution->parts[i];

        pipe(fd);

        // This handles the case where someone is dumb and writes stuff
        // like "ls -l > test.txt | grep whatever", which would otherwise
        // leave an open file descriptor. We should however still allow
        // I/O redirection into the first command, thus we check != 0
        if (in != -1) {
            if (part->in > 1) {
                // Warn the idiots
                fprintf(stderr, "Attempted I/O redirect into command that is part of pipeline at illegal position in command line [%s]! This redirection has been overwritten and ignored.\n", execution->command_line);
                close(part->in);
            }

            part->in = in;
        }

        if (part->out >= 0) {
            fprintf(stderr, "Attempted I/O redirect out of command that is part of pipeline at illegal position in command line [%s]! This redirection has been overwritten and ignored.\n", execution->command_line);
            close(part->out);
        }

        part->out = fd[1];
        execute_part(part, true);

        if (in != -1) {
            close(in);  // Close previous pipe read end
        }

        in = fd[0];
    }

    part = &execution->parts[execution->part_count - 1];
    if (in != -1) {
        if (part->in >= 0) {
            // Warn the idiots
            fprintf(stderr, "Attempted I/O redirect into command that is part of pipeline at illegal position in command line [%s]! This redirection has been overwritten and ignored.\n", execution->command_line);
            close(part->in);
        }

        part->in = in;
    }

    execute_part(part, false);

    if (execution->background) {
        llist_append_element(&RUNNING_JOBS, execution);
        return;
    }

    int status;
    // Wait for all children to complete in order
    for (size_t i = 0; i < execution->part_count; i++) {
        if (waitpid(execution->parts[i].pid, &status, 0) == -1) {
            fprintf(stderr, "Error while waiting for PID %d [%s]\n", execution->parts[i].pid,
                    execution->command_line);
        } else if (!WIFEXITED(status)) {
            fprintf(stderr, "Process did not exit normally for PID %d [%s]\n", execution->parts[i].pid,
                    execution->command_line);
        }
    }

    if (WIFEXITED(status)) {
        fprintf(stdout, "Exit status [%s] = %d\n", execution->command_line,
                WEXITSTATUS(status));
    }

    free_exec(execution);
}

size_t commands_get_running_count() {
    return RUNNING_JOBS.size;
}

struct command_execution_t *commands_get_running(size_t index) {
    return (struct command_execution_t *)llist_get(&RUNNING_JOBS, index);
}

void commands_cleanup_running() {
    pid_t child;
    int status;

    struct command_execution_t **running_jobs = malloc(sizeof(struct command_execution_t *) * RUNNING_JOBS.size);
    llist_elements(&RUNNING_JOBS, (void **)running_jobs);

    struct command_execution_t *current;
    struct command_execution_t *tmp;
    while ((child = waitpid(-1, &status, WNOHANG)) > 0) {
        current = NULL;
        for (size_t i = 0; i < RUNNING_JOBS.size; i++) {
            // Only consider the last part
            tmp = running_jobs[i];
            if (tmp->parts[tmp->part_count - 1].pid == child) {
                current = tmp;
                break;
            }
        }

        if (current == NULL) {
            continue;
        }

        if (!WIFEXITED(status)) {
            fprintf(stderr, "Process did not exit normally for PID %d [%s]\n", child,
                    current->command_line);
        } else {
            fprintf(stdout, "Exit status [%s] = %d\n", current->command_line,
                    WEXITSTATUS(status));
        }

        llist_remove_element(&RUNNING_JOBS, current);
        free_exec(current);
    }

    free(running_jobs);
}