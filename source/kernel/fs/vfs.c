// =====================================================
// vfs.c - Virtual File System handler
// =====================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/vfs.h" // Main header file

fsNode_t *fs_root = 0; // Root of the filesystem
tree_t *fs_tree = NULL; // Mountpoint tree


/*
    In reduceOS, we follow a UNIX like structure with one device being the root device and having a filesystem tree.
    We structure the root directory like UNIX, where it is /
    Other devices are mounted in /device/
    For things like hard drives, they are mounted in hd01, where 0 is the drive number and 1 is the partition.
    At least, that's the plan ;)
*/

// Functions:

/* Most of these functions are really simple - just check if there's a callback in the node and call it! */

// readFilesystem(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf) - Reads a file in a filesystem.
uint32_t readFilesystem(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf) {
    // This is pretty simple, we just use the callbacks in each node.
    // Check if the node has a callback.
    if (node->read != 0) {
        return node->read(node, off, size, buf);
    } else {
        return 0; // It doesn't.
    }
}

// writeFilesystem(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf) - Writes a file in a filesystem.
uint32_t writeFilesystem(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf) {
    if (node->write != 0) {
        return node->write(node, off, size, buf);
    } else {
        return 0;
    }
}

// openFilesystem(fsNode_t *node, uint8_t read, uint8_t write) - Opens a filesystem.
void openFilesystem(fsNode_t *node, uint8_t read, uint8_t write) {
    if (node->open != 0) {
        return node->open(node);
    }
}

// closeFilesystem(fsNode_t *node) - Closes a filesystem.
void closeFilesystem(fsNode_t *node) {
    if (node->close != 0)
        return node->close(node);
}

// Now it gets slightly more complicated.

// readDirectoryFilesystem(fsNode_t *node, uint32_t index) - Reads a directory in a filesystem.
struct dirent *readDirectoryFilesystem(fsNode_t *node, uint32_t index) {
    // Check if the node is a directory and if it has a callback.
    if ((node->flags & 0x7) == VFS_DIRECTORY && node->readdir != 0) {
        return node->readdir(node, index);
    } else { return 0; }
}


// findDirectoryFilesystem(fsNode_t *node, char *name) - Finds a directory in a filesystem.
fsNode_t *findDirectoryFilesystem(fsNode_t *node, char *name) {
    // Check if the node is a directory and if it has a callback.
    if ((node->flags & 0x7) == VFS_DIRECTORY && node->finddir != 0) {
        return node->finddir(node, name);
    } else { return 0; }
}

// mountRootFilesystem(fsNode_t *node) - Mounts a root filesystem.
void mountRootFilesystem(fsNode_t *node) {
    fs_root = node;
}

// getRootFilesystem() - Returns root filesystem
fsNode_t *getRootFilesystem() {
    return fs_root;
}

// vfsInit() - Initializes the VFS
void vfsInit() {
    fs_tree = tree_create(); // Create the monuntpoint tree

    vfsEntry_t *root = kmalloc(sizeof(vfsEntry_t));

    // Allocate memory for string pointers
    root->fs_type = kmalloc(20);
    root->device = kmalloc(20);

    strcpy(root->name, "/");
    root->file = 0; // Nothing is mounted

    
    tree_set_root(fs_tree, root);

    if (fs_tree->root->value != root) panic("VFS", "vfsInit", "tree fail");
}


void debug_print_vfs_tree_node(tree_node_t *node, size_t height) {
    if (!node) return;

    // Indent output according to height
    char *indents = kmalloc(100);
    memset(indents, 0, 100);
    for (int i = 0; i < height; i++) {
        indents[i] = ' ';
    }

    // Get the current VFS node
    vfsEntry_t *fnode = (vfsEntry_t*)node->value;

    // Print it out
    if (fnode->file) {
        serialPrintf("%s%s (0x%x) -> 0x%x (%s)\n", indents, fnode->name, node->value, (void*)fnode->file, fnode->file->name);
    } else {
        serialPrintf("%s%s (0x%x) -> (empty)\n", indents, fnode->name, node->value);
    }

    kfree(indents);

    foreach(child, node->children) {
        debug_print_vfs_tree_node(child->value, height + 1);
    }
}


void debug_print_vfs_tree() {
    serialPrintf("=== VFS TREE ===\n");
    debug_print_vfs_tree_node(fs_tree->root, 1);
    serialPrintf("=== END VFS TREE ===\n");
}

