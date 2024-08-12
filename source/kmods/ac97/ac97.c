// ====================================================
// ac97.c - Driver for the AC'97 soundcard series
// ====================================================
// This module is part of reduceOS. Please credit me if you use this code.

#include "ac97.h" // Main header file
#include <kernel/mod.h>

// A good, high quality impl. of the AC'97 soundcard can be found here (it's where a lot of the header code came from):
// https://github.com/klange/toaruos/blob/master/modules/ac97.c#L48

// A high-quality page detailing the AC'97 and its specifications can be found here:
// https://wiki.osdev.org/AC97#Native_Audio_Bus_Master_registers

static ac97_t *device;


// (static) ac97_find(uint32_t device, uint16_t vendorID, uint16_t deviceID, void *extra) - Scans PCI bus for AC'97 cards and finds them
static void ac97_find(uint32_t device, uint16_t vendorID, uint16_t deviceID, void *extra) {
    ac97_t *ac97 = extra;

    // PCI driver is sort of borked. Need to investigate why.
    serialPrintf("[module ac97] 0x%x 0x%x\n", vendorID, deviceID);
    if ((deviceID == 0x2415)) {
        serialPrintf("[module ac97] Found AC97 device\n");
        ac97->pciDevice = device;
    }
}

// (static) ac97_init(int argc, char *argv[]) - Initialize the AC97 driver
static int ac97_init(int argc, char *argv[]) {
    serialPrintf("[module ac97] Initializing AC97\n");
    device = kmalloc(sizeof(ac97_t));
    device->pciDevice = -1;
    pciScan(&ac97_find, -1, &device);

    if (device->pciDevice == -1) {
        serialPrintf("[module ac97] No AC'97 card was found.\n");
        kfree(device);
        return 0;
    }

    device->IRQ = pciGetInterrupt(device->pciDevice);
    serialPrintf("[module ac97] Device wants IRQ %zd\n", device->IRQ);
    return 0;
}

static int ac97_deinit() {
    serialPrintf("[module ac97] Deinitialized\n");
    return 0;
}

struct Metadata data = {
    .name = "AC97 Driver",
    .description = "Driver for AC'97 soundcards",
    .init = ac97_init,
    .deinit = ac97_deinit
};