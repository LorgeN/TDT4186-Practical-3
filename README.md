# TDT4186 Operating Systems - Practical Exercise 3

Contains solution for assignment made by group 80

## Progress

- [x] 3.1 Lorgen
- [ ] 3.2 Martinsen
- [x] 3.3 Jakob
- [x] 3.4 Lorgen
- [ ] 3.5 Martinsen
- [x] 3.6 Lorgen

## Building

Simply run `make` to build the project. The compiled program will be located at `./flush`. You can run it using `./flush`. You may also use `make clean` to clean any generated build files.

## Running

Usage: `./flush`

## Useful commands

Check for memory and file descriptor leaks with Valgrind:

`valgrind --leak-check=full --track-fds=yes ./flush`
