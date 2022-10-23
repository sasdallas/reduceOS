// IDT setup

#include "idt.h"
#include "isr.h"
#include "8259_pic.h"

IDT g_idt[NO_IDT_DESCRIPTORS];
IDT_PTR g_idt_ptr;

void idt_set_entry(int index, uint32_t base, uint16_t seg_sel, uint8_t flags) {
    IDT *this = &g_idt[index];

    this->base_low = base & 0xFFFF;
    this->segment_selector = seg_sel;
    this->zero = 0;
    this->type = flags | 0x60;
    this->base_high = (base >> 16) & 0xFFFF;
}

void idt_init() {
    g_idt_ptr.base_address = (uint32_t)g_idt;
    g_idt_ptr.limit = sizeof(g_idt) - 1;
    pic8259_init();

    idt_set_entry(0, (uint32_t)exception_0, 0x08, 0x8E);
    idt_set_entry(1, (uint32_t)exception_1, 0x08, 0x8E);
    idt_set_entry(2, (uint32_t)exception_2, 0x08, 0x8E);
    idt_set_entry(3, (uint32_t)exception_3, 0x08, 0x8E);
    idt_set_entry(4, (uint32_t)exception_4, 0x08, 0x8E);
    idt_set_entry(5, (uint32_t)exception_5, 0x08, 0x8E);
    idt_set_entry(6, (uint32_t)exception_6, 0x08, 0x8E);
    idt_set_entry(7, (uint32_t)exception_7, 0x08, 0x8E);
    idt_set_entry(8, (uint32_t)exception_8, 0x08, 0x8E);
    idt_set_entry(9, (uint32_t)exception_9, 0x08, 0x8E);
    idt_set_entry(10, (uint32_t)exception_10, 0x08, 0x8E);
    idt_set_entry(11, (uint32_t)exception_11, 0x08, 0x8E);
    idt_set_entry(12, (uint32_t)exception_12, 0x08, 0x8E);
    idt_set_entry(13, (uint32_t)exception_13, 0x08, 0x8E);
    idt_set_entry(14, (uint32_t)exception_14, 0x08, 0x8E);
    idt_set_entry(15, (uint32_t)exception_15, 0x08, 0x8E);
    idt_set_entry(16, (uint32_t)exception_16, 0x08, 0x8E);
    idt_set_entry(17, (uint32_t)exception_17, 0x08, 0x8E);
    idt_set_entry(18, (uint32_t)exception_18, 0x08, 0x8E);
    idt_set_entry(19, (uint32_t)exception_19, 0x08, 0x8E);
    idt_set_entry(20, (uint32_t)exception_20, 0x08, 0x8E);
    idt_set_entry(21, (uint32_t)exception_21, 0x08, 0x8E);
    idt_set_entry(22, (uint32_t)exception_22, 0x08, 0x8E);
    idt_set_entry(23, (uint32_t)exception_23, 0x08, 0x8E);
    idt_set_entry(24, (uint32_t)exception_24, 0x08, 0x8E);
    idt_set_entry(25, (uint32_t)exception_25, 0x08, 0x8E);
    idt_set_entry(26, (uint32_t)exception_26, 0x08, 0x8E);
    idt_set_entry(27, (uint32_t)exception_27, 0x08, 0x8E);
    idt_set_entry(28, (uint32_t)exception_28, 0x08, 0x8E);
    idt_set_entry(29, (uint32_t)exception_29, 0x08, 0x8E);
    idt_set_entry(30, (uint32_t)exception_30, 0x08, 0x8E);
    idt_set_entry(31, (uint32_t)exception_31, 0x08, 0x8E);
    idt_set_entry(32, (uint32_t)irq_0, 0x08, 0x8E);
    idt_set_entry(33, (uint32_t)irq_1, 0x08, 0x8E);
    idt_set_entry(34, (uint32_t)irq_2, 0x08, 0x8E);
    idt_set_entry(35, (uint32_t)irq_3, 0x08, 0x8E);
    idt_set_entry(36, (uint32_t)irq_4, 0x08, 0x8E);
    idt_set_entry(37, (uint32_t)irq_5, 0x08, 0x8E);
    idt_set_entry(38, (uint32_t)irq_6, 0x08, 0x8E);
    idt_set_entry(39, (uint32_t)irq_7, 0x08, 0x8E);
    idt_set_entry(40, (uint32_t)irq_8, 0x08, 0x8E);
    idt_set_entry(41, (uint32_t)irq_9, 0x08, 0x8E);
    idt_set_entry(42, (uint32_t)irq_10, 0x08, 0x8E);
    idt_set_entry(43, (uint32_t)irq_11, 0x08, 0x8E);
    idt_set_entry(44, (uint32_t)irq_12, 0x08, 0x8E);
    idt_set_entry(45, (uint32_t)irq_13, 0x08, 0x8E);
    idt_set_entry(46, (uint32_t)irq_14, 0x08, 0x8E);
    idt_set_entry(47, (uint32_t)irq_15, 0x08, 0x8E);
    idt_set_entry(128, (uint32_t)exception_128, 0x08, 0x8E);

    load_idt((uint32_t)&g_idt_ptr);
    asm volatile("sti");
}