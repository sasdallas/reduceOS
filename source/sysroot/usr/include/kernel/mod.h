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

// Function
typedef int mod_init_func(int argc, char **args);

// Definitions
#define MODULE_ADDR_START 0xA0000000

// Errors
#define MODULE_LOAD_ERROR   -1          // ELF load failed
#define MODULE_CONF_ERROR   -2          // Configuration invalid
#define MODULE_META_ERROR   -3          // No metadata found
#define MODULE_INIT_ERROR   -3          // No init symbol found (backup)
#define MODULE_PARAM_ERROR  -4          // You specified incorrect parameters (dunce)
#define MODULE_READ_ERROR   -5          // Failed to read the module


#endif
