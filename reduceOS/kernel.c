#include "kernel.h" // Kernel headers
#include "console.h" // Console handling
#include "string.h" // Memory and String handling
#include "gdt.h" // Global Descriptor Table
#include "idt.h" // Interrupt Descriptor Table
#include "keyboard.h" // Keyboard driver

void kernel_main(void) {
	gdt_init();
	idt_init();
	initConsole(COLOR_WHITE, COLOR_BLUE);
	keyboard_init();

	printf("Please enter anything...\n");
	while (1) {
		printf("%c", kb_getchar());
	}
}