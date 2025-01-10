reduceOS module/driver system
==========================================================

reduceOS can load its drivers from .mod files, which are created with kernel and libc includes.

The modules themselves can have entirely different contents, but the important part is that all modules MUST have a metadata.
In kernel/mod.h, the metadata structure can be found. It requires a name, description, init() method, and deinit() method.

The modules must define the structures like this:
struct Metadata data = {
    .name = "Example Module",
    .description = "An example module for reduceOS",
    .init = init,
    .deinit = deinit
};


These modules must also have a mod.conf file in their directory. This .conf file will dictate the module's parameters.
Structure of the .conf file:

CONF_START
FILENAME <filename>
MODLOAD <load>
PRIORITY <priority>
CONF_END

THIS CONF FILE IS CASE-SENSITIVE, MEANING THERE MUST BE NO EXTRA SPACES OR ANY OTHER CHARACTERS.

Options for MODLOAD: 
BOOTTIME (execute right after basic initialization has completed)
USERSPACE (execute when jumping to user-space)

Options for PRIORITY:
REQUIRED (module will cause a fault if not loaded)
HIGH (module will notify the user if not loaded)
LOW (logs it, but continues)

BOOTTIME modules are assembled into the initial ramdisk under modules/, as well as under /boot/modules in sysroot.
Their config files are combined into the initial ramdisk and sysroot as mod_boot.conf

USERSPACE modules are assembled into the initial ramdisk under modules/, as well as under /boot/modules in sysroot.
Their config files are combined into the initial ramdisk and sysroot as mod_user.conf

If this changes, it will automatically be updated.
