// ======================================================================
// tarfs.c - Handles the ustar filesystem driver
// ======================================================================
// This file is based off an implementation from ToaruOS.
// The original implementation is available here: https://github.com/klange/toaruos/blob/master/kernel/vfs/tarfs.c

#include <kernel/tarfs.h> // Main header file
#include <kernel/tokenize.h>

// Function prototypes
static fsNode_t *tarfs_ustarToFile(tarfs_t *fs, ustar_t *file, unsigned int offset);


/* Interpreter Functions */

// These functions simply interpret things like UID, GID, mode, and size from USTAR files to integers that can be stored.
// They are copy and pasted from ToaruOS's implementation.


static unsigned int interpret_uid(ustar_t * file) {
	return 
		((file->ownerid[0] - '0') << 18) |
		((file->ownerid[1] - '0') << 15) |
		((file->ownerid[2] - '0') << 12) |
		((file->ownerid[3] - '0') <<  9) |
		((file->ownerid[4] - '0') <<  6) |
		((file->ownerid[5] - '0') <<  3) |
		((file->ownerid[6] - '0') <<  0);
}

static unsigned int interpret_gid(ustar_t * file) {
	return 
		((file->groupid[0] - '0') << 18) |
		((file->groupid[1] - '0') << 15) |
		((file->groupid[2] - '0') << 12) |
		((file->groupid[3] - '0') <<  9) |
		((file->groupid[4] - '0') <<  6) |
		((file->groupid[5] - '0') <<  3) |
		((file->groupid[6] - '0') <<  0);
}

static unsigned int interpret_mode(ustar_t * file) {
	return 
		((file->mode[0] - '0') << 18) |
		((file->mode[1] - '0') << 15) |
		((file->mode[2] - '0') << 12) |
		((file->mode[3] - '0') <<  9) |
		((file->mode[4] - '0') <<  6) |
		((file->mode[5] - '0') <<  3) |
		((file->mode[6] - '0') <<  0);
}

static unsigned int interpret_size(ustar_t * file) {
	return
		((file->size[ 0] - '0') << 30) |
		((file->size[ 1] - '0') << 27) |
		((file->size[ 2] - '0') << 24) |
		((file->size[ 3] - '0') << 21) |
		((file->size[ 4] - '0') << 18) |
		((file->size[ 5] - '0') << 15) |
		((file->size[ 6] - '0') << 12) |
		((file->size[ 7] - '0') <<  9) |
		((file->size[ 8] - '0') <<  6) |
		((file->size[ 9] - '0') <<  3) |
		((file->size[10] - '0') <<  0);
}


/* USTAR FUNCTIONS */

// (static) tarfs_ustarFromOffset(tarfs_t *fs, unsigned int offset, ustar_t *out) - Tries to get a ustar_t object from an offset
static int tarfs_ustarFromOffset(tarfs_t *fs, unsigned int offset, ustar_t *out) {
    readFilesystem(fs->device, offset, sizeof(ustar_t), (unsigned char *)out);
    if (out->ustar[0] != 'u' ||
            out->ustar[1] != 's' ||
            out->ustar[2] != 't' ||
            out->ustar[3] != 'a' ||
            out->ustar[4] != 'r') {
        return 0;            
    }
    return 1;
}

/* UTILITY FUNCTIONS */

static int countSlashes(char *string) {
    int i = 0;
    char *s = strstr(string, "/");

    while (s) {
        if (*(s + 1) == '\0') return i;
        i++;
        s = strstr(s + 1, "/");
    }

    return i;
}

static unsigned int round_to_512(unsigned int i) {
    unsigned int t = i % 512;

    if (!t) return i;
    return i + (512 - t);
}


/* VFS FUNCTIONS */



static struct dirent* readdir_tar_root(fsNode_t *node, unsigned long index) {
    // If the index is 0 or 1, return ./..
    if (index == 0) {
        struct dirent *out = kmalloc(sizeof(struct dirent));
        memset(out, 0x00, sizeof(struct dirent));
        strcpy(out->name, "."); 
        out->ino = 0;
        return out;
    }

    if (index == 1) {
        struct dirent *out = kmalloc(sizeof(struct dirent));
        memset(out, 0x00, sizeof(struct dirent));
        out->ino = 0;
        strcpy(out->name, "..");
        return out;
    }

    // Now, actually read through it and try to see if there's anything
    index -= 2;

