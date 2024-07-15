// ================================================
// command.c - reduceOS command parser
// ================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use it.

#include "include/command.h" // Main header file

// Commands in reduceOS must follow this structure:
// The command function is stored in an array of a type called cmdData (this typedef is present in command.h, note that it is different from command)
// The function returns an integer, which can be any value, but mainly only 1 and -1. 
// 1 means success, while -1 means a failure.
// The command can also take a parameter called args (of type char*[]) and argc (of type int)

cmdData cmdFunctions[1024]; // As of now maximum commands is 1024.
static int index = 0; // Index of commands to add.

// Functions
// (static) parseArguments(char *cmd, char ) - Removes spaces and parses any arguments. Returns amount of arguments and sets the actual arguments in *args. Each argument is seperated by a 
static int parseArguments(char *cmd, char ***parsedArguments) {
    // Count number of arguments in the command.
    int commandAmount = 0;
    int charactersFromLastSpace = 0;
    for (int i = 0; cmd[i] != '\0'; i++) {
        if (cmd[i] == ' ' && charactersFromLastSpace >= 1) {
            commandAmount++;
            charactersFromLastSpace = 0;
        } else if (cmd[i] == ' ' && charactersFromLastSpace == 0) {
            // users are attempting to crash the OS by typing only spaces!
            // not on my watch!
            return -1;
        } else {
            charactersFromLastSpace++;
        }
    }

    // Add one for the command itself.
    commandAmount++;

    // Allocate memory for argument array
    *parsedArguments = kmalloc(commandAmount * sizeof(char *));
    if (*parsedArguments == NULL) return -1;

    // Fill the array with arguments
    int i = 0, start = 0;
    for (int j = 0; cmd[j] != '\0'; j++) {
        if (cmd[j] == ' ') {
            int len = j - start;
            (*parsedArguments)[i] = kmalloc((len + 1) * sizeof(char));
            if ((*parsedArguments)[i] == NULL) return -1;
            strcpy((*parsedArguments)[i], &cmd[start]);
            (*parsedArguments)[i][len] = '\0';
            i++;
            start = j + 1;
        }
    }

    int len = strlen(cmd) - start;
    (*parsedArguments)[i] = kmalloc((len + 1) * sizeof(char));
    if ((*parsedArguments)[i] == NULL) return -1;

    strcpy((*parsedArguments)[i], &cmd[start]);
    return commandAmount;
}


// parseCommand(char *cmd) - Parses a command to get the function to call.
int parseCommand(char *cmd) {
    if (index == 0 || strlen(cmd) == 0) return -1;
    
    char **argv;
    int argc = parseArguments(cmd, &argv);

    if (argc == -1) return -1;
   

    for (int i = 0; i < 1024; i++) {
        cmdData *data = &cmdFunctions[i];
        if (strlen(data->cmdName) == strlen(argv[0])) {
            if (strcmp(argv[0], data->cmdName) == 0) {
                command *func = data->cmdFunc;
                int ret;
                ret = func(argc, argv);
                // Free the memory allocated for the parsed command
                for (int arg = 0; arg < argc; arg++) kfree(argv[arg]);
                kfree(argv);
                return ret;
            }
        }
    }  


    printf("Unknown command - %s\n", cmd);
    
}

// Help command - prints all available commands.
int help(int argc, char *args[]) {
    printf("reduceOS v%s - help command\nAvailable commands: ", VERSION);
    
    for (int i = 0; i < index; i++) {
        cmdData *data = &cmdFunctions[i];
        printf("%s, ", data->cmdName);
    }
    
    
    printf("\n");
    return 0;
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
        data.cmdName = "\0";
        data.cmdFunc = NULL;

        cmdFunctions[i] = data;
    }

    // Register help command.
    registerCommand("help", (command*)help);
    printf("Command parser initialized successfully.\n");
}