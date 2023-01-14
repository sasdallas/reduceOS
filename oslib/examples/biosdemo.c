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

/*	VM86 demo: tries to call BIOS services in vm86 mode... */

#include <ll/i386/hw-func.h>
#include <ll/i386/tss-ctx.h>
#include <ll/i386/x-dosmem.h>
#include <ll/i386/x-bios.h>
#include <ll/i386/cons.h>
#include <ll/i386/error.h>
#include <ll/stdlib.h>

#define T  1000
#if 1
#define WAIT()  for (w = 0; w < 0xFFFFFFFF; w++)
#else
#define WAIT()  for (w = 0; w < 0xFFFFF; w++)
#endif
#define BSIZE 1024
static unsigned long int w;

#define __VM86__

#ifdef __VM86__

void emulate(DWORD intnum, struct registers r)
{
    struct tss *vm86_tss;
    DWORD *bos;
    DWORD isr_cs, isr_eip;
    WORD *old_esp;
    DWORD *IRQTable_entry;
    CONTEXT c = get_tr();
   
    vm86_tss = vm86_get_tss();
    bos = (DWORD *)vm86_tss->esp0;
    if (c == X_VM86_TSS) {
	    /*
	message("Entering ESP: %lx (= 0x%lx?)\n",
		(DWORD)(tos + 9), vm86_tss->esp0);
	message("Old EIP: 0x%lx    0x%lx\n", *(tos + 9), *(bos - 9));
	message("Old CS: 0x%x    0x%x\n", (WORD)(*(tos + 10)), (WORD)*(bos - 8));
	message("Old EFlags: 0x%lx    0x%lx\n", *(tos + 11), *(bos - 7));
	message("Old ESP: 0x%lx    0x%lx\n", *(tos + 12), *(bos - 6));
        message("Emulate, please!!!\n");
*/	
	old_esp = (WORD *)(*(bos - 6) + (*(bos - 5) << 4));
#if 0
	*(old_esp - 1) = /*(WORD)(*(bos - 7))*/ CPU_FLAG_VM | CPU_FLAG_IOPL;
#endif
	r.flags = CPU_FLAG_VM | CPU_FLAG_IOPL;
	*(old_esp - 2) = (WORD)(*(bos - 8));
	*(old_esp - 3) = (WORD)(*(bos - 9));
	*(bos - 6) -= 6;
	/* We are emulating INT 0x6d */
	IRQTable_entry = (void *)(0L);
	isr_cs= ((IRQTable_entry[0x6d]) & 0xFFFF0000) >> 16;
	isr_eip = ((IRQTable_entry[0x6d]) & 0x0000FFFF);
	/*
	message("I have to call 0x%lx:0x%lx\n", isr_cs, isr_eip);
	*/
	*(bos - 8) = isr_cs;
	*(bos - 9) = isr_eip;
    }
}

void vm86BIOSDemo(void)
{
    X_REGS16 ir,or;
    X_SREGS16 sr;
    /* Print ASCII character */
    ir.h.ah = 9;
    /* AL = character to display */
    ir.h.al = '+';
    /* BH = page number, BL = attribute */
    ir.x.bx = 3;
    /* Count number */
    ir.x.cx = 3;
    vm86_callBIOS(0x10,&ir,&or,&sr);
}
#else

void XBIOSDemo(void)
{
    X_REGS16 ir,or;
    X_SREGS16 sr;
    /* Set video mode */
    ir.h.ah = 9;
    ir.h.al = '+';
    ir.x.bx = 3;
    ir.x.cx = 3;
    X_callBIOS(0x10,&ir,&or,&sr);
}                                                                            
#endif

void BIOSDemo(void)
{
    X_REGS16 ir,or;
    X_SREGS16 sr;
    register int i;
    /* Set video mode */
    ir.h.ah = 0;

#if 0
    ir.h.al = 0x03;
    vm86_callBIOS(0x10,&ir,&or,&sr);

	ir.h.ah = 0x0C;
	ir.h.al = i % 16;
	ir.x.bx = 0;
	ir.x.dx = i+40;
	ir.x.cx = i+100;
	vm86_callBIOS(0x10,&ir,&or,&sr);


#else
    ir.h.al = 0x12;
    vm86_callBIOS(0x10,&ir,&or,&sr);
    /* Put some pixels */
    for (i = 0; i < 200; i++) {
	ir.h.ah = 0x0C;
	ir.h.al = i % 16;
	ir.x.bx = 0;
	ir.x.dx = i+40;
	ir.x.cx = i+100;
	vm86_callBIOS(0x10,&ir,&or,&sr);
    }
#endif
    WAIT();
    /* Set video mode */
    ir.h.ah = 0;
    ir.h.al = 0x03;    
    vm86_callBIOS(0x10,&ir,&or,&sr);
}

int main (int argc, char *argv[])
{
    DWORD sp1, sp2;
    WORD c;
    int i;
    struct multiboot_info *mbi;
    LIN_ADDR b;

    sp1 = get_sp();
    mbi = l1_init();

    if (mbi == NULL) {
	    message("Error in LowLevel initialization code...\n");
	    l1_exit(-1);
    }
    
    message("Starting...");
    c = ll_context_save();
    message("CX=%x\n",c);
    for (i = 0; i < 0x4F000; i++);
#ifdef __VM86__
    dos_mem_init();
    b = dos_alloc(BSIZE);
    vm86_init(b, BSIZE);

    l1_int_bind(0x6d, emulate);
    /*
    l1_irq_bind(0x6d, emulate);
    */

    BIOSDemo();
#else
    XBIOSDemo();
#endif
    sp2 = get_sp();
    message("End reached!\n");
    message("Actual stack : %lx - ", sp2);
    message("Begin stack : %lx\n", sp1);
    message("Check if same : %s\n",sp1 == sp2 ? "Ok :-)" : "No :-(");
    
    l1_end();
    return 1;
}