    // Go through each file and pick ones at the root.
    // Root files simply won't have a '/'
    tarfs_t *fs = node->device;
    unsigned int offset = 0;
    ustar_t *file = kmalloc(sizeof(ustar_t));
    while (offset < fs->length) {
        int status = tarfs_ustarFromOffset(fs, offset, file);
        if (!status) {
            kfree(file);
            return NULL;
        }

        char filename_workspace[256];
        memset(filename_workspace, 0, 256);
        strncat(filename_workspace, file->prefix, 155);
        strncat(filename_workspace, file->filename, 100);

        if (!countSlashes(filename_workspace)) {
            char *slash = strstr(filename_workspace, "/");
            if (slash) *slash = '\0'; // Remove trailing slash

            if (strlen(filename_workspace)) {
                if (index == 0) {
                    struct dirent *out = kmalloc(sizeof(struct dirent));
                    memset(out, 0, sizeof(struct dirent));
                    out->ino = offset;
                    strcpy(out->name, filename_workspace);
                    kfree(file);
                    return out;
                } else {
                    index--;
                }
            }
        }

        offset += 512;
        offset += round_to_512(interpret_size(file));

    }

    kfree(file);
    return NULL;
}


static uint32_t read_tarfs(fsNode_t *node, off_t offset, uint32_t size, uint8_t *buffer) {
    tarfs_t *fs = node->device;
    ustar_t *file = kmalloc(sizeof(ustar_t));
    tarfs_ustarFromOffset(fs, node->inode, file);
    size_t file_size = interpret_size(file);

    if ((size_t)offset > file_size) return 0; // Trying to read more than is allowed
    if (offset + size > file_size) {
        size = file_size - offset;
    }

    kfree(file);

    return readFilesystem(fs->device, offset + node->inode + 512, size, buffer);
}

struct dirent* readdir_tarfs(fsNode_t *node, unsigned long index) {
    // Return . or .. if index is 0 or 1
    if (index == 0) {
        struct dirent *out = kmalloc(sizeof(struct dirent));
        memset(out, 0, sizeof(struct dirent));
        strcpy(out->name, ".");
        out->ino = 0;
        return out;
    }

    if (index == 1) {
        struct dirent *out = kmalloc(sizeof(struct dirent));
        memset(out, 0, sizeof(struct dirent));
        strcpy(out->name, "..");
        out->ino = 0;
        return out;
    }

    index -= 2;

    // Now let's go through each file and pick the ones that are root
    unsigned int offset = node->inode;

    tarfs_t *fs = node->device;
    ustar_t *file = kmalloc(sizeof(ustar_t));
    int status = tarfs_ustarFromOffset(fs, node->inode, file);

    char filename[256];
    memset(filename, 0, 256);
    strncat(filename, file->prefix, 155);
    strncat(filename, file->filename, 100);

    while (offset < fs->length) {
        status = tarfs_ustarFromOffset(fs, offset, file);

        if (!status) {
            kfree(file);
            return NULL;
        }


        char filename_workspace[256];
        memset(filename_workspace, 0, 256);
        strncat(filename_workspace, file->prefix, 155);
        strncat(filename_workspace, file->filename, 100);

        if (strstr(filename_workspace, filename) == filename_workspace) {
            if (!countSlashes(filename_workspace + strlen(filename))) {
                if (strlen(filename_workspace + strlen(filename))) {
                    if (index == 0) {
                        char *slash = strstr(filename_workspace + strlen(filename), "/");
                        if (slash) *slash = '\0'; // Remove trailing slash
                        struct dirent *out = kmalloc(sizeof(struct dirent));
                        memset(out, 0, sizeof(struct dirent));
                        out->ino = offset;
                        strcpy(out->name, filename_workspace+strlen(filename));
                        kfree(file);
                        return out;
                    } else {
                        index--;
                    }
                }
            }
        }

        offset += 512;
        offset += round_to_512(interpret_size(file));
    }

    kfree(file);
    return NULL;
}


fsNode_t *finddir_tarfs(fsNode_t *node, char *name) {
    tarfs_t *fs = node->device;

    // Find filename
    ustar_t *file = kmalloc(sizeof(ustar_t));
    tarfs_ustarFromOffset(fs, node->inode, file);

    char filename[256];
    memset(filename, 0, 256);
    strncat(filename, file->prefix, 155);
    strncat(filename, file->filename, 100);

    // Append the name
    strncat(filename, name, strlen(name));
    if (strlen(filename) > 255) {
        serialPrintf("finddir_tarfs: wtf?\n");
        kfree(file);
        return NULL;
    }

    unsigned int offset = node->inode;
    while (offset < fs->length) {
        int status = tarfs_ustarFromOffset(fs, offset, file);

        if (!status) {
            kfree(file);
            return NULL;
        }

        char filename_workspace[256];
        memset(filename_workspace, 0, 256);
        strncat(filename_workspace, file->prefix, 155);
        strncat(filename_workspace, file->filename, 100);

        if (filename_workspace[strlen(filename_workspace)-1] == '/') {
            filename_workspace[strlen(filename_workspace)-1] = '\0';
        }

        if (!strcmp(filename_workspace, filename)) {
            return tarfs_ustarToFile(fs, file, offset);
        }

        offset += 512;
        offset += round_to_512(interpret_size(file));
    }

    kfree(file);
    return NULL;
} 



