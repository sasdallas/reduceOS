// anniversary.h - header file for the reduceOS anniversary handler
// There's a note in the source code, if you want to read it :)


#ifndef ANNIVERSARY_H
#define ANNIVERSARY_H

// Includes
#include "include/libc/stdint.h" // Integer definitions
#include "include/libc/sleep.h" // Sleep function
#include "include/command.h" // Command functions
#include "include/keyboard.h" // Keyboard functions
#include "include/terminal.h" // Terminal functions.
// Variable definitions

// Pixel art of reduceOS (the art looks ugly because I had to do double slashes)
static char *anniversaryArt1 = "              \n\
               _                 ____   _____ \n\
              | |               / __ \\ / ____|\n\
  _ __ ___  __| |_   _  ___ ___| |  | | (___  \n\
 | '__/ _ \\/ _` | | | |/ __/ _ \\ |  | |\\___ \\ \n\
 | | |  __/ (_| | |_| | (_|  __/ |__| |____) |\n\
 |_|  \\___|\\__,_|\\__,_|\\___\\___|\\____/|_____/ \n\
\n\n";


// The message displayed when anniversary is called
static char *myMessage = "Thank you everyone, for all your support.\n\
When I started this project, I never expected to learn so much about OS development.\n\
I went from copying and pasting to being a full-fledged developer\n\
This was an amazing journey. Thanks for joining me on it.\n\
I hope you enjoy!\n\
";

static char *finalMessage = "That is all from this command!\n\
If you would like to see some of the special features embedded in reduceOS, try running anniversary help\n\
I hope you enjoy my operating system! If you find any bugs or have any suggestions, feel free to let me know!\n\
Press ENTER to continue...\n\
\n";


static char egg1messages[18][255] = {
    "Because an error occurred.",
    "Because that's an error.",
    "Because that's the way you typed it.",
    "Because that's the way the code works.",
    "Because that's the way the developer made it.",
    "Because that's impossible.",
    "Because I said so.",
    "Because yes.",
    "How long are you going to do this?",
    "I'm getting bored.",
    "This is a waste of CPU resources.",
    "Why don't you try another command?",
    "This is pretty stupid, you know that?",
    "Run the help command.",
    "I said run the help command.",
    "Do you even understand directions?",
    "SERIOUSLY?",
    "Fine. Have it your way."
};

// Functions
void anniversaryRegisterCommands(); // Registers all the anniversary commands with their respective names.
void anniversaryRegisterEasterEggs(); // Registers all the easter eggs.

#endif