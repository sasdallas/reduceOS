// module.h - Exposes the module loader's functions and its typedefs

#ifndef MODULE_H
#define MODULE_H

// Includes
#include <kernel/elf.h>
#include <kernel/mod.h>
#include <kernel/vfs.h>
#include <libk_reduced/stdint.h>

// Typedefs

// Structure created
typedef struct {
    uint32_t load_addr;
    struct Metadata* metadata;
    uint32_t load_size;
    uint32_t file_length;
} loaded_module_t;

// Function
typedef int mod_init_func(int argc, char** args);

// Definitions
#define MODULE_ADDR_START   0xD0000000

// Errors
#define MODULE_OK           0  // Success!
#define MODULE_LOAD_ERROR   -1 // ELF load failed
#define MODULE_CONF_ERROR   -2 // Configuration invalid
#define MODULE_META_ERROR   -3 // No metadata found
#define MODULE_PARAM_ERROR  -4 // You specified incorrect parameters (dunce)
#define MODULE_READ_ERROR   -5 // Failed to read the module
#define MODULE_INIT_ERROR   -6 // Module did not initialize
#define MODULE_EXISTS_ERROR -7 // Module already exists

// Functions
int module_load(fsNode_t* modfile, int argc, char** args, struct Metadata* mdataout); // Loads a module from modfile
void module_parseCFG();
hashmap_t* module_getHashmap();
void module_init();
uint32_t module_getLoadAddress(char* modulename);

#endif
