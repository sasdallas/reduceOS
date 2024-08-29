// ksym.h - header file for kernel symbol table

#ifndef KSYM_H
#define KSYM_H

// Includes
#include <libk_reduced/stdint.h>
#include <kernel/vfs.h>

// Typedefs



typedef struct {
    char *symname;
    void *address;
} ksym_symbol_t;

// Functions
void ksym_init();
int ksym_bind_symbols(fsNode_t *symbolTable);
void *ksym_lookup_addr(const char *name);
int ksym_find_best_symbol(long addr, ksym_symbol_t *out);
hashmap_t *ksym_get_hashmap();

#endif
