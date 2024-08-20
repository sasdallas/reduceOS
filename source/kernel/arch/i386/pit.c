// =====================================================================
// pit.c - Programmable Interval Timer
// This file handles setting up the Programmable Interval Timer (PIT)
// =====================================================================

#include <kernel/pit.h> // Main header file
#include <kernel/process.h>

static volatile uint64_t pitTicks = 0; // Total PIT ticks
static bool pit_isInit = false; // Is the PIT initialized?


// PIT timer interrupt handler
void pitIRQ(registers_t *reg) { 
    pitTicks++; // Increment tick count
    if (terminalMode == 1) updateTextCursor_vesa(); // To be replaced with some sort of handler/caller list

    // Update the clock
    clock_update();

    // Acknowledge the IRQ (ISR has hardcoded values to not acknowledge specifically PIT IRQ)
    interruptCompleted(reg->int_no); // XXX: Stop hardcoding

    // If we are in kernel mode we should do whatever we need to do to launch the usermode process
    if (reg->cs == 0x08) return;
    
    process_switchTask(1);
}



// Waits seconds.
void pitWaitSeconds(int seconds) {
    int secondsPassed = 0;
    do {
        if (pitTicks % 100 == 0) secondsPassed++;
    } while (secondsPassed < seconds);
}


// uint64_t pitSetTickCount(uint64_t i) - Sets a new tick count and returns the previous one.
uint64_t pitSetTickCount(uint64_t i) {
    uint64_t ret = pitTicks;
    pitTicks = i;
    return ret;
}

// uint64_t pitGetTickCount() - Returns tick count
uint64_t pitGetTickCount() { return pitTicks; }

// void pitInit() - Initialize PIT
void pitInit() {

    // Install our interrupt handler (IRQ 0 uses INT 32)
    isrRegisterInterruptHandler(32, pitIRQ);

    // Update the isInitialized variable.
    pit_isInit = true;

    // We want to add some extra code to fully initialize the PIT properly.
    // The default what we want to is to enable the PIT with a square wave generator and 100hz frequencey.
    // If the user wants they can use one of the above functions to do so, but we're going to do it here.
    // Setup the PIT for 100 hz.
    // TODO: Remove above functions.



    int divisor = 1193180 / 1000; // Calculate divisor.
    outportb(PIT_REG_COMMAND, 0x36); // Command byte (0x36)
    outportb(0x40, divisor & 0xFF); // Set the low and high byte of the divisor.
    outportb(0x40, (divisor >> 8) & 0xFF);

    printf("Programmable Interval Timer initialized.\n");
    serialPrintf("pit: Initialized successfully.\n");
}