// vfsMount(char *path, fsNode_t *localRoot) - Mount a filesystem to the specified path (ToaruOS has a really good impl. of this)
void *vfsMount(char *path, fsNode_t *localRoot) {
    if (!fs_tree) {
        serialPrintf("vfsMount: Attempt to mount filesystem when tree is non-existant\n");
        return NULL;
    } 
    if (!path || path[0] != '/') {
        serialPrintf("vfsMount: We don't know current working directory - cannot mount to %s\n", path);
        return NULL;
    }

    tree_node_t *ret_val = NULL;


    // Slice up the path
    char *p = kmalloc(strlen(path));
    memcpy(p, path, strlen(path)); // Memory safety!

    char *i = p;

    while (i < p + strlen(path)) {
        if (*i == '/') *i = '\0';
        i++;
    }

    p[strlen(path)] = '\0';
    i = p + 1;

    // Setup the root node
    tree_node_t *rootNode = fs_tree->root;

    if (*i == '\0') {
        // We are setting the root node! Should be easy, I think.
        serialPrintf("vfsMount: Mounting to /\n");
        vfsEntry_t *root = (vfsEntry_t*)rootNode->value;
        if (root->file != 0) serialPrintf("vfsMount: Path %s is already mounted - please do the correct thing and UNMOUNT.\n", path);
        serialPrintf("vfsMount: Setting up values (localRoot = 0x%x)\n", localRoot);
        root->file = localRoot;
        strcpy(root->device, "N/A");
        strcpy(root->fs_type, "N/A");
        strcpy(root->name, "/");
        
        fs_root = localRoot;
        ret_val = rootNode;
    } else {
        tree_node_t *node = rootNode;
        char *at = i;
        while (true) {
            // Search for the path

            if (at >= p + strlen(path)) break;
            int found = 0;
            serialPrintf("vfsMount: Searching for %s...\n", at);

            foreach(child, node->children) {
                tree_node_t *tchild = (tree_node_t *)child->value;
                vfsEntry_t *entry = (vfsEntry_t*)tchild->value;
                if (!strcmp(entry->name, at)) {
                    found = 1;
                    node = tchild;
                    ret_val = node;
                    break;
                }
            }

            if (!found) {
                serialPrintf("vfsMount: Could not find %s - creating it.\n", at);
                
                
                vfsEntry_t *entry = kmalloc(sizeof(vfsEntry_t));

                strcpy(entry->name, at);
                entry->file = 0;
                entry->device = 0;
                entry->fs_type = 0;
                node = tree_node_insert_child(fs_tree, node, entry);

            }
            at = at + strlen(at) + 1;
        }

        vfsEntry_t *entry = (vfsEntry_t*)node->value;
        if (entry->file) {
            serialPrintf("vfsMount: Path %s is already mounted - please do the correct thing and UNMOUNT.\n", path);
        }

        entry->file = localRoot;
        ret_val = node;
    }

    kfree(p);
    return ret_val;
}

// vfs_canonicalizePath(const char *cwd, const char *input) - Canonicalizes a path (cwd = current working directory, input = path to append/canonicalize on)
char *vfs_canonicalizePath(const char *cwd, const char *input) {
    list_t *out = list_create(); // Stack-based canonicalizer, use list as a stack (ToaruOS)

    // Check if we have a relative path (not starting from root). If so, we need to canonicalize the working directory.
    if (strlen(input) && input[0] != '/') {
        // Make a copy of the current working directory
        char *path = kmalloc((strlen(cwd) + 1) * sizeof(char));
        memcpy(path, cwd, strlen(cwd) + 1);

        // Setup the tokenizer
        char *pch;
        char *save;
        pch = strtok_r(path, "/", &save);

        while (pch != NULL) {
            // Make copies of the path elements
            char *s = kmalloc(sizeof(char) * (strlen(pch) + 1));
            memcpy(s, pch, strlen(pch) + 1);

            // Put them in the list
            list_insert(out, s);
            
            // Repeat!
            pch = strtok_r(NULL, "/", &save); // note to self: why did I think I needed strtok_r? isn't this OS supposed to be memory-efficient?
                                              // having serious reflection moments in code comments - this is why reduceOS is the best OS
        }

        kfree(path);
    }

    // Insert elements from new path
    char *path = kmalloc((strlen(input) + 1) * sizeof(char));
    memcpy(path, input, strlen(input) + 1);

    // Initialize tokenizer 
    char *save;
    char *pch;
    pch = strtok_r(path, "/", &save);


    // Tokenize the path and make sure to take dare of .. and . to represent list pop and current.
    while (pch != NULL) {
        if (!strcmp(pch, "..")) {
            // Pop the list to move up.
            node_t *node = list_pop(out);
            if (node) {
                kfree(node->value);
                kfree(node);
            }
        } else if (!strcmp(pch, ".")) {
            // Do nothing
        } else {
            // Normal path. Push onto list.
            char *str = kmalloc(sizeof(char) * (strlen(pch) + 1));
            memcpy(str, pch, strlen(pch) + 1);
            list_insert(out, str);
        }

        pch = strtok_r(NULL, "/", &save);
    }

    kfree(path);

    // Calculate the size of the path string
    size_t size = 0;
    foreach(item, out) {
        size += strlen(item->value) + 1;
    }

    // Join the list
    char *output = kmalloc(sizeof(char) * (size + 1));
    char *offset = output;

    if (size == 0) {
        // Assume this is the root directory
        output = krealloc(output, sizeof(char) * 2);
        output[0] = '/';
        output[1] = '\0'; // Don't forget the null-termination!
    } else {
        // Append each element together
        foreach (item, out) {
            offset[0] = '/';
            offset++;
            memcpy(offset, item->value, strlen(item->value) + 1);
            offset += strlen(item->value);
        }
    }

    // Cleanup the list and return
    list_destroy(out);
    list_free(out);
    kfree(out);

    return output;
}