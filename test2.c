
int main(int argc, char **argv) {
	// Syscall to sleep for 4 seconds
	int ret;
	while (1) {
		asm volatile ("int $0x80" : "=a"(ret) : "a"(5));
		asm volatile ("int $0x80" : "=a"(ret) : "a"(0));
	}
}

