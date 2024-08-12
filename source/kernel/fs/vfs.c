// =====================================================
// vfs.c - Virtual File System handler
// =====================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.
// NOTE: Some functions are from ToaruOS by klange - these functions will be marked with a (TOARU) in their header.
// Thank you!


#include <kernel/vfs.h> // Main header file

fsNode_t *fs_root = 0; // Root of the filesystem
tree_t *fs_tree = NULL; // Mountpoint tree
hashmap_t *fs_types = NULL;
char cwd[256] = "/";


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

// openFilesystem(fsNode_t *node, uint8_t read, uint8_t write) - Opens a node.
void openFilesystem(fsNode_t *node, uint8_t read, uint8_t write) {
    if (node->open != 0) {
        return node->open(node);
    }
}

// closeFilesystem(fsNode_t *node) - Closes a node.
void closeFilesystem(fsNode_t *node) {
    if (node->close != 0)
        return node->close(node);
}

// Now it gets slightly more complicated.

// readDirectoryFilesystem(fsNode_t *node, uint32_t index) - Reads a directory in a filesystem.
struct dirent *readDirectoryFilesystem(fsNode_t *node, uint32_t index) {
    // BUG: User needs to be able to see the /device/ directory but that's not possible right now

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
    } else {
        serialPrintf("The node does not have a finddir function\n");
        return 0; 
    }
}

// unlinkFilesystem(char *name) - Unlinks/removes a file from the filesystem
int unlinkFilesystem(char *name) {
    fsNode_t *parent;
    char *path = vfs_canonicalizePath(cwd, name);

    char *parent_path_tmp = kmalloc(strlen(path) + 5);
    snprintf(parent_path_tmp, strlen(path) + 4, "%s/..", path);

    char *parent_path = vfs_canonicalizePath(cwd, parent_path_tmp);

    kfree(parent_path_tmp);

    char *file_path = path + strlen(path) - 1;
    while (file_path > path) {
        if (*file_path == '/') {
            file_path += 1;
            break;
        }

        file_path--;
    }

    while (*file_path == '/') file_path++;

    serialPrintf("unlinkFilesystem: Unlinking %s in %s\n", file_path, parent_path);
    parent = open_file(parent_path, 0);
    kfree(parent_path);

    if (!parent) {
        kfree(path);
        return -1;
    }

    // TODO: Permissions

    int ret = 0;
    if (parent->unlink) {
        ret = parent->unlink(parent, file_path);
    } else {
        ret = -1;
    }

    kfree(path);
    closeFilesystem(parent);
    return ret;
}


// getRootFilesystem() - Returns root filesystem
fsNode_t *getRootFilesystem() {
    return fs_root;
}

// vfsInit() - Initializes the VFS
void vfsInit() {
    fs_tree = tree_create(); // Create the mountpoint tree

    vfsEntry_t *root = kmalloc(sizeof(vfsEntry_t));

    // Allocate memory for string pointers
    root->fs_type = kmalloc(20);
    root->device = kmalloc(20);

    strcpy(root->name, "/");
    root->file = 0; // Nothing is mounted

    // Setup the tree root    
    tree_set_root(fs_tree, root);

    fs_root = NULL;

    // Create the hashmap
    fs_types = hashmap_create(5);
}


// vfs_registerFilesystem(const char *name, vfs_mountCallback callback) - Register a filesystem mount callback that will be called when needed (TOARU)
int vfs_registerFilesystem(const char *name, vfs_mountCallback callback) {
    if (hashmap_get(fs_types, name)) return 1;
    hashmap_set(fs_types, name, (void*)(uintptr_t)callback);
    return 0;
}

// vfs_mountType(const char *type, const char *arg, const char *mountpoint) - Calls the mount handler for a filesystem with the type (TOARU)
int vfs_mountType(const char *type, const char *arg, const char *mountpoint) {
    vfs_mountCallback t = (vfs_mountCallback)(uintptr_t)hashmap_get(fs_types, type);
    if (!t) {
        serialPrintf("vfs_mountType: Unknown filesystem type: %s\n", type);
        return -1;
    }

    fsNode_t *n = t(arg, mountpoint);

    // Quick hack to let partition mappers not return a node to mount at mountpoint
    if ((uintptr_t)n == (uintptr_t)1) return 0;
    if (!n) return -1;

    // Mount the filesystem
    tree_node_t *node = vfsMount(mountpoint, n);
    if (node && node->value) {
        vfsEntry_t *entry = (vfsEntry_t*)node->value;
        entry->fs_type = kmalloc(strlen(type) + 1);
        entry->device = kmalloc(strlen(arg) + 1);

        strcpy(entry->fs_type, type);
        strcpy(entry->device, arg);
    }

    serialPrintf("vfs_mountType: Mounted %s[%s] to %s: %p\n", type, arg, mountpoint, (void*)n);
    debug_print_vfs_tree(false);

    return 0;
}

