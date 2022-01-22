#ifndef KERNEL_H
#define KERNEL_H

#include "types.h"

// Linker LD symbols
extern uint8 __kernel_section_start;
extern uint8 __kernel_section_end;
extern uint8 __kernel_text_section_start;
extern uint8 __kernel_text_section_end;
extern uint8 __kernel_data_section_start;
extern uint8 __kernel_data_section_end;
extern uint8 __kernel_rodata_section_start;
extern uint8 __kernel_rodata_section_end;
extern uint8 __kernel_bss_section_start;
extern uint8 __kernel_bss_section_end;

#endif