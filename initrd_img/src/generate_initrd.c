// =========================================================
// generate_initrd.c - reduceOS initrd image generator
// =========================================================
// This file is a tool used to make an initrd image.
// Original adaptation is by James Molloy, modified by sasdallas to support more options.


// WHAT NEEDS TO BE DONE: Add subdirectory support and more.

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

// Every file in an initrd image contains a header.
// The GRUB magic number is 0xBF
typedef struct {
    uint8_t magic; // Beautiful magic number.
    char name[64]; // File name.
    uint32_t offset; // File offset.
    uint32_t length; // File length
} image_header;


// These argument parsing methods are probably pretty inefficient, but whatever.


// Arguments structure
struct args {
    char *files[100]; // List of files to add
    int fileAmount; // Amount of files
    char *switches[100]; // Amount of options/switches used.
    int switchesAmount; // Amount of switches
    char *switchArguments[100]; // Arguments passed to switches.
    bool useDirectory; // Determines if we use a directory.
    bool doVerbose; // Determines if we use verbose logging.
};

struct args parseArguments(int argc, char **argv) {
    struct args arguments;
    arguments.fileAmount = 0;
    arguments.switchesAmount = 0;

    bool switchOccurred = false;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // We have a switch.
            arguments.switches[arguments.switchesAmount] = argv[i];
            arguments.switchesAmount++;
            if (!strcmp(argv[i], "-d")) arguments.useDirectory = true;

            if (!strcmp(argv[i], "-v")) { arguments.doVerbose = true; }
            else { switchOccurred = true; }
        } else if (switchOccurred) {
            arguments.switchArguments[arguments.switchesAmount-1] = argv[i]; // switchesAmount was already incremented, decrement to get proper index
            switchOccurred = false;
        } else {
            arguments.files[arguments.fileAmount] = argv[i];
            arguments.fileAmount++;
        }
    }

    

    return arguments;
}


// This is a recursive function that loops itself a few times to find all files everywhere
int listDirectory(char *folder, char *files[], int index, char *fileNames[], bool verbose) {
    if (folder) {
        if (verbose) printf("Opening directory %s...\n", folder);
        DIR *directory = opendir(folder);
        if (directory == NULL) {
            printf("Error: Directory does not exist or open failed.\n");
            return -1;
        }

        struct dirent *inFile;
        FILE *entryFile;

        while (inFile = readdir(directory)) {
            if (!strcmp(inFile->d_name, ".") || !strcmp(inFile->d_name, "..")) continue;
            // Check if it's another directory.
            if (inFile->d_type == DT_DIR) {
                // First, get absolute path.
                char *absDirPath = (char*)malloc(strlen(folder) + strlen(inFile->d_name) + 1);
                sprintf(absDirPath, "%s/%s", folder, inFile->d_name);
                // Call ourselves to walk the directory.
                int returnResult = listDirectory(absDirPath, files, index, fileNames, verbose);
                if (returnResult == -1) return -1;
                else { index = index + (returnResult-index); }
                continue;
            }

            // We need to get an absolute file path (inFile->d_name is only the filename, doesn't include the directory)
            char *absFilePath = (char*)malloc(strlen(folder) + strlen(inFile->d_name) + 1);
            sprintf(absFilePath, "%s/%s", folder, inFile->d_name);

            entryFile = fopen(absFilePath, "r");
            if (entryFile == NULL) {
                printf("Error: File %s in directory %s failed to open.\n", inFile->d_name, folder);
                return -1;
            }

            fclose(entryFile);
            if (verbose) printf("Found file: %s (index: %i)\n", absFilePath, index);
            files[index] = absFilePath;
            fileNames[index] = inFile->d_name;
            index++;
        }

        // Check if there were no files found.
        if (index == 0) {
            printf("Error: No files present in the directory %s\n", folder);
            return -1;
        }
        return index;
    }
}




int main(int argc, char **argv) {
    if (argc == 1 || strcmp(argv[1], "-h") == 0) {
        printf("generate_initrd v1.0.0\n");
        printf("Usage: generate_initrd [options] file...\n");
        printf("Options:\n");
        printf("  -h             Print this message and exit.\n");
        printf("  --version      Prints the version number and author and exits.\n");
        printf("  -d [directory] Build an initrd image based on a directory\n");
        printf("  -o [file]      Specifies the output image (default: initrd.img)\n");
        printf("  -m [magic]     Use a custom magic number (default: 0xBF)\n");
        printf("  -v             Enables verbose logging\n");
        return 0;
    } else if (strcmp(argv[1], "--version") == 0) {
        printf("generate_initrd version 1.0.0\n");
        printf("Created originally for reduceOS\n");
        printf("Written by sasdallas on GitHub\n");
        return 0;
    }

    struct args arguments = parseArguments(argc, argv);
    printf("generate_initrd v1.0.0\n");

    // Now do some extra parsing to figure out if we need to swap default values.
    char *outputName = "initrd.img";
    uint8_t magicNumber = 0xBF;
    char *folder = "\0";

    if (arguments.switchesAmount != 0) {
        for (int i = 0; i < arguments.switchesAmount; i++) {
            if (strcmp(arguments.switches[i], "-o") == 0) {
                outputName = arguments.switchArguments[i];
            } else if (strcmp(arguments.switches[i], "-m") == 0) {
                
                magicNumber = arguments.switchArguments[i];
            } else if (strcmp(arguments.switches[i], "-d") == 0) {
                folder = arguments.switchArguments[i];
            }
        }
    }

    char *filePaths[64];
    char *fileNames[64]; // (this is used mainly for directories)
    // We use fileNames because (as of now) the image generator doesn't make subdirectories.
    int headerAmnt = 0;
    if (!arguments.useDirectory) {
        for (int i = 0; i < arguments.fileAmount; i++) {
            FILE *file = fopen(arguments.files[i], "r");
            if (file != NULL) {
                filePaths[i] = arguments.files[i];
                fileNames[i] = arguments.files[i];
                fclose(file);
            }

            else {
                printf("Error: File %s not found.\n", arguments.files[i]);
                return -1;
            }

        }

        if (arguments.fileAmount == 0) {
            printf("Error: No files were specified. Cannot continue.\n");
            return -1;
        }

        headerAmnt += arguments.fileAmount;
        printf("Adding %i files to initrd image...\n", arguments.fileAmount);
    } else {
        int index = 0;
        headerAmnt = listDirectory(folder, filePaths, index, fileNames, arguments.doVerbose); // The doVerbose setting allows us to output if we found files.
        printf("Adding %i files to initrd image...\n", headerAmnt);
    }

    
    
    // Now, generate all the headers for the files.
    printf("Generating image...\n");

    image_header headers[64];
    if (arguments.doVerbose) printf("Header struct size: %d\n", sizeof(image_header));
    uint32_t offset = sizeof(image_header) * 64 + sizeof(int);
    if (arguments.doVerbose) printf("Header offset: %d\n", offset);


    for (int i = 0; i < headerAmnt; i++) {
        if (arguments.doVerbose) printf("Writing file %s->%s at offset 0x%x...\n", filePaths[i], fileNames[i], offset);
        strcpy(headers[i].name, fileNames[i]);
        headers[i].offset = offset;
        FILE *fstream = fopen(filePaths[i], "r");
        // The file path getter already took care of checking if the files exist or not.
        fseek(fstream, 0, SEEK_END);
        headers[i].length = ftell(fstream);
        offset += headers[i].length;
        fclose(fstream);
        
        headers[i].magic = 0xBF;
    }

    // Finally, write them to the image file.
    printf("Writing image data to file %s...\n", outputName);
    
    int imageSize = 0;
    FILE *imgstream = fopen(outputName, "w");
    unsigned char *data = (unsigned char*)malloc(offset);
    fwrite(&headerAmnt, sizeof(int), 1, imgstream);
    fwrite(headers, sizeof(image_header), 64, imgstream);

    for (int i = 0; i < headerAmnt; i++) {
        FILE *fstream = fopen(filePaths[i], "r");
        imageSize += headers[i].length;

        // Read the file.
        unsigned char *buffer = (unsigned char*)malloc(headers[i].length);
        fread(buffer, 1, headers[i].length, fstream);


        // Write the read contents of the file.
        fwrite(buffer, 1, headers[i].length, imgstream);
        if (arguments.doVerbose) printf("Wrote file %s with filename %s to image file %s (file size: %i)\n", filePaths[i], fileNames[i], outputName, headers[i].length);

        // Close the file and free memory.
        fclose(fstream);
        free(buffer);
    }

    printf("Image generated at file path %s (amount of files: %i, image size: %i)\n", outputName, headerAmnt, imageSize);
}