// (static) vfs_readdirMapper(fsNode_t *node, unsigned long index) - Maps a dirent to the VFS (TOARU)
static struct dirent *vfs_readdirMapper(fsNode_t *node, unsigned long index) {
    tree_node_t *d = (tree_node_t*)node->device;

    if (!d) return NULL;
    
    if (index == 0) {
        struct dirent *dir = kmalloc(sizeof(struct dirent));
        strcpy(dir->name, ".");
        dir->ino = 0;
        return dir;
    } else if (index == 1) {
        struct dirent *dir = kmalloc(sizeof(struct dirent));
        strcpy(dir->name, "..");
        dir->ino = 1;
        return dir;
    }

    index -= 2;
    unsigned long i = 0;
    foreach (child, d->children) {
        if (i == index) {
            // Recursively print the children
            tree_node_t *tchild = (tree_node_t*)child->value;
            vfsEntry_t *n  = (vfsEntry_t*)tchild->value;
            struct dirent *dir = kmalloc(sizeof(struct dirent));

            size_t len = strlen(n->name) + 1;
            memcpy(&dir->name, n->name, (256 < len) ? 256 : len);
            dir->ino = 1;
            return dir;
        }
        i++;
    }

    return NULL;
}

// (static) vfs_mapper() - Works with vfs_mapDirectory (TOARU)
static fsNode_t *vfs_mapper() {
    fsNode_t *fnode = kmalloc(sizeof(fsNode_t));
    memset(fnode, 0x00, sizeof(fsNode_t));
    fnode->mask = 0555;
    fnode->flags = VFS_DIRECTORY;
    fnode->readdir = vfs_readdirMapper;
    strcpy(fnode->name, "Mapped Directory");
    return fnode;
}

// vfs_mapDirectory(const char *c) - Maps a directory in the virtual filesystem (TOARU)
void vfs_mapDirectory(const char *c) {
    fsNode_t *f = vfs_mapper();
    vfsEntry_t *entry = vfsMount((char*)c, f);
    if (!strcmp(c, "/")) {
        f->device = fs_tree->root;
    } else {
        f->device = entry;
    }
}

// Technically (TOARU)
void debug_print_vfs_tree_node(tree_node_t *node, size_t height, bool printout) {
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
        if (printout) printf("%s%s (0x%x) -> 0x%x (%s)\n", indents, fnode->name, node->value, (void*)fnode->file, fnode->file->name);
    } else {
        serialPrintf("%s%s (0x%x) -> (empty)\n", indents, fnode->name, node->value);
        if (printout) printf("%s%s (0x%x) -> (empty)\n", indents, fnode->name, node->value);
    }

    kfree(indents);

    foreach(child, node->children) {
        debug_print_vfs_tree_node(child->value, height + 1, printout);
    }
}


void debug_print_vfs_tree(bool printout) {
    serialPrintf("=== VFS TREE ===\n");
    debug_print_vfs_tree_node(fs_tree->root, 1, printout);
    serialPrintf("=== END VFS TREE ===\n");
}

// vfsMount(char *path, fsNode_t *localRoot) - Mount a filesystem to the specified path (TOARU)
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
        // serialPrintf("vfsMount: Setting up values (localRoot = 0x%x)\n", localRoot);
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
            //serialPrintf("vfsMount: Searching for %s...\n", at);

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
                //serialPrintf("vfsMount: Could not find %s - creating it.\n", at);
                
                
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

// vfs_canonicalizePath(const char *cwd, const char *input) - Canonicalizes a path (cwd = current working directory, input = path to append/canonicalize on) (TOARU)
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

// vfs_getMountpoint(char *path, unsigned int path_depth, char **outpath, unsigned int *outdepth) - Gets the mount point of something at path (TOARU)
fsNode_t *vfs_getMountpoint(char *path, unsigned int path_depth, char **outpath, unsigned int *outdepth) {
    size_t depth = 0;

    for (depth = 0; depth <= path_depth; depth++) {
        path += strlen(path) + 1;
    }

    // Last available node
    fsNode_t *last = fs_root;
    tree_node_t *node = fs_tree->root;

    char *at = *outpath;
    int _depth = 1;
    int _tree_depth = 0;

    while (true) {
        if (at >= path) break; // We exceeded path, we're done.

        int found = 0;

        foreach(child, node->children) {
            tree_node_t *tchild = (tree_node_t*)child->value;
            vfsEntry_t *entry = (vfsEntry_t*)tchild->value;

            if (!strcmp(entry->name, at)) {
                // We found the entry
                found = 1;
                node = tchild;
                at = at + strlen(at) + 1;
                if (entry->file) {
                    _tree_depth = _depth;
                    last = entry->file;
                    *outpath = at;
                }
                break;
            }
        }

        if (!found) break;
        _depth++;
    }

    *outdepth = _tree_depth;

    if (last) {
        // Clone the last object and return it.
        fsNode_t *last2 = kmalloc(sizeof(fsNode_t));
        memcpy(last2, last, sizeof(fsNode_t));
        return last2;
    }

    return last;
}


