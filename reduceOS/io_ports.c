#include "io_ports.h"





uint8_t inportb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outportb(uint16_t port, uint8_t val) {
    asm volatile("outb %1, %0" :: "dN"(port), "a"(val));
}

uint16_t inports(uint16_t port) {
    uint16_t rv;
    asm volatile ("inw %1, %0" : "=a" (rv) : "dN" (port));
    return rv;
}
outports(uint16_t port, uint16_t data) {
    asm volatile ("outw %1, %0" : : "dN" (port), "a" (data));
}


uint32_t inportl(uint16_t port) {
    uint32_t rv;
    asm volatile ("inl %%dx, %%eax" : "=a" (rv) : "dN" (port));
    return rv;
}


void outportl(uint16_t port, uint32_t data) {
    asm volatile ("outl %%eax, %%dx" : : "dN" (port), "a" (data));
}