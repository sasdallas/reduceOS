// =========================================================
// dma.c - Direct Memory Addressing
// =========================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/dma.h> // Main header file

// NOTE: This file is NOT for PCI Busmastering DMA, rather ISA DMA
// Also, ISA DMA is pretty useless (OSDev wiki refers to it as the fitting "appendix of the PC")
// But whatever lol
// A lot of DMA registers are useless, like the command register or the status register, so they are not included and do not have interface functions.


// dma_setStartAddress(uint8_t channel, uint8_t addrLow, uint8_t addrHi) - Set the starting address for the DMA controller
void dma_setStartAddress(uint8_t channel, uint8_t addrLow, uint8_t addrHi) {
    // Ugly switch statement to determine the correct DMA port

    uint8_t dma_port = 0;
    switch (channel) {
        case 0: dma_port = DMA_SLAVE_STARTADDR0_REG; break;
        case 1: dma_port = DMA_SLAVE_STARTADDR1_REG; break;
        case 2: dma_port = DMA_SLAVE_STARTADDR2_REG; break;
        case 3: dma_port = DMA_SLAVE_STARTADDR3_REG; break;
        case 4: dma_port = DMA_MASTER_STARTADDR4_REG; break;
        case 5: dma_port = DMA_MASTER_STARTADDR5_REG; break;
        case 6: dma_port = DMA_MASTER_STARTADDR6_REG; break;
        case 7: dma_port = DMA_MASTER_STARTADDR7_REG; break;
        default:
            serialPrintf("dma_setStartAddress: Unknown channel %i.\n", channel);
            return;
    }

    // Send the addresses
    outportb(dma_port, addrLow);
    outportb(dma_port, addrHi);
}

// dma_setCount(uint8_t channnel, uint8_t countLow, uint8_t countHi) - Sets the channel count
void dma_setCount(uint8_t channel, uint8_t countLow, uint8_t countHi) {
    // Ugly switch statement to determine the correct DMA port

    uint8_t dma_port = 0;
    switch (channel) {
        case 0: dma_port = DMA_SLAVE_COUNT0_REG; break;
        case 1: dma_port = DMA_SLAVE_COUNT1_REG; break;
        case 2: dma_port = DMA_SLAVE_COUNT2_REG; break;
        case 3: dma_port = DMA_SLAVE_COUNT3_REG; break;
        case 4: dma_port = DMA_MASTER_COUNT4_REG; break;
        case 5: dma_port = DMA_MASTER_COUNT5_REG; break;
        case 6: dma_port = DMA_MASTER_COUNT6_REG; break;
        case 7: dma_port = DMA_MASTER_COUNT7_REG; break;
        default:
            serialPrintf("dma_setCount: Unknown channel %i.\n", channel);
            return;
    }

    // Send the count
    outportb(dma_port, countLow);
    outportb(dma_port, countHi);
}

// dma_setPageAccess(uint8_t channel, uint8_t addr) - Set the page access bits
void dma_setPageAccess(uint8_t channel, uint8_t addr) {
    // Ugly switch statement to determine the correct DMA port

    uint8_t dma_port = 0;
    switch (channel) {
        case 0: dma_port = DMA_CHNL0_PAGEACCESS_REG; break;
        case 1: dma_port = DMA_CHNL1_PAGEACCESS_REG; break;
        case 2: dma_port = DMA_CHNL2_PAGEACCESS_REG; break;
        case 3: dma_port = DMA_CHNL3_PAGEACCESS_REG; break;
        case 4: dma_port = DMA_CHNL4_PAGEACCESS_REG; break;
        case 5: dma_port = DMA_CHNL5_PAGEACCESS_REG; break;
        case 6: dma_port = DMA_CHNL6_PAGEACCESS_REG; break;
        case 7: dma_port = DMA_CHNL7_PAGEACCESS_REG; break;
        default:
            serialPrintf("dma_setCount: Unknown channel %i.\n", channel);
            return;
    }

    // Send the count
    outportb(dma_port, addr);
}


// dma_disableDMAC(int port, bool disabled) - Disables a DMAC (probably doesn't even work)
void dma_disableDMAC(int port, bool disabled) {
    outportb((port ? DMA_MASTER_COMMAND_REG : DMA_SLAVE_COMMAND_REG), disabled & DMA_CMD_ENABLE);
}