// open_file_recursive(const char *filename, uint64_t flags, uint64_t symlink_depth, char *relative) - Opens a file (but recursive and does all the work) (TOARU)
fsNode_t *open_file_recursive(const char *filename, uint64_t flags, uint64_t symlink_depth, char *relative) {
    /* TODO: Add support for flags */

    if (!filename) return NULL; // Stupid users.

    // Canonicalize the potentially relative path (doesn't start from /)
    char *path = vfs_canonicalizePath(relative, filename);

    // Store the length to save those precious CPU cycles
    size_t pathLength = strlen(path);


    // If the length is 1, it can only be for the root filesystem, so clone & return that.
    if (pathLength == 1) {
        fsNode_t *rootClone = kmalloc(sizeof(fsNode_t));
        memcpy(rootClone, fs_root, sizeof(fsNode_t));

        // Free the path
        kfree(path);

        // Call the filesystem's open method
        openFilesystem(rootClone, 1, 1); // Ignore flags because why not.

        return rootClone;
    }

    // Man, we gotta do actual work now. This sucks.
    
    // First, we'll need to find each path separator.
    char *pathOffset = path;
    uint64_t pathDepth = 0;
    
    while (pathOffset < path + pathLength) {
        if (*pathOffset == '/') {
            *pathOffset = '\0';
            pathDepth++;
        }

        pathOffset++;
    }

    // Null-terminate
    path[pathLength] = '\0';
    pathOffset = path + 1;



    // The path is currently tokenized and pathOffset points to the first token
    // pathDepth is the number of directories in the path
    
    // So we'll have to dig through the tree to find the file.
    unsigned int depth = 0;
    fsNode_t *nodePtr = vfs_getMountpoint(path, pathDepth, &pathOffset, &depth); // Get the mountpoint for the file

    if (!nodePtr) return NULL; // Failed to find the file

    do {
        // TODO: Symlink support is needed. Coming to you in 2026.

        if (pathOffset >= path + pathLength) {
            kfree(path);
            openFilesystem(nodePtr, 1, 1);
            return nodePtr;
        }

        if (depth == pathDepth) {
            // We found the file and are done, open the node
            openFilesystem(nodePtr, 1, 1);
            kfree(path);
            return nodePtr;
        }

        // Still searching
        // serialPrintf("open_file_recursive: ... continuing to search for %s (current node: %s)...\n", pathOffset, nodePtr->name);
        
        fsNode_t *node_next = findDirectoryFilesystem(nodePtr, pathOffset);
        kfree(nodePtr);
        nodePtr = node_next;

        if (!nodePtr) {
            // Could not find the requested directory
            kfree(path);
            return NULL;
        }

        pathOffset += strlen(pathOffset) + 1;
        ++depth;
    } while (depth < pathDepth + 1);

    // serialPrintf("open_file_recursive: Not found\n");
    kfree(path);
    return NULL;
}




// open_file(const char *filename, unsigned int flags) - Opens a file using the VFS current working directory
fsNode_t *open_file(const char *filename, unsigned int flags) {
    return open_file_recursive(filename, flags, 0, (char *)cwd);
}


// change_cwd(const char *newdir) - Changes the current working directory
void change_cwd(const char *newdir) {
    if (fs_root == NULL) return;

    // This is a little weird - but we'll first need to just see if newdir refers to a relative path or a root directory
    if (newdir[0] == '/') {
        // It refers to the root directory, so this is easy, just overwrite cwd with newdir.
        if (strlen(newdir) > 256) {
            serialPrintf("change_cwd: Maximum path length (256) reached! Cannot continue.\n");
            return;
        }

        strcpy(cwd, newdir);
        return;
    }

    // We just have to canonicalize the path, and then update CWD.

    char *temp_cwd = vfs_canonicalizePath(cwd, newdir);
    if (strlen(temp_cwd) > 256) {
        serialPrintf("change_cwd: Maximum path length (256) reached! Cannot continue.\n");
        kfree(temp_cwd);
        return;
    }

    strcpy(cwd, temp_cwd);
    kfree(temp_cwd);


    // Inform terminal of changes
    updateShell();
}

// get_cwd() - Returns the current working directory
char *get_cwd() {
    return cwd;
}
