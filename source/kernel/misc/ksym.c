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
    char *symbolbuffer = kmalloc(1024);
    memset(symbolbuffer, 0, 1024);
    int i = 0;
    int symbuf_idx = 0;
    int symcount = 0;
    while (i < symbolTable->length) {
        // BE WARNED: THIS IS BAD

        // First, get the line.
        while (*symbuf != '\n') {
            symbolbuffer[symbuf_idx] = *symbuf;
            *symbuf++;
            symbuf_idx++;
            i++;
            if (symbuf_idx > 1024) {
                if (*symbuf == 0) break; // Bug where it will break on the last character. 
            
                // reduceOS: Preventing buffer overflows since 2022.
                serialPrintf("ksym_bind_symbols: i = %i / %i, symbuf_idx = %i\n", i, symbolTable->length, symbuf_idx);
                panic("ksym", "ksym_bind_symbols", "Detected overflow in symbuf_idx");
            }
        }

        // Sometimes things happen.
        if (strlen(symbolbuffer) <= 1) {
            i++;
            symbuf_idx = 0;
            continue;
        }


        // Get the address as a string
        char *addrstring = kmalloc(strlen(symbolbuffer));
        int pos = -1;
        for (pos = 0; pos < strlen(symbolbuffer); pos++) {
            if (symbolbuffer[pos] != ' ') {
                addrstring[pos] = symbolbuffer[pos];
            } else {
                break;
            }
        }
        addrstring[pos] = 0;

        // Get the type
        pos++;
        char type = symbolbuffer[pos];
        pos += 2;

        // Get the symbol name
        char *symname = kmalloc(strlen(symbolbuffer) - pos + 1);
        memcpy(symname, symbolbuffer + pos, strlen(symbolbuffer) - pos);
        symname[strlen(symbolbuffer) - pos] = 0;

        // Due to some complexities with strtol, it will panic on anything greater than LONG_MAX.
        // This is normally fine since our addresses are within that range.
        // However, there's a small circumstance in which a variable like ENABLE_PAGING (which is 80000001) is too great and will panic it.
        // So we'll instead run a check
        if (type == 'a') {
            // a = absolute, should really add these types
            // Do not register anything that is absolute
            memset(symbolbuffer, 0, 512);
            symbuf_idx = 0;
            *symbuf++;
            continue;
        }
        void *addr = (void*)strtol(addrstring, NULL, 16);
        
        // Bind the symbol
        ksym_bind_symbol(symname, addr);

        // kfree(symname)
        kfree(addrstring);

        // Next iteration
        memset(symbolbuffer, 0, 512);
        symbuf_idx = 0;
        *symbuf++; // Increment symbuf to prevent infinite loop

        serialPrintf("Loaded symbol at %s (address 0x%x)\n", symname, addr);
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
