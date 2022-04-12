
#ifndef __FILEHANDLE_H__
#define __FILEHANDLE_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

FILE *get_file_output_from_command_line(char *command_line);

FILE *get_file_input_from_command_line(char *command_line);

#endif