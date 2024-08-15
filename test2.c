
int main(int argc, char **argv) {
	// Syscall to exit
	int ret;
	asm volatile ("int $0x80" : "=a"(ret) : "a"(1), "b"(0));
}

