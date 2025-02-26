
void _start() {
	int a = 50;
	char buf[127];
	buf[4] = 'a';

	// Syscall
	asm volatile ("int $128" :: "a"(12345678));

	for (;;);
}
