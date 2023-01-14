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

/* Another VM86 demo: try to access the floppy by using BIOS calls... */

/* This program seems to run fine on real hardware, but generates a
 * panic in bochs...
 * If you chose "cont" 2 or 3 times, it works...
 * 
 * It seems that the problem is caused by someone (the bochs bios???)
 * doing an outb(0x3f2, 0), that screws up the floppy pretty badly,
 * causing the "FATAL: int13_diskette:f02: ctrl not ready" panic. :(
 * 
 * I am investigating the problem, and this program is a good test case
 */
#include <ll/i386/hw-func.h>
#include <ll/i386/tss-ctx.h>
#include <ll/i386/x-bios.h>
#include <ll/i386/pic.h>
#include <ll/i386/cons.h>
#include <ll/i386/error.h>
#include <ll/i386/x-dosmem.h>
#include <ll/sys/ll/ll-func.h>
#include <ll/stdlib.h>

#define BSIZE 1024


/* The bochs BIOS may call int 0x10 to print some panic to the screen...
 * For the moment, we simply ignore it...
 */
void int0x10(void)
{
    CONTEXT ctx;
    struct tss *vm86_tss;
    
    ctx = ll_context_save();
    vm86_tss = vm86_get_tss();
    if (ctx != X_VM86_TSS) {
       message("Interrupt 0x10 happended in PM!!!\n");
    }

    return;
}

void reflect(DWORD intnum, struct registers r)
{
    struct tss *vm86_tss;
    DWORD *bos;
    DWORD isr_cs, isr_eip;
    WORD *old_esp;
    DWORD *IRQTable_entry;
    CONTEXT ctx;
    int rmint;

    message("Reflect int 0x%lx\n",intnum);
    ctx = ll_context_save();
    vm86_tss = vm86_get_tss();
    bos = (DWORD *)vm86_tss->esp0;
    if (ctx == X_VM86_TSS) {
       message("Interrupt 0x%lx in VM86 context!\n", intnum);
    }

    if ((intnum == 0x15) || (intnum==16)) {
       DWORD ef;

       ef = *(bos - 7);
       ef &= 0xFFFFFFFE;
       *(bos - 7) = ef;
       rmint=intnum;

#if 0 /* Why goto??? */
       goto ccc; /* nothing changes whether we put goto or return */
#endif
       return;
    }
    if ((intnum >= PIC1_BASE) && (intnum < PIC1_BASE + 8)) {
        rmint = intnum - PIC1_BASE + 8;
    } else {
        if ((intnum < PIC2_BASE) || (intnum > PIC2_BASE + 8)) {
	    if (intnum != 0x40) {
                /* Error!!! Should we panic? */
	        message("Error: Trying to reflect 0x%lx (%ld)!!!\n",
			   intnum, intnum);
                return;
	    } else {
                rmint = 0x40;
	    }
        } else {
            rmint = intnum;
	}
    }
    
    old_esp = (WORD *)(*(bos - 6) + (*(bos - 5) << 4));
    *(old_esp - 1) = (WORD)(*(bos - 7)) /*CPU_FLAG_VM | CPU_FLAG_IOPL*/;
    *(old_esp - 2) = (WORD)(*(bos - 8));
    *(old_esp - 3) = (WORD)(*(bos - 9));
    *(bos - 6) -= 6;

    IRQTable_entry = (void *)(0L);
    isr_cs= ((IRQTable_entry[rmint]) & 0xFFFF0000) >> 16;
    isr_eip = ((IRQTable_entry[rmint]) & 0x0000FFFF);
    *(bos - 8) = isr_cs;
    *(bos - 9) = isr_eip;
}

void disk_demo(void)
{
    X_REGS16 ir,or;
    X_SREGS16 sr;
    char*dosptr;

    /* reset */
    ir.h.ah = 0x00;
    ir.h.al = 0;
    ir.h.dl = 0;
    ir.x.di = 0;
    sr.es = 0;
    vm86_callBIOS(0x13, &ir, &or, &sr);
    message("reset finished.\n");

    ir.h.ah = 0x08;
    ir.h.al = 0;
    ir.h.dl = 0;
    ir.x.di = 0;
    sr.es = 0;
    vm86_callBIOS(0x13, &ir, &or, &sr);
    if (or.x.cflag &0x01) {
        message("Failed to get the floppy drive parameters...\n");
	
	return;
    }

    message("Drives: %d\n", or.h.dl);
    message("Drive Type: 0x%x\n", or.h.bl);
    message("Cylinders: %d\n", or.h.ch + ((WORD) (or.h.cl& 0xC0) << 2) + 1);
    message("Sectors: %d\n", or.h.cl & 0x3F);
    message("...");

#if 0
    ir.h.ah = 0x04;
    ir.h.al = 8;       /* Let's verify 8 sectors... */
    ir.h.dl = 0;
    ir.h.ch = 0;       /* Cylonder 0 */
    ir.h.cl = 1;       /* Sector 0 */
    ir.h.dh = 0;       /* Head 0 */
#else

    dosptr = dos_alloc(512);
    ir.h.ah = 2;
    ir.h.al = 1;
    ir.h.ch = 0; /* C */
    ir.h.cl = 1; /* S */
    ir.h.dh = 0; /* H */
    ir.h.dl = 0;
    sr.es = DOS_SEG(dosptr);
    ir.x.bx = DOS_OFF(dosptr);
#endif

    message("Read a sector:");
    vm86_callBIOS(0x13, &ir, &or, &sr);

    message("Bytes: %s\n",&dosptr[0x36]);

    dos_free(dosptr,512);

    if (or.x.cflag &0x01) {
        message("failed...\n");

        return;
    }
    message("Ok!!!\n");
}

int main (int argc, char *argv[])
{
    DWORD sp1, sp2;
    WORD c;
    int i;
    LIN_ADDR b;
    struct multiboot_info *mbi;

    sp1 = get_sp();
    mbi = ll_init();


    if (mbi == NULL) {
        message("Error in LowLevel initialization code...\n");
        l1_exit(-1);
    }

    message("Starting...");
    c = ll_context_save();
    message("CX=%x\n",c);
    for (i = 0; i < 0x4F000; i++);

    dos_mem_init();
    b = dos_alloc(BSIZE);
    vm86_init(b, BSIZE);


    l1_int_bind(0x40, reflect);
    l1_int_bind(0x15, reflect);
    l1_int_bind(0x10, int0x10);
    l1_irq_bind(6, reflect);
    l1_irq_bind(15, reflect);
    l1_irq_bind(14, reflect);
    irq_unmask(6);
    irq_unmask(15);
    irq_unmask(14);

    disk_demo();
    sp2 = get_sp();
    message("End reached!\n");
    message("Actual stack : %lx - ", sp2);
    message("Begin stack : %lx\n", sp1);
    message("Check if same : %s\n",sp1 == sp2 ? "Ok :-)" : "No :-(");

    ll_end();
    return 1;
}