// dma_maskChannel(uint8_t channel) - Masks a channel in the DMAC 
void dma_maskChannel(uint8_t channel) {
    if (channel < 4) outportb(DMA_SLAVE_MASKCHANNEL_REG, 4 | channel); // Amazing coding practices here (not). As long as the user passes in a value starting from 0, should work (0100b & 00XXb)
    else outportb(DMA_MASTER_MASKCHANNEL_REG, 4 | (channel - 4));
}

// dma_unmaskChannel(uint8_t channel) - Unmasks a channel in the DMAC
void dma_unmaskChannel(uint8_t channel) {
    if (channel <= 4) outportb(DMA_SLAVE_MASKCHANNEL_REG, channel);
    else outportb(DMA_MASTER_MASKCHANNEL_REG, channel);
}

// dma_resetFlipFlop(int dma) - Resets the DMA flip flop (b/n 16-bit transfers)
void dma_resetFlipFlop(int dma) {
    outportb((dma) ? DMA_MASTER_RESETFLOP_REG : DMA_SLAVE_RESETFLOP_REG, 0xBA); // You can write any value, any write signals reset the flip flop
}

// dma_resetDMA(int dma) - Resets the DMAC
void dma_resetDMA(int dma) {
    outportb((dma) ? DMA_MASTER_RESETMASTER_REG : DMA_SLAVE_RESETMASTER_REG, 0xBA); // You can write any value, any write signals trigger a reset.
}

// dma_resetMask(int dma) - Resets the masks in the DMAC, unmasking all channels
void dma_resetMask(int dma) {
    outportb((dma) ? DMA_MASTER_MASKRESET_REG : DMA_SLAVE_MASKRESET_REG, 0xBA); // You can write any value, any write signals trigger a reset
}


// (static) dma_setMode(uint8_t channel, uint8_t mode) - Sets mode (note: SHOULD NOT BE CALLED FROM EXTERNAL. USE INTERFACE FUNCTIONS, MARKED AS STATIC)
static void dma_setMode(uint8_t channel, uint8_t mode) {
    int dma_port = (channel < 4) ? 0 : 1; // Slave/Master DMAC
    int chnl = (dma_port == 0) ? channel : channel - 4;

    dma_maskChannel(chnl); // Mask the DMA channel
    outportb((channel < 4) ? DMA_SLAVE_MODE_REG : DMA_MASTER_MODE_REG, chnl | mode);
    dma_resetMask(dma_port); // Unmask all channels
}




// dma_setRead(uint8_t channel) - Puts a channel into read transfer mode
void dma_setRead(uint8_t channel) {
    dma_setMode(channel, DMA_MODE_READTRANSFER | DMA_MODE_SINGLETRANSFER | DMA_MODE_AUTO_MASK);
}

// dma_setWrite(uint8_t channel) - Puts a channel into write transfer mode
void dma_setWrite(uint8_t channel) {
    dma_setMode(channel, DMA_MODE_WRITETRANSFER | DMA_MODE_SINGLETRANSFER | DMA_MODE_AUTO_MASK);
}

// Quick n' dirty DMA pool implementation, DMA processes that use this memory pool can pull their memory.
// NOTE: They cannot return the memory to the pool, as I said, this is quick n' dirty.
// 256 KB should be more than enough for all ISA DMA required processes to get their fair share.

typedef struct {
    uint8_t *next;
    uint8_t *end;
} dmaPool_t;


dmaPool_t *dmaPool = 0;

// dma_initPool(int pool_size) - Initialize a DMA pool of memory, that processes can pull from for their channels
void dma_initPool(int pool_size) {
    dmaPool = pmm_allocateBlocks((sizeof(dmaPool_t) + pool_size) / 4096);    
    
    dmaPool->next = (uint8_t*)&dmaPool[1];
    dmaPool->end = dmaPool->next + pool_size;
}

// dma_allocPool(size_t size) - Gets a chunk from the DMA pool. Note: This chunk is statutory! You cannot put it back!
void *dma_allocPool(size_t size) {
    if ((dmaPool->end - dmaPool->next) < size) return NULL; // That's what happens when you forget about freeing memory chunks.
    void *mem = (void*)dmaPool->next;
    dmaPool->next += size;
    return mem;
}

