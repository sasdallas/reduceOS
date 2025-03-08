#include <unistd.h>

void main() {
	open("/device/initrd/test_app", O_RDONLY);
	for (;;);
}
