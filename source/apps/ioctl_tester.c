// ioctl_tester.c - Tests the I/O control part of reduceOS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <kernel/fb.h>

void setup_console() { 
    open("/device/stdin", 0);
    open("/device/console", 1);
    open("/device/console", 1); // stderr will be forced anyways, this doesn't really matter
}

int main(int argc, char **argv) {
    setup_console();
    
    int fb = open("/device/fb0", 0);
    if (!fb) {
        printf("FATAL: Could not successfully open the framebuffer");
        fprintf(stderr, "ioctl_tester: FATAL: Could not open the framebuffer.\n");
        goto done;
    }


    size_t screenHeight, screenWidth, screenDepth, screenPitch;
    int ret = ioctl(fb, FBIOGET_SCREENH, &screenHeight);
    if (ret) {
        printf("FATAL: SCREENH failed");
        fprintf(stderr, "ioctl_tester: FATAL: SCREENH failed.\n");
        goto done;
    }
    
    ret = ioctl(fb, FBIOGET_SCREENDEPTH, &screenDepth);
    if (ret) {
        printf("FATAL: SCREENDEPTH failed");
        fprintf(stderr, "ioctl_tester: FATAL: SCREENDEPTH failed.\n");
        goto done;
    }

    ret = ioctl(fb, FBIOGET_SCREENW, &screenWidth);
    if (ret) {
        printf("FATAL: SCREENW failed");
        fprintf(stderr, "ioctl_tester: FATAL: SCREENH failed.\n");
        goto done;
    }

    ret = ioctl(fb, FBIOGET_SCREENPITCH, &screenPitch);
    if (ret) {
        printf("FATAL: SCREENPITCH failed");
        fprintf(stderr, "ioctl_tester: FATAL: SCREENPITCH failed.\n");
        goto done;
    }
    
    printf("\twidth: %i\n", screenWidth);
    printf("\theight: %i\n", screenHeight);
    printf("\tdepth: %i\n", screenDepth);
    printf("\tpitch: %i\n", screenPitch);


    done:
    for (;;);
}