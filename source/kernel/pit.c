// =====================================================================
// pit.c - Programmable Interval Timer
// This file handles setting up the Programmable Interval Timer (PIT)
// =====================================================================

#include "include/pit.h" // Main header file

static volatile uint32_t pitTicks = 0; // Total PIT ticks
static bool pit_isInit = false; // Is the PIT initialized?



// PIT timer interrupt handler
void i86_pitIRQ() {
    
    
    pitTicks++; // Increment tick count

    // Notify HAL we are done.
    interruptCompleted(0);

    
}


// uint32_t i86_pitSetTickCount(uint32_t i) - Sets a new tick count and returns the previous one.
uint32_t i86_pitSetTickCount(uint32_t i) {
    uint32_t ret = pitTicks;
    pitTicks = i;
    return ret;
}

// uint32_t i86_pitGetTickCount() - Returns tick count
uint32_t i86_pitGetTickCount() { return pitTicks; }

// void i86_pitSendCommand(uint8_t cmd) - Sends a command to PIT
void i86_pitSendCommand(uint8_t cmd) {
    outportb(I86_PIT_REG_COMMAND, cmd);
}

// void i86_pitSendData(uint16_t data, uint8_t counter) - Send data to a counter
void i86_pitSendData(uint16_t data, uint8_t counter) {
    uint8_t port = (counter == I86_PIT_OCW_COUNTER_0) ? I86_PIT_REG_COUNTER0:
        ((counter==I86_PIT_OCW_COUNTER_1) ? I86_PIT_REG_COUNTER1 : I86_PIT_REG_COUNTER2);
    
    outportb(port, (uint8_t) data); // Write the data.
}

// uint8_t i86_pitReadData(uint16_t counter) - Reads data from a counter
uint8_t i86_pitReadData(uint16_t counter) {
    uint8_t port = (counter == I86_PIT_OCW_COUNTER_0) ? I86_PIT_REG_COUNTER0:
        ((counter==I86_PIT_OCW_COUNTER_1) ? I86_PIT_REG_COUNTER1 : I86_PIT_REG_COUNTER2);
    
    return inportb(port); // Return the read data.
}


// void i86_pitStartCounter(uint32_t freq, uint8_t counter, uint8_t mode) - Starts a counter from the provided parameters.
void i86_pitStartCounter(uint32_t freq, uint8_t counter, uint8_t mode) {
    if (freq == 0) return; // If the frequency is 0, return.

    uint16_t divisor = (uint16_t) (1193181 / (uint16_t)freq);

    // Sending the operational command
    uint8_t ocw = 0;
    ocw = (ocw & ~I86_PIT_OCW_MASK_MODE) | mode;
    ocw = (ocw & ~I86_PIT_OCW_MASK_RL) | I86_PIT_OCW_RL_DATA;
    ocw = (ocw & ~I86_PIT_OCW_MASK_COUNTER) | counter;
    i86_pitSendCommand(ocw);

    // Set frequency rate
    i86_pitSendData(divisor & 0xff, 0);
    i86_pitSendData((divisor >> 8) & 0xff, 0);

    // Reset tick count
    pitTicks = 0;
}

// void i86_pitInit() - Initialize PIT
void i86_pitInit() {
    // Install our interrupt handler (IRQ 0 uses INT 32)
    setVector(32, i86_pitIRQ);

    // Update the isInitialized variable.
    pit_isInit = true;
    
    printf("Programmable Interval Timer initialized.\n");
}