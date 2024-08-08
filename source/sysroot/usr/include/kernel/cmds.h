// cmds.h - header file

#ifndef CMDS_H
#define CMDS_H

// Includes
#include <kernel/kernel.h> // Steal everything from the kernel includes file (very bad practice)

// Functions
// I'm too tired to put them here, gcc will deal with it


int testISRException(int argc, char *args[]);
int getSystemInformation(int argc, char *args[]);
int echo(int argc, char *args[]);
int crash(int argc, char *args[]);
int pciInfo(int argc, char *args[]);
int getInitrdFiles(int argc, char *args[]);
int ataPoll(int argc, char *args[]);
int panicTest(int argc, char *args[]);
int readSectorTest(int argc, char *args[]);
int shutdown(int argc, char *args[]);
int memoryInfo(int argc, char *args[]);
int dump(int argc, char *args[]);
int about(int argc, char *args[]);
int color(int argc, char *args[]);
int doPageFault(int argc, char *args[]);
int read_floppy(int argc, char *args[]);


int mountFAT(int argc, char *args[]);
int ls(int argc, char *args[]);
int cd(int argc, char *args[]);
int cat(int argc, char *args[]);
int create(int argc, char *args[]);
int mkdir(int argc, char *args[]);
int pwd(int argc, char *args[]);
int show_bitmap(int argc, char *args[]);
int edit(int argc, char *args[]);
int rm(int argc, char *args[]);



#endif
