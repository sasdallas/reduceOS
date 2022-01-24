#include "kernel.h" // Kernel headers
#include "console.h" // Console handling
#include "string.h" // Memory and String handling
#include "gdt.h" // Global Descriptor Table
#include "idt.h" // Interrupt Descriptor Table
#include "keyboard.h" // Keyboard driver
#include "vga.h"

void __cpuid(uint32 type, uint32 *eax, uint32 *ebx, uint32 *ecx, uint32 *edx) {
	asm volatile("cpuid"
				: "=a"(*eax), "=b"(*ebx), "=c"(ecx), "=d"(*edx)
				: "0"(type)); // Add type to ea
}

BOOL loadTest = FALSE;





void doTestStuff() {
	initConsole(COLOR_WHITE, COLOR_BLACK);
	draw_box(BOX_SINGLELINE, 28, 1, 38, 20, COLOR_WHITE, COLOR_BLACK);

  	draw_box(BOX_SINGLELINE, 28, 1, 12, 6, COLOR_WHITE, COLOR_BLACK);
  	draw_box(BOX_SINGLELINE, 41, 1, 12, 6, COLOR_WHITE, COLOR_BLACK);
  	draw_box(BOX_SINGLELINE, 54, 1, 12, 6, COLOR_WHITE, COLOR_BLACK);

  	draw_box(BOX_SINGLELINE, 28, 8, 12, 6, COLOR_WHITE, COLOR_BLACK);
  	draw_box(BOX_SINGLELINE, 41, 8, 12, 6, COLOR_WHITE, COLOR_BLACK);
  	draw_box(BOX_SINGLELINE, 54, 8, 12, 6, COLOR_WHITE, COLOR_BLACK);

  	draw_box(BOX_SINGLELINE, 28, 15, 12, 6, COLOR_WHITE, COLOR_BLACK);
  	draw_box(BOX_SINGLELINE, 41, 15, 12, 6, COLOR_WHITE, COLOR_BLACK);
  	draw_box(BOX_SINGLELINE, 54, 15, 12, 6, COLOR_WHITE, COLOR_BLACK);
	
	// Draw instructions
	consoleGoXY(0,0);
	consolePrintColorString("Tic-Tac-Toe v0.1", COLOR_YELLOW, COLOR_BLACK);
	consoleGoXY(0,1);
	consolePrintColorString("Made for reduceOS", COLOR_YELLOW, COLOR_BLACK);

	draw_box(BOX_SINGLELINE, 0, 2, 18, 3, COLOR_GREY, COLOR_BLACK);

	consoleGoXY(1,3);
	consolePrintColorString("Player 1 moves: ", COLOR_BRIGHT_RED, COLOR_BLACK);
	consoleGoXY(1,5);
	consolePrintColorString("Player 2 moves: ", COLOR_CYAN, COLOR_BLACK);

	consoleGoXY(1,7);
	consolePrintColorString("Turn: ", COLOR_CYAN, COLOR_BLACK);
	consoleGoXY(8,7);
	consolePrintColorString("idc enough", COLOR_CYAN, COLOR_BLACK);

	draw_box(BOX_SINGLELINE, 0, 9, 18, 8, COLOR_GREY, COLOR_BLACK);

  	consoleGoXY(1, 9);
  	consolePrintColorString("Keys", COLOR_WHITE, COLOR_BLACK);

  	consoleGoXY(1, 11);
  	consolePrintColorString("Arrows", COLOR_WHITE, COLOR_BLACK);

	consoleGoXY(12, 10);
  	consolePutchar(30);	
	
	consoleGoXY(10, 11);
	consolePutchar(17);
	
	consoleGoXY(14, 11);
	consolePutchar(16);

	consoleGoXY(12, 12);
	consolePutchar(31);
	
	consoleGoXY(1, 14);
	consolePrintColorString("Use spacebar to select", COLOR_WHITE, COLOR_BLACK);
	consoleGoXY(1, 16);
	consolePrintColorString("Mov White Box", COLOR_GREY, COLOR_BLACK);
	consoleGoXY(1, 17);
	consolePrintColorString(" to select cell", COLOR_GREY, COLOR_BLACK);
}

void guiTest(void) {
	clearConsole(COLOR_WHITE, COLOR_BLACK);
	draw_box(BOX_SINGLELINE, 0, 1, 75, 20, COLOR_GREY, COLOR_BLACK);
	draw_box(BOX_SINGLELINE, 0, 4, 75, 4, COLOR_GREY, COLOR_BLACK);

}

void getCPUIDInfo() {
	uint32 brand[12];
	uint32 eax, ebx, ecx, edx;
	uint32 type;

	memset(brand, 0, sizeof(brand));
	__cpuid(0x80000002, (uint32 *)brand+0x0, (uint32 *)brand+0x1, (uint32 *)brand+0x2, (uint32 *)brand+0x3);
    __cpuid(0x80000003, (uint32 *)brand+0x4, (uint32 *)brand+0x5, (uint32 *)brand+0x6, (uint32 *)brand+0x7);
    __cpuid(0x80000004, (uint32 *)brand+0x8, (uint32 *)brand+0x9, (uint32 *)brand+0xa, (uint32 *)brand+0xb);
	printf("System brand: %s\n", brand);
	for (type = 0; type < 4; type++) {
		__cpuid(type, &eax, &ebx, &ecx, &edx);
		printf("Type:0x%x, eax: 0x%x, ebx: 0x%x, ecx: 0x%x, edx:0x%x\n", type, eax, ebx, ecx, edx);
	}
}
BOOL is_echo(char *b) {
    if((b[0]=='e')&&(b[1]=='c')&&(b[2]=='h')&&(b[3]=='o'))
        if(b[4]==' '||b[4]=='\0')
            return TRUE;
    return FALSE;
}

BOOL is_color(char *b) {
	if ((b[0]=='s')&&(b[1]=='e')&&(b[2]=='t')&&(b[3]=='c')&&(b[4]=='o')&&(b[5]=='l')&&(b[6]=='o')&&(b[7]=='r'))
		if (b[8]==' '||b[8]=='\0')
			return TRUE;
	return FALSE;
}

/*
void doGUIStuff(void) {
	clearConsole(COLOR_BLUE, COLOR_BLACK);
	
	vga_disable_cursor();

	setColor(COLOR_BLUE, COLOR_BLACK);
	int t = 1;
	for (int i=0; i<4; i++) {
		consoleGoXY((VGA_WIDTH/2), t);
		for (int i=0; i<4; i++) {
			consolePutchar(' ');
		}
		t++;
	}
	
} */



void kernel_main(void) {
	gdt_init();
	idt_init();
	initConsole(COLOR_WHITE, COLOR_BLUE);
	keyboard_init();

	char buffer[255];
	const char *shell = ">";

	printf("reduceOS v0.2 loaded\n");
	printf("Type help for help...\n");
	while (1) {
		printf(shell); // Print basic shell
		memset(buffer, 0, sizeof(buffer)); // memset buffer to 0
		getStringBound(buffer, strlen(shell));
		if (strlen(buffer) == 0)
			continue;
		if (strcmp(buffer, "about") == 0) {
			printf("reduceOS v0.2\n");
			printf("Build 3\n");
		} else if (strcmp(buffer, "getcpuid") == 0) {
			getCPUIDInfo();
		} else if (strcmp(buffer, "help") == 0) {
			printf("reduceOS shell v0.1\n");
			printf("Commands: help, getcpuid, echo, about\n");
		} else if (is_echo(buffer)) {
			printf("%s\n", buffer + 5);
		} else if (strcmp(buffer, "setcolor") == 0) {
			setColor(COLOR_RED, COLOR_BLUE);
		} else if (strcmp(buffer, "test") == 0) { 
			loadTest = TRUE;
			break;

		} else if (strcmp(buffer, "gui") == 0) {
			guiTest();
		}else {
			printf("Command not found: %s\n", buffer);
		} 
	}
	if (loadTest) {
		doTestStuff();
	}
}
