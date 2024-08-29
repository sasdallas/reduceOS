// ===================================================================================
// monitor.c - A very simple header file to check monitors (none/color/monochrome)
// ===================================================================================

#include <kernel/vga.h>
#include <kernel/hal.h>



extern enum monitor_video_type monitor_getVideoType() {
    return (enum monitor_video_type) (hal_getBIOSArea(BIOS_DETECTED_HARDWARE_OFFSET));
}
