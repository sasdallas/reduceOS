// mod.h - structure for modules

#ifndef MOD_H
#define MOD_H

// Typedefs
struct Metadata {
    char *name;
    int (*init)(int argc, char *argv[]);
};

#endif