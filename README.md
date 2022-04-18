# TDT4186 Operating Systems - Practical Exercise 3

Contains solution for assignment made by group 80

## Building

Simply run `make` to build the project. The compiled program will be located at `./flush`. You can run it using `./flush`. You may also use `make clean` to clean any generated build files.

## Running

Usage: `./flush`

## Useful commands

Check for memory and file descriptor leaks with Valgrind:

`valgrind --leak-check=full --track-fds=yes ./flush`
