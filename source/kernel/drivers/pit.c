// =====================================================================
// pit.c - Programmable Interval Timer
// This file handles setting up the Programmable Interval Timer (PIT)
// =====================================================================

#include <kernel/pit.h> // Main header file

static volatile uint32_t pitTicks = 0; // Total PIT ticks
static bool pit_isInit = false; // Is the PIT initialized?



// PIT timer interrupt handler
void pitIRQ() { 
    pitTicks++; // Increment tick count
    if (terminalMode == 1) updateTextCursor_vesa(); // To be replaced with some sort of handler/caller list
}

// Waits seconds.
void pitWaitSeconds(int seconds) {
    int secondsPassed = 0;
    do {
        if (pitTicks % 100 == 0) secondsPassed++;
    } while (secondsPassed < seconds);
}


// uint32_t pitSetTickCount(uint32_t i) - Sets a new tick count and returns the previous one.
uint32_t pitSetTickCount(uint32_t i) {
    uint32_t ret = pitTicks;
    pitTicks = i;
    return ret;
}

// uint32_t pitGetTickCount() - Returns tick count
uint32_t pitGetTickCount() { return pitTicks; }

// void pitSendCommand(uint8_t cmd) - Sends a command to PIT
void pitSendCommand(uint8_t cmd) {
    outportb(PIT_REG_COMMAND, cmd);
}

// void pitSendData(uint16_t data, uint8_t counter) - Send data to a counter
void pitSendData(uint16_t data, uint8_t counter) {
    uint8_t port = (counter == PIT_OCW_COUNTER_0) ? PIT_REG_COUNTER0:
        ((counter==PIT_OCW_COUNTER_1) ? PIT_REG_COUNTER1 : PIT_REG_COUNTER2);
    
    outportb(port, (uint8_t) data); // Write the data.
}



// void pitStartCounter(uint32_t freq, uint8_t counter, uint8_t mode) - Starts a counter from the provided parameters.
void pitStartCounter(uint32_t freq, uint8_t counter, uint8_t mode) {
    if (freq == 0) return; // If the frequency is 0, return.

    uint16_t divisor = (uint16_t) (1193181 / (uint16_t)freq);

    // Sending the operational command
    uint8_t ocw = 0;
    ocw = (ocw & ~PIT_OCW_MASK_MODE) | mode;
    ocw = (ocw & ~PIT_OCW_MASK_RL) | PIT_OCW_RL_DATA;
    ocw = (ocw & ~PIT_OCW_MASK_COUNTER) | counter;
    pitSendCommand(ocw);

    // Set frequency rate
    pitSendData(divisor & 0xff, 0);
    pitSendData((divisor >> 8) & 0xff, 0);

    // Reset tick count
    pitTicks = 0;
}

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
    outportb(0x43, 0x36); // Command byte (0x36)
    outportb(0x40, divisor & 0xFF); // Set the low and high byte of the divisor.
    outportb(0x40, divisor >> 8);

    printf("Programmable Interval Timer initialized.\n");
}
