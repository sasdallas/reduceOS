// =============================================================================================
// anniversary.c - The file that holds all of reduceOS's anniversary functions and stuff.
// =============================================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

// Before I get to the content, I have a note for all you source code readers :)
/*
reduceOS was a journey I started back in the beginning of 2022. That's probably not that long ago, but who knows.
Back then, I just copied and pasted whatever code I found (mostly from pritamzope, thanks for being so chill about it bro).
This resulted in an unbuildable base, with me having no knowledge of what to do. I later gave up and started from scratch.
Around September of 2022, I picked up reduceOS again, for the rewrite, and that brings us to today.
I could have never dreamed that one of my goals as a very young child would finally be fufilled.
Thank you to everyone who helped support my dream, whether directly or indirectly.
Thank you to all the sources used to make this rewrite.
Thank you to my friends and family for supporting me on this dream.
I hope you enjoy reduceOS, the rewrite.

(P.S: Don't look in this file if you don't want spoilers!)
*/


#include "include/anniversary.h" // Anniversary header file.

static char thankYous[6][1000] = {
    "pritamzope (most of the code for reduceOS alpha)                                ",
    "BrokenThorn Entertainment (original bootloader and some misc pieces of reduceOS)",
    "James Molloy (for his kernel development tutorials and excellent paging driver) ",
    "The OSdev wiki (for almost everything I know)                                   ",
    "All of my friends and family (for their support)                                ",
    "You :) (for trying reduceOS out)                                                "
};


// DONT LOOK AT THIS FILE FOR SPOILERS! I made sure the easter eggs are at the bottom just in case.

// anniversary() - the main command, prints some special unicode and more.
int anniversary(int argc, char *argv) {
    if (argc > 1) {
        if (!strcmp(argv[1], "help")) {
            printf("reduceOS 1.0 anniversary edition - anniversary command\n");
            printf("Available special commands (sub commands to anniversary):\n");
            printf("- placeholder1\n");
            printf("- placeholder2\n");
            printf("- placeholder3\n");
            printf("- placeholder4\n");
        }
    }

    // Clear the screen.
    clearScreen(vgaColorEntry(COLOR_WHITE, COLOR_CYAN));
    
    // Print the reduceOS logo
    printf("\n%s", anniversaryArt1);
    printf("reduceOS anniversary edition - version 1.0-rewrite\n");
    printf("Written by sasdallas.\n\n");

    sleep(500);

    printf("Thank you to:\n");
    sleep(100);
    for (int i = 0; i < 6; i++) {
        printf("%s", thankYous[i]);
        sleep(500);
        printf("\r");
    }

    clearScreen(vgaColorEntry(COLOR_WHITE, COLOR_CYAN));
    printf("\n\n\n%s", anniversaryArt1);
    printf("reduceOS anniversary edition - version 1.0-rewrite\n");
    printf("Written by sasdallas.\n\n");
    printf("%s\n", myMessage);

    sleep(1000);
    
    clearScreen(vgaColorEntry(COLOR_WHITE, COLOR_CYAN));
    printf("\n\n\n%s", anniversaryArt1);
    printf("reduceOS anniversary edition - version 1.0-rewrite\n");
    printf("Written by sasdallas.\n\n");
    printf("\n\n%s", finalMessage);
    keyboardGetKey('\e', false);
    clearScreen(vgaColorEntry(COLOR_WHITE, COLOR_CYAN));
    
    return 1;
}


// anniversaryRegisterCommands() - Registers all the anniversary commands with their respective names.
void anniversaryRegisterCommands() {
    registerCommand("anniversary", (command*)anniversary);
}






// SPOILERS BELOW!!!!!!!




















static int eggTimes = 0;


int easterEggOne(int argc, char *argv) {
    printf("%s\n", egg1messages[eggTimes]);
    
    eggTimes++;
    if (eggTimes == 18) {
        // Maximum amount of messages
        sleep(500);
        panic("anniversary", "easterEggOne", "terminated due to having enough.");
        return 0; // Unachievable.
    }
    return 1;
}


int easterEggTwo(int argc, char *argv) {
    printf("That's mean :(\n");
    return 1;
} 




void anniversaryRegisterEasterEggs() {
    registerCommand("why", (command*)easterEggOne);
    registerCommand("die", (command*)easterEggTwo);
}