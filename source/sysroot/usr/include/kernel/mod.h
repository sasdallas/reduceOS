// mod.h - structure for modules

#ifndef MOD_H
#define MOD_H

// Typedefs
typedef struct {
    char *name;
    int (*init)(int argc, char *argv[]);
} mod_metadata_t;

#endif