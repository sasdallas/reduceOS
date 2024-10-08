// command.h - Header file for the command parser (command.c)

#ifndef COMMAND_H
#define COMMAND_H

// Includes
#include <libk_reduced/stdint.h> // Integer declarations
#include <libk_reduced/string.h> // String functions
#include <kernel/terminal.h> // printf()

// Typedefs
typedef int command(int argc, char *args[]); // This is the command FUNCTION - the thing that is called on a command.


// The main command array uses cmdData. The reason we use cmdData instead of command is because we need the NAME of the command.
// Comparisons would be pretty hard without a command name.
typedef struct {
    char *cmdName;
    command *cmdFunc;
} cmdData;


// Functions
int parseCommand(char *cmd);
void registerCommand(char *name, command cmd);
void initCommandHandler();

#endif
