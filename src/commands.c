#include "commands.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
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

// Function for handling the cd command
int change_wkd(struct command_part_t *part) {
    // check if argv has 2 or more args
    if (part->argc < 2) {
        printf("Please provide a target directory to change to\n");
        return -1;
    }

    // Check if passed argument is a legal dir
    if (chdir(part->argv[1]) == -1) {
        printf("Couldn't find target directory \"%s\"\n", part->argv[1]);
        return -1;
    }

    return 0;
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
    // chack if executable is cd
    if (strcmp(part->executable, "cd") == 0) {
        part->pid = -1;
        change_wkd(part);

        return;
    }

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

        // Internal command that prints all running background processes. We
        // run this in a forked process so that we can support pipes and
        // redirects for this command which could be useful for things like
        // searching for active processes (e.g. "jobs | grep rsync")
        if (strcmp(part->executable, "jobs") == 0) {
            if (commands_get_running_count() > 0) {
                printf("Jobs running in background (%ld):\n", commands_get_running_count());
                struct command_execution_t *exec;
                for (size_t i = 0; i < commands_get_running_count(); i++) {
                    exec = commands_get_running(i);
                    printf(" PID %d - \"%s\"\n", exec->parts[exec->part_count - 1].pid, exec->command_line);
                }
            } else {
                printf("There are no jobs running in the background\n");
            }

            exit(EXIT_SUCCESS);
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
        if (llist_append_element(&RUNNING_JOBS, execution)) {
            printf("Failed to append command line [%s] to background task list\n", execution->command_line);
        }
        return;
    }

    int status = 0;  // Default it to 0

    pid_t pid;
    // Wait for all children to complete in order
    for (size_t i = 0; i < execution->part_count; i++) {
        pid = execution->parts[i].pid;
        if (pid == (pid_t)-1) {
            continue;  // Can happen if no new process is spawned
        }

        if (waitpid(pid, &status, 0) == -1) {
            fprintf(stderr, "Error while waiting for PID %d [%s]\n", pid, execution->command_line);
        } else if (!WIFEXITED(status)) {
            fprintf(stderr, "Process did not exit normally for PID %d [%s]\n", pid,
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

    // Flag for later removals. This avoids concurrent modification, which
    // in this case results in us having free'd some of the blocks that are
    // pointed to by running_jobs above
    bool *remove_flag = malloc(sizeof(bool) * RUNNING_JOBS.size);
    memset(remove_flag, 0, sizeof(bool) * RUNNING_JOBS.size);

    struct command_execution_t *current;
    struct command_execution_t *tmp;
    while ((child = waitpid(-1, &status, WNOHANG)) > 0) {
        current = NULL;
        for (size_t i = 0; i < RUNNING_JOBS.size; i++) {
            // Only consider the last part
            tmp = running_jobs[i];
            if (tmp->parts[tmp->part_count - 1].pid == child) {
                current = tmp;
                remove_flag[i] = true;
                break;
            }
        }

        // This means that we have some piped command running in the background,
        // and we have just gotten a signal that one of the earlier commands have
        // finished. This does not mean that the entire command is done, so we wait
        // until the last part of the command completes before we print that it is
        // done. This does mean that if an error code occurs in one of the earlier
        // parts this is lost, but that is an issue for another day
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
    }

    for (size_t i = 0; i < RUNNING_JOBS.size; i++) {
        if (remove_flag[i]) {
            free_exec(running_jobs[i]);
        }
    }

    free(running_jobs);

    llist_remove_by_flag(&RUNNING_JOBS, remove_flag);
    free(remove_flag);
}
