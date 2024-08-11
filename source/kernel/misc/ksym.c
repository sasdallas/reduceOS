// =================================================================
// ksym.c - Kernel symbol loading
// =================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/ksym.h>


static hashmap_t *ksym_hashmap = NULL;
static bool debug_symbols_populated = false;

// ksym_init() - Initializes the hashmap
void ksym_init() {
    ksym_hashmap = hashmap_create(20);
}


// (static) ksym_bind_symbol(const char *symname, void *addr) - Binds a symbol to the hashmap
static void ksym_bind_symbol(const char *symname, void *addr) {
    if (ksym_hashmap != NULL) {
        hashmap_set(ksym_hashmap, symname, addr);
    }
}


// ksym_bind_symbols(fsNode_t *symbolTable) - Binds symbols to the hashmap
int ksym_bind_symbols(fsNode_t *symbolTable) {
    // First, read the contents into a buffer.
    uint8_t *symbuf = kmalloc(symbolTable->length);
    int ret = symbolTable->read(symbolTable, 0, symbolTable->length, symbuf);
    
    if (ret != symbolTable->length) {
        serialPrintf("ksym: Debugging symbols disabled - reading file failed.\n");
        return -1;
    }

    serialPrintf("ksym_bind_symbols: read file\n");
    

    // The nm tool constructs a symbol map like so:
    // Column 1 - Address of the symbol
    // Column 2 - Type of the symbol (not defined)
    // Column 3 - Name of the symbol
    
    char *save;
    char *save2;
    char *token = strtok_r(symbuf, "\n", &save);

    while (token != NULL) {
        token = strtok_r(NULL, "\n", &save);

        char *token2 = kmalloc(strlen(token));
        strcpy(token2, token);

        if (strstr(token2, " ") == NULL) {
            serialPrintf("ksym_bind_symbols: Early termination, assuming symbols populated.\n");
            break;
        }

        char *address = strtok_r(token2, " ", &save2);
        char *type = strtok_r(NULL, " ", &save2);
        char *symname = strtok_r(NULL, " ", &save2);

        if (!strcmp(type, "T")) {
            void *addr = strtol(address, NULL, 16);
            ksym_bind_symbol(symname, addr);
        }


        kfree(token2); 
    }

    debug_symbols_populated = true;
}

// ksym_lookup_addr(const char *name) - Gets the address needed
void *ksym_lookup_addr(const char *name) {
    if (ksym_hashmap == NULL) return -1;

    return hashmap_get(ksym_hashmap, name);
}

// ksym_find_best_symbol(long addr, ksym_symbol_t *out) - Tries to find the best symbol
int ksym_find_best_symbol(long addr, ksym_symbol_t *out) {
    // NOTE: In this case, addr is instruction pointer

    if (ksym_hashmap == NULL) {
        // Ignore the call
        return -1; // -1 = ksym not initialized
    }

    if (!debug_symbols_populated) {
        // Ignore the call
        return -2; // -2 = ksym no debug symbols
    }

    long best_match = 0;
    char *name;
    for (int i = 0; i < ksym_hashmap->size; i++) {
        hashmap_entry_t *x = ksym_hashmap->entries[i];
        while (x) {
            void *symbol_addr = x->value;
            char *symbol_name = x->key;
            if ((long)symbol_addr < addr && (long)symbol_addr > best_match) {
                best_match = (long)symbol_addr;
                name = symbol_name;
            }
            x = x->next;
        }
    }   

    out->address = best_match;
    out->symname = kmalloc(strlen(name)); 
    strcpy(out->symname, name);

    return 0;
}
