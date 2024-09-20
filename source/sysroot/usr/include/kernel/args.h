// args.h - Header file for the kernel arguments header

#ifndef ARGS_H
#define ARGS_H

void args_init(char *arguments);
char *args_get(char *key);
int args_has(char *string);


#endif 