#include "io_ports.h"


uint8 inportb(uint16 port) {
    uint8 ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outportb(uint16 port, uint8 val) {
    asm volatile("outb %1, %0" :: "dN"(port), "a"(val));
}

uint16 inports(uint16 port) {
    uint16 rv;
    asm volatile ("inw %1, %0" : "=a" (rv) : "dN" (port));
    return rv;
}
outports(uint16 port, uint16 data) {
    asm volatile ("outw %1, %0" : : "dN" (port), "a" (data));
}


uint32 inportl(uint16 port) {
    uint32 rv;
    asm volatile ("inl %%dx, %%eax" : "=a" (rv) : "dN" (port));
    return rv;
}


void outportl(uint16 port, uint32 data) {
    asm volatile ("outl %%eax, %%dx" : : "dN" (port), "a" (data));
}