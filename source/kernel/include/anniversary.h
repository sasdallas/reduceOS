// anniversary.h - header file for the reduceOS anniversary handler
// There's a note in the source code, if you want to read it :)


#ifndef ANNIVERSARY_H
#define ANNIVERSARY_H

// Includes
#include "include/libc/stdint.h" // Integer definitions
#include "include/command.h" // Command functions
#include "include/keyboard.h" // Keyboard functions

// Variable definitions

// Pixel art of reduceOS
char *anniversaryArt1 = "                       \
               _                 ____   _____ \n\
              | |               / __ \ / ____|\n\
  _ __ ___  __| |_   _  ___ ___| |  | | (___  \n\
 | '__/ _ \/ _` | | | |/ __/ _ \ |  | |\___ \ \n\
 | | |  __/ (_| | |_| | (_|  __/ |__| |____) |\n\
 |_|  \___|\__,_|\__,_|\___\___|\____/|_____/ \n\
                                                \
";

// Thank you list
char **thankYous = {
    "pritamzope (most of the code for reduceOS alpha)",
    "BrokenThorn Entertainment (original bootloader and some misc pieces of reduceOS)",
    "James Molloy (for his kernel development tutorials and excellent paging driver)",
    "The OSdev wiki (for almost everything I know)",
    "All of my friends and family (for their support)",
    "You :) (for trying reduceOS out)"
};

// The message displayed when anniversary is called
char *myMessage = "                         \
Thank you everyone, for all your support.\n\
When I started this project, I never expected to learn so much about OS development.\n\
I went from copying and pasting to being a full-fledged developer\n\
This was an amazing journey. Thanks for joining me on it.\n\
I hope you enjoy!\n\
";

char *finalMessage = "\
That is all from this command!\n\
If you would like to see some of the special features embedded in reduceOS, try running anniversary help\n\
I hope you enjoy my operating system! If you find any bugs or have any suggestions, feel free to let me know!\n\
Press ENTER to continue...\n\\";
// Functions



#endif