-=-=-=-=-= HEXAHEDRON INTERNAL DRIVER STRUCTURE =-=-=-=-=-

Hexahedron follows a unique internal driver structure. There are two main types of drivers:
- generic drivers
- machine-specific drivers

== GENERIC DRIVERS ==

Generic drivers provide a management system for common parts of hardware.
For example, a generic driver such as the "drive.c" driver will take into place VFS mounting of the drive and handling errors.

== MACHINE-SPECIFIC DRIVERS ==

It's another way of saying architecture-specific, however since x86 is largely compatible with
drivers, I didn't want that.

These drivers are not prefixed into one folder. They have their machine-type as a folder (e.g. x86/ drivers are x86 and x86_64-specific)

These drivers are still being worked out how they need to be read and initialized. 
Another layer of abstraction is a possibility, but I was thinking some sort of init method list in the config, if that wasn't too clunky.

