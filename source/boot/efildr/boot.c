// ==============================================
// boot.c - Polyaniline's EFI bootloader
// ==============================================
// This file is part of the Polyaniline bootloader. Please credit me if you use this code.

#include <efi.h>
#include <efilib.h>
#include <boot_terminal.h>

EFI_HANDLE Image_Handle;

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    // Initialize the main library
    InitializeLib(ImageHandle, SystemTable);
    
    // Setup ST and ImageHandle_Ptr
    ST = SystemTable;
    Image_Handle = ImageHandle;

    // Print loading text
    Print(L"Starting the Polyaniline bootloader...\n");
    Print(L"Initializing graphics subsystem...\n");

    int ret = GOP_Init();
    if (ret != 0) {
        Print(L"Failed to initialize the graphics subsystem.\n");
        return EFI_ABORTED;
    }

    Print(L"Successfully initialized graphics subsystem.\n");

    bootloader_main();

    return EFI_SUCCESS;
}
