// ================================================
// command.c - reduceOS command parser
// ================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use it.

#include "include/command.h" // Main header file

// Commands in reduceOS must follow this structure:
// The command function is stored in an array of a type called cmdData (this typedef is present in command.h, note that it is different from command)
// The function returns an integer, which can be any value, but mainly only 1 and -1. 
// 1 means success, while -1 means a failure.
// The command can also take a parameter called args (of type char*[])

cmdData cmdFunctions[1024]; // As of now maximum commands is 1024.
static int index = 0; // Index of commands to add.

// Functions

// parseCommand(char *cmd) - Parses a command to get the function to call.
int parseCommand(char *cmd) {
    if (index == 0) return;
    for (int i = 0; i < 1024; i++) {
        cmdData *data = &cmdFunctions[i];
        
        if (strcmp(cmd, data->cmdName) == 0) {
            command *func = data->cmdFunc;
            int ret = func(NULL);
            return ret;
        }
            
        
    }  

    printf("Unknown command - %s\n", cmd);
    return -1;
}



// registerCommand(char *name, command cmd) - Registers a command and stores it in cmdFunctions.
void registerCommand(char *name, command cmd) {
    if (index < 1024) {
        
        cmdData data; // Add the command's name and function to data.
        data.cmdName = name;
        data.cmdFunc = cmd;

        cmdFunctions[index] = data; // Store the command data in array. It will be called in parseCommand.
        index += 1;
    }
}
