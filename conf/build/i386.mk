# i386 build configuration for Hexahedron/Polyhedron/any I386-based project
# YOU MAY EDIT THIS

# ACPICA settings
# WARNING: It is possible that debug output will not compile! Our ACPICA file doesn't have the requiements!
USE_ACPICA = 1
ACPI_DEBUG = 0

ifeq ($(USE_ACPICA), 1)
CFLAGS += -DACPICA_ENABLED
endif