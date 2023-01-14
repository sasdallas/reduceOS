/* Project:     OSLib
 * Description: The OS Construction Kit
 * Date:                1.6.2000
 * Idea by:             Luca Abeni & Gerardo Lamastra
 *
 * OSLib is an SO project aimed at developing a common, easy-to-use
 * low-level infrastructure for developing OS kernels and Embedded
 * Applications; it partially derives from the HARTIK project but it
 * currently is independently developed.
 *
 * OSLib is distributed under GPL License, and some of its code has
 * been derived from the Linux kernel source; also some important
 * ideas come from studying the DJGPP go32 extender.
 *
 * We acknowledge the Linux Community, Free Software Foundation,
 * D.J. Delorie and all the other developers who believe in the
 * freedom of software and ideas.
 *
 * For legalese, check out the included GPL license.
 */

/*	CPU Detection test file			*/

#include <ll/i386/stdlib.h>
#include <ll/i386/error.h>
#include <ll/i386/mem.h>
#include <ll/i386/hw-arch.h>
#include <ll/sys/ll/ll-func.h>

#define T  1000

int main (int argc, char *argv[])
{
    DWORD sp1, sp2;
    struct ll_cpu_info cpu;
    void *res;
    char vendor_name[13];

    sp1 = get_sp();
    cli();
    res = ll_init();

    if (res == NULL) {
	    message("Error in LowLevel initialization code...\n");
	    sti();
	    l1_exit(-1);
    }
    sti();

    message("LowLevel started...\n");
    message("CPU informations:\n");
    x86_get_cpu(&cpu);

    message("\tCPU type: %lu\n", cpu.x86_cpu);

    if (cpu.x86_cpu_id_flag) {
    	message("CPUID instruction workin'...\n");
	message("\tCPU Vendor ID: ");

	memcpy(vendor_name, &(cpu.x86_vendor_1), 12);
	vendor_name[12] = 0;
	message("%s\n", vendor_name);

	message("\tCPU Stepping ID: %lx", cpu.x86_cpu & 0x0F);
	message("\tCPU Model: %lx", (cpu.x86_cpu >> 4) & 0x0F);
	message("\tCPU Family: %lx", (cpu.x86_cpu >> 8) & 0x0F);
	message("\tCPU Type: %lx\n", (cpu.x86_cpu >> 12) & 0x0F);

	message("\tStandard Feature Flags %lx\n", cpu.x86_standard_feature);
	if (cpu.x86_standard_feature & 0x01) {
		message("Internal FPU present...\n");
	}
	if (cpu.x86_standard_feature & 0x02) {
		message("V86 Mode present...\n");
	}
	if (cpu.x86_standard_feature & 0x04) {
		message("Debug Extension present...\n");
	}
	if (cpu.x86_standard_feature & 0x08) {
		message("4MB pages present...\n");
	}
	if (cpu.x86_standard_feature & 0x10) {
		message("TimeStamp Counter present...\n");
	}
	if (cpu.x86_standard_feature & 0x20) {
		message("RDMSR/WRMSR present...\n");
	}
	if (cpu.x86_standard_feature & 0x40) {
		message("PAE present...\n");
	}
	if (cpu.x86_standard_feature & 0x80) {
		message("MC Exception present...\n");
	}
	if (cpu.x86_standard_feature & 0x0100) {
		message("CMPXCHG8B present...\n");
	}
	if (cpu.x86_standard_feature & 0x0200) {
		message("APIC on chip...\n");
	}
	if (cpu.x86_standard_feature & 0x1000) {
		message("MTRR present...\n");
	}
	if (cpu.x86_standard_feature & 0x2000) {
		message("Global Bit present...\n");
	}
	if (cpu.x86_standard_feature & 0x4000) {
		message("Machine Check present...\n");
	}
	if (cpu.x86_standard_feature & 0x8000) {
		message("CMOV present...\n");
	}
	if (cpu.x86_standard_feature & 0x800000) {
		message("MMX present...\n");
	}
    }
    cli();
    ll_end();
    sp2 = get_sp();
    message("End reached!\n");
    message("Actual stack : %lx - ", sp2);
    message("Begin stack : %lx\n", sp1);
    message("Check if same : %s\n",sp1 == sp2 ? "Ok :-)" : "No :-(");

    return 1;
}
