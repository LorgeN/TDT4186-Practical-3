#include "filehandle.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokenizer.h"

int get_file_output_from_command_line(char *command_line) {
    char *search_for_file_name_string = strchr(command_line, '>');

    if (search_for_file_name_string == NULL) {
        return -1;
    }

    // strchr always returns position of >
    int current_char = 1;

    // offset whitespaces before file
    while (IS_WHITESPACE(search_for_file_name_string[current_char])) {
        current_char++;
    }

    int length = strlen(search_for_file_name_string) - current_char;
    char *filename_to_read = malloc(length);

    memcpy(filename_to_read, &search_for_file_name_string[current_char], length);

    printf("Found filename to write output to: %s\n", filename_to_read);

    return open(filename_to_read, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
}

int get_file_input_from_command_line(char *command_line) {
    return -1;
}