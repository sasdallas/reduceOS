// mod.h - header file for reduceOS modules

#ifndef MOD_H
#define MOD_H

// NO INCLUDES ARE IN THIS FILE, BECAUSE IT IS INCLUDED BY THE MODULES THEMSELVES.

// Typedefs

// Metadata exposed by the module and picked up by the loader
struct Metadata {
    char *name;
    char *description;
    int (*init)(int argc, char *argv[]);
    int (*deinit)();
};

#endif