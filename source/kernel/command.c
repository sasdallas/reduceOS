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

// (static) parseArguments(char *cmd, char *args) - Removes spaces and parses any arguments. Returns amount of arguments and sets the actual arguments in *args. Each argument is seperated by a 
static int parseArguments(char *cmd, char *args[]) {
    char tmp[256]; // tmp stores our current argument
    int argumentAmnt = 0;
    int tmpIndex = 0; // Seperate index is used because if we find an argument, tmp is reset, and so the index needs to be reset.
    
    for (int i = 0; i < strlen(cmd); i++) {
        if (cmd[i] == ' ' && i+1 != strlen(cmd)) {
            // We have an argument!
            args[argumentAmnt] = tmp; // Set the proper value in arguments.
            memset(tmp, 0, sizeof(tmp)); // Clear tmp.
            argumentAmnt++; // Increment the amount of arguments
            tmpIndex = 0; // Reset tmpIndex.
        } else if (cmd[i] != ' ') {
            tmp[tmpIndex] = cmd[i]; // Add the character to tmp and increment tmpIndex.
            tmpIndex++;
        }
    }

    // Done. Return argumentAmnt.
    return argumentAmnt;
}

// parseCommand(char *cmd) - Parses a command to get the function to call.
int parseCommand(char *cmd) {
    if (index == 0 || strlen(cmd) == 0) return;

    for (int i = 0; i < 1024; i++) {
        cmdData *data = &cmdFunctions[i];
        
        if (strcmp(cmd, data->cmdName) == 0) {
            command *func = data->cmdFunc;
            int ret;
            ret = func(NULL);
            return ret;
        }
    }  


    printf("Unknown command - %s\n", cmd);
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



// initCommandHandler() - Inititializes the command handler
void initCommandHandler() {
    for (int i = 0; i < 1024; i++) {
        cmdData data;
        data.cmdName = '\0';
        data.cmdFunc = NULL;

        cmdFunctions[i] = data;
    }

    printf("Command parser initialized successfully.\n");
}