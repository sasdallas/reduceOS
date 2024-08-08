// ============================================================
// initrd.c - loads the reduceOS initial ramdisk
// ============================================================
// Some code and concepts are from JamesM's kernel development tutorials.

#include <kernel/initrd.h> // Main header file

// First of all, what is an initial ramdisk?
/* Well, an initial ramdisk is a filesystem that is loaded in to memory on boot.
In reduceOS, it stores configuration files, executables, drivers, anything really.
However, it is important to note that an initial ramdisk is not actually a root filesystem.
In fact, it usually contains the drivers to access the root filesystem. 
You cannot delete files in an initrd.
As of January 2nd, 2022, reduceOS's initial ramdisk cannot access subdirectories (we're using James Molloy's adaptation right now and will later switch.)*/


// This initial ramdisk implemenetation is also CURRENTLY based on James Molloy's implementation. I'm working on my own implementation with subdirectory support and more.

// GRUB passes our initrd as a mod - we handle the mod getting and stuff in the kernel C file.

// First of all, begin with our variables.

initrd_imageHeader_t *initrdHeader; // The header for the initrd.
initrd_fileHeader_t *fileHeaders; // A list of file headers
fsNode_t *initrdRoot; // The root directory node
fsNode_t *rootNodes; // List of file nodes
int rootNodes_amount; // Amount of file nodes.

struct dirent dirent;


// Moving on to the functions....



// (static) initrdRead(fsNode_t *node, off_t offset, uint32_t size, uint8_t *buffer) - Read a file from an initrd.
static uint32_t initrdRead(fsNode_t *node, off_t offset, uint32_t size, uint8_t *buffer) {
    initrd_fileHeader_t header = fileHeaders[node->inode];
    if (offset > header.length) return 0;
    if (offset + size > header.length) size = header.length - offset;

    memcpy(buffer, (uint8_t*)(header.offset), size);
    return size;
}



// (static) initrdReaddir(fsNode_t *node, uint32_t index) - Reads a directory in the initrd.
static struct dirent *initrdReaddir(fsNode_t *node, uint32_t index) {
    struct dirent *retval = kmalloc(sizeof(struct dirent));
    
    if (index >= rootNodes_amount) {
        kfree(retval);
        return 0; // Sneaky users trying to muck up our input!
    }

    strcpy(retval->name, rootNodes[index].name);
    retval->name[strlen(rootNodes[index].name)] = 0;
    retval->ino = rootNodes[index].inode;
    return retval;
}


// (static) initrdFinddir(fsNode_t *node, char *name) - Finds a directory in the VFS node *node with the name *name.
static fsNode_t *initrdFinddir(fsNode_t *node, char *name) {
    for (int i = 0; i < rootNodes_amount; i++) {
        if (!strcmp(name, rootNodes[i].name)) {
            // Clone the root node
            fsNode_t *clone = kmalloc(sizeof(fsNode_t));
            memcpy(clone, &rootNodes[i], sizeof(fsNode_t));
            return clone;
        }
    }

    return 0;
}



// We have just one function that is accessible from the outside.
// initrdInit(uint32_t location) - Initializes the initrd image (located at location)
fsNode_t *initrdInit(uint32_t location) {
    // First, initalize the main and file header pointers and populate the root directory.
    initrdHeader = (initrd_imageHeader_t*)location;
    fileHeaders = (initrd_fileHeader_t*) (location + sizeof(initrd_imageHeader_t));

    // TODO: Is the location mapped into memory?

    // Next, initialize the root directory.
    // This is pretty simple. Allocate memory and setup all of the values (most are 0, but some have function pointers)
    initrdRoot = (fsNode_t*)kmalloc(sizeof(fsNode_t));
    strcpy(initrdRoot->name, "initrd");
    initrdRoot->mask = initrdRoot->uid = initrdRoot->gid = initrdRoot->inode = initrdRoot->length = 0;
    initrdRoot->flags = VFS_DIRECTORY;
    initrdRoot->read = 0;
    initrdRoot->write = 0;
    initrdRoot->open = 0;
    initrdRoot->close = 0;
    initrdRoot->readdir = &initrdReaddir;
    initrdRoot->finddir = &initrdFinddir;
    initrdRoot->ptr = 0;
    initrdRoot->impl = 0;


    // reduceOS has a special initial ramdisk structure
    // Each file has a magic number starting at the top.
    // The magic number is specific to each directory the file is in (starting at 0xAA for root directory)
    // Each subdirectory has its own magic number too, 0xBA.
    


    rootNodes = (fsNode_t*)kmalloc(sizeof(fsNode_t) * initrdHeader->fileAmnt);
    rootNodes_amount = initrdHeader->fileAmnt;

    // Iterate through all files and setup each one.
    for (int i = 0; i < initrdHeader->fileAmnt; i++) {
        // Edit the files header (currently holds the offset relative to ramdisk start) to point to file offset relative to size of memory.
        fileHeaders[i].offset += location;
        // Create a new file node.
        strcpy(rootNodes[i].name, &fileHeaders[i].name);
        rootNodes[i].mask = rootNodes[i].uid = rootNodes[i].gid = 0;
        rootNodes[i].length = fileHeaders[i].length;
        rootNodes[i].inode = i;
        rootNodes[i].flags = VFS_FILE;
        rootNodes[i].read = &initrdRead;
        rootNodes[i].write = 0;
        rootNodes[i].readdir = 0;
        rootNodes[i].finddir = 0;
        rootNodes[i].open = 0;
        rootNodes[i].close = 0;
        rootNodes[i].impl = 0;
    }

    return initrdRoot;
} 
