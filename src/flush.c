#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "commands.h"
#include "tokenizer.h"

#define NEW_LINE '\n'

volatile sig_atomic_t kill_line_flag;

static void prompt() {
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        fprintf(stderr, "Unable to retrieve current working directory!\n");
        exit(EXIT_FAILURE);
    }

    ssize_t data = 0, allocated = 128;
    char *buf = malloc(allocated);
    memset(buf, 0, allocated);

    fprintf(stdout, "%s: ", cwd);
    fflush(stdout);

    char last_char;
    ssize_t res;
    while ((res = read(STDIN_FILENO, buf + data, allocated - data)) >= 0) {
        // This only happens when the user enters CTRL + D, which gives EOF
        if (res == 0) {
            fprintf(stdout, "\nGood bye!\n");
            exit(EXIT_SUCCESS);
        }

        data += res;
        last_char = buf[data - 1];

        if (kill_line_flag) {
            kill_line_flag = 0;
            printf("\n");

            free(cwd);
            free(buf);
            // Start next prompt
            return;
        }

        if (last_char == NEW_LINE) {
            // Input complete
            buf[data - 1] = '\0';
            break;
        }

        if (allocated - data < 32) {
            allocated += 64;
            buf = realloc(buf, allocated);
        }
    }

    if (strlen(buf) != 0) {
        struct command_tokens_t tokens;
        tokens_read(&tokens, buf, data);

        struct command_execution_t *execution;
        commands_make_exec(buf, &tokens, &execution);
        tokens_finish(&tokens);

        commands_execute(execution);
    }

    free(cwd);
    free(buf);
}

void __shutdown_sig_handler(int sig) {
    // Ctrl + C allows you to cancel the current command line,
    // but doesn't exit the shell
    kill_line_flag = 1;
}

int main(int argc, char **argv) {
    struct sigaction shutdown_sig_action;
    sigemptyset(&shutdown_sig_action.sa_mask);
    shutdown_sig_action.sa_flags = 0;
    shutdown_sig_action.sa_handler = __shutdown_sig_handler;

    sigaction(SIGINT, &shutdown_sig_action, NULL);

    while (true) {
        prompt();
    }
}
