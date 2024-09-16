// ================================================
// serial.c - reduceOS serial logging driver
// ================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use it.

#include <kernel/serial.h> // Main header file
#include <kernel/clock.h>
#include <kernel/keyboard.h>
#include <libk_reduced/stdio.h>

// Variable declarations
bool serialTestPassed = true; 
bool isSerialEnabled = false;

uint16_t selected_com = SERIAL_COM1;

// gross...
int has_com1 = 0;
int has_com2 = 0;
int has_com3 = 0;
int has_com4 = 0;

// Functions

// (static) serialHasReceived() - Returns if the serial port received anything
static int serialHasReceived() {
    return inportb(selected_com + 5) & 1;
}

// serialRead() - Reads SERIAL_COM1 and returns whatever it found (only returns if serialHasReceived returns 1)
char serialRead() {
    while (serialHasReceived() == 0);
    return inportb(selected_com);
}

// (static) serialIsTransmitEmpty() - Checks if the serial port's transfer is empty. Returns 1 on yes, 0 on no.
static int serialIsTransmitEmpty() {
    return inportb(selected_com + 5) & 0x20; 
}

// serialWrite(void *user, char c) - Writes character 'c' to serial when transmit is empty.
void serialWrite(void *user, char c) {
    while (serialIsTransmitEmpty() == 0);
    
    // Monitors such as QEMU's and any authentic one will need a return carriage before a LF.
    // This is a hack, but it works.
    if (c == '\n') outportb(selected_com, '\r');
    outportb(selected_com, c);
}


void serialClock(unsigned long ticks, unsigned long subticks) {
    // Reads if anything has been received, and if so, prints it out
    if (serialHasReceived()) {

        char c = serialRead();
        if (c == '\r' || c == '\n') {
            serialPrintf("\n"); // Because of the way teletypes work, pressing ENTER (or RETURN) will send the carriage return. 
            keyboardRegisterKeyPress('\n');
        } else {
            serialPrintf("%c", c);
            keyboardRegisterKeyPress(c);
        }
    }
}



// Now, for the actual functions...

// serialPrintf(const char *str, ...) - Prints a formatted line to SERIAL_COM1.
void serialPrintf(const char *str, ...) {
    if (!serialTestPassed) return;

    va_list(ap);
    va_start(ap, str);
    xvasprintf((xvas_callback)serialWrite, NULL, str, ap);
    va_end(ap);
}


int testSerial() {
    // The first thing we need to do to test the serial chip is to set it in loopback mode.
    outportb(selected_com + 4, 0x1E); // Set chip in loopback mode.

    // Now, we send byte 0xAE and check if the serial returns the same byte.
    outportb(selected_com + 0, 0xAE);

    // Read the serial.
    // Note: We do this manually through inportb() because (I think) the serial can hang and not respond, and the serialRead method only reads after the serial was read.
    if (inportb(selected_com + 0) != 0xAE) {
        return -1;     
    }

    // The serial isn't faulty, hooray!
    // Set in normal operations mode (non-loopback, IRQs enabled, OUT #1 and #2 bits enabled)
    outportb(selected_com + 4, 0x0F);
    return 0;
}

// serialInit() - Initializes serial properly.
void serialInit() {
    // TODO: Allow user to choose ports.
    outportb(selected_com + 1, 0x00); // Disable all interrupts
    outportb(selected_com + 3, 0x80); // Enable DLAB (set the baud rate divisor).
    outportb(selected_com + 0, 0x03); // (low byte) Set divisor to 3 - 38400 baud.
    outportb(selected_com + 1, 0x00); // (high byte) Set divisor to 3 - 38400 baud.
    outportb(selected_com + 3, 0x03); // 8 bits, no parity, 1 stop bit.
    outportb(selected_com + 2, 0xC7); // Enable the FIFO, clear them with 14-byte threshold
    outportb(selected_com + 4, 0x0B); // Enable IRQs, set RTS / DSR (RTS stands for "request to send" and DSR stands for "data set ready")
    
    // Test the serial port to make sure it works properly.
    if (testSerial() == -1) {
        printf("Failed to initialize serial logging.\n");
        serialTestPassed = false;
        return;
    }

    serialTestPassed = true;
    isSerialEnabled = true;

    // Create the clock handler for output
    clock_registerCallback(serialClock);

    printf("Serial logging initialized on COM1\n");
    serialPrintf("Serial logging started on COM1.\n");
}


// GROSS
static int serial_isCOMOK(uint16_t com) {
    switch (com) {
        case SERIAL_COM1:
            return has_com1;
        case SERIAL_COM2:
            return has_com2;
        case SERIAL_COM3:
            return has_com3;
        case SERIAL_COM4:
            return has_com4;
        default:
            serialPrintf("serial_isCOMOK: Unadded COM type 0x%x\n", com);
            return 0;
    }
}

static void serial_setCOM(uint16_t com, int value) {
    // Update COM status
    switch (com) {
        case SERIAL_COM1:
            has_com1 = value;
            break;
        case SERIAL_COM2:
            has_com2 = value;
            break;
        case SERIAL_COM3:
            has_com3 = value;
            break;
        case SERIAL_COM4:
            has_com4 = value;
            break;
        default:
            break;
    }

}

// serial_changeCOM(uint16_t com) - Change the COM port to one's liking (0 = success, anything else is fail)
int serial_changeCOM(uint16_t com) {

    // WE ONLY DO THESE TERRIBLE HACKS BECAUSE OF THE FACT THAT THE SERIAL PORT WILL LOCK UP
    // IF IT IS TESTED TOO MUCH

    if (serial_isCOMOK(com) == -1) return -1;

    if (serial_isCOMOK(com) == 0) {
        // Run a test
        if (testSerial() == -1) {
            serial_setCOM(com, -1);
            return -1;
        }
    }

    serial_setCOM(com, 1);
    selected_com = com;

    return 0;
}

// serial_getCOM() - Returns the COM port
uint16_t serial_getCOM() {
    return selected_com;
}