fsNode_t *finddir_tar_root(fsNode_t *node, char *name) {
    tarfs_t *fs = node->device;

    unsigned int offset = 0;
    ustar_t *file = kmalloc(sizeof(ustar_t));
    while (offset < fs->length) {
        int status = tarfs_ustarFromOffset(fs, offset, file);

        if (!status) {
            kfree(file);
            return NULL;
        }

        char filename_workspace[256];
        memset(filename_workspace, 0, 256);
        strncat(filename_workspace, file->prefix, 155);
        strncat(filename_workspace, file->filename, 100);

        if (countSlashes(filename_workspace)) {
            // Skip
        } else {
            char *slash = strstr(filename_workspace, "/");
            if (slash) *slash = '\0';
            if (!strcmp(filename_workspace, name)) {
                return tarfs_ustarToFile(fs, file, offset);
            }
        }

        offset += 512;
        offset += round_to_512(interpret_size(file));
    }

    kfree(file);
    return NULL;
}

// (static) tarfs_ustarToFile(tarfs_t *fs, ustar_t *file, unsigned int offset) - Converts a ustar_t to a fsNode_t
static fsNode_t *tarfs_ustarToFile(tarfs_t *fs, ustar_t *file, unsigned int offset) {
    fsNode_t *out = kmalloc(sizeof(fsNode_t));
    memset(out, 0, sizeof(fsNode_t));

    out->device = fs;
    out->inode = offset;
    out->impl = 0;

    char filename_workspace[127];
    memcpy(out->name, filename_workspace, strlen(filename_workspace)+1);

    out->uid = interpret_uid(file);
    out->gid = interpret_gid(file);
    out->length = interpret_size(file);
    out->mask = interpret_mode(file);

    out->flags = VFS_FILE;

    if (file->type[0] == '5') {
        out->flags = VFS_DIRECTORY;
        out->readdir = readdir_tarfs;
        out->finddir = finddir_tarfs;
        out->create = NULL;
    } else if (file->type[0] == '1') {
        serialPrintf("tarfs_ustarToFile: Hardlink detected, cannot process.\n");
        kfree(out);
        return NULL;
    } else if (file->type[0] == '2') {
        serialPrintf("tarfs_ustarToFile: Symlink detected, cannot process.\n");
        out->flags = VFS_SYMLINK; // We set symlink method up but there is no readlink method (yet)
    } else {
        out->flags = VFS_FILE;
        out->read = read_tarfs;
    }

    kfree(file);
    return out;
}

fsNode_t *tar_mount(const char *device, const char *mount_path) {
    char *arg = kmalloc(strlen((char*)device));
    strcpy(arg, device);
    arg[strlen((char*)device)] = 0;

    char *argv[10];
    int argc = tokenize(arg, ",", argv);

    if (argc > 1) {
        serialPrintf("tar_mount: unexpected mount arguments: %s\n", device);
    }

    fsNode_t *dev = open_file(argv[0], 0);
    kfree(arg);

    if (!dev) {
        serialPrintf("tar_mount: Could not open target device '%s'\n", argv[0]);
        return NULL;
    }

    // Create a metadata structure for the mount
    tarfs_t *fs = kmalloc(sizeof(tarfs_t));

    fs->device = dev;
    fs->length = dev->length;

    fsNode_t *root = kmalloc(sizeof(fsNode_t));
    memset(root, 0, sizeof(fsNode_t));

    strcpy(root->name, "tarfs");
    root->uid = 0;
    root->gid = 0;
    root->length = 0;
    root->mask = 0555;
    root->readdir = readdir_tar_root;
    root->finddir = finddir_tar_root;
    root->create = NULL;
    root->flags = VFS_DIRECTORY;
    root->device = fs;

    return root;
}

int tar_install() {
    vfs_registerFilesystem("tar", tar_mount);
    return 0;
}