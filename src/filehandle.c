#include "filehandle.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *get_file_output_from_command_line(char *command_line) {
    char *search_for_file_name_string = strchr(command_line, '>');

    if (search_for_file_name_string == NULL) {
        return NULL;
    }

    int length = strlen(search_for_file_name_string) - 2;
    char *filename_to_read = malloc(length);

    memcpy(filename_to_read, &search_for_file_name_string[2], length);

    printf("Found Filename to read input to: %s\n\n", filename_to_read);

    return fopen(filename_to_read, "w");
}

FILE *get_file_input_from_command_line(char *command_line) {
    return NULL;
}