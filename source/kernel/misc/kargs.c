/**
 * @file source/kernel/misc/kargs.c
 * @brief Handles parsing arguments passed by the bootloader to the kernel
 * 
 * This file handles argument parsing for the kernel. It stores them in a hashmap
 * 
 * @copyright
 * This file is part of the reduceOS C kernel, which is licensed under the terms of GPL.
 * Please credit me if this code is used.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include <kernel/args.h>
#include <kernel/hashmap.h>

static hashmap_t *kernel_arguments;


int args_has(char *string) {
    return hashmap_has(kernel_arguments, string);
}

char *args_get(char *key) {
    return hashmap_get(kernel_arguments, key);
}

void args_init(char *arguments) {
    if (kernel_arguments) return; // Already initialized
    kernel_arguments = hashmap_create(10);

    char *save;
    char *save2;
    char *token = strtok_r(arguments, " ", &save); 
    while (token != NULL) {

        
        // Interesting bug where GRUB will append a \024 character. Remove that.
        if (token[strlen(token)-1] == '\024')  token[strlen(token)-1] = '\0';

        
        /**
         * Kernel arguments can be passed in two ways:
         * argument (just this)
         * argument=something
         * Right now we only support one-word long arguments.
         * 
         * For the latter type, we also need to parse until we reach the ending ' or " character
         */

        if (strstr(token, "=") != NULL) {
            // Okay, it has an equals sign, that's cool.
            // We'll need to double parse this. Create a new string that's before the equals sign
            char *saved_token = strdup(token);
            char *key = strtok_r(saved_token, "=", &save2);

            char *value = strtok_r(NULL, "=", &save2);
            
            // pretty simple since we don't support spaces yet (lol)
            // we'll just remove the ' and " - this is normally pretty bad, but idc
            for (int i = 0; i < strlen(value); i++) if (value[i] == '\'') value[i] = '\0';
            for (int i = 0; i < strlen(value); i++) if (value[i] == '\"') value[i] = '\0';

            hashmap_set(kernel_arguments, key, value);
        } else {
            
            hashmap_set(kernel_arguments, token, "N/A"); // should replace the N/A with something actually funny
        }

        token = strtok_r(NULL, " ", &save);
    }
}