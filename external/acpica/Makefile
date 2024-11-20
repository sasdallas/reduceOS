include ./make.config

ifneq ($(USE_ACPICA), 1)
$(error "USE_ACPICA is set to 0. You cannot compile ACPICA")
endif

ifeq ($(ACPI_DEBUG), 1)
CFLAGS += -D ACPI_DEBUG_OUTPUT=1 -D ACPI_DISASSEMBLER=1
endif

# ACPICA directories
ACPICA_BASE_DIRECTORY = $(PROJECT_ROOT)/external/acpica/acpica-src
ACPICA_HEADERS_DIRECTORY = $(ACPICA_BASE_DIRECTORY)/source/include/
ACPICA_SOURCE_DIRECTORY = $(ACPICA_BASE_DIRECTORY)/source/components/

# Object stuff
ACPICA_OBJ = $(OBJ_OUTPUT_DIRECTORY)/acpica

# ACPICA sources. We will compile in this Makefile.
ACPICA_SRCS := $(wildcard $(ACPICA_SOURCE_DIRECTORY)*/*.c) 

# debugging is not supported
ACPICA_SRCS := $(filter-out $(ACPICA_SOURCE_DIRECTORY)debugger/%,$(ACPICA_SRCS)) 
ACPICA_SRCS := $(filter-out $(ACPICA_SOURCE_DIRECTORY)disassembler/%,$(ACPICA_SRCS))
ACPICA_SRCS := $(filter-out %/resources/rsdump.c,$(ACPICA_SRCS))

ACPICA_OBJS = $(ACPICA_SRCS:$(ACPICA_SOURCE_DIRECTORY)%.c=$(ACPICA_OBJ)/%.o)

# Our header files
HEXAHEDRON_ACPICA_HEADER = achexahedron.h

# Check to make sure the user actually installed the directory.
CHECK_DIRECTORY:
ifeq ($(wildcard $(ACPICA_BASE_DIRECTORY)/*),)
		$(error ACPICA does not exist. Please extract the ACPICA sources to $(ACPICA_BASE_DIRECTORY))
endif

# Target disabled
all:
	@echo "=== TARGET DISABLED ==="
	@echo "Please use make install"

MAKE_DIRECTORIES:
	@-mkdir -pv $(OBJ_OUTPUT_DIRECTORY)
	@-mkdir -pv $(DESTDIR)$(LIB_DIRECTORY)

$(ACPICA_OBJ)/%.o: $(ACPICA_SOURCE_DIRECTORY)%.c
	@echo "[ACPICA] Compiling $(shell basename $(dir $(abspath $(dir $<))))/$(notdir $<)"
	@-mkdir -pv $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(ACPICA_OBJ)/libacpica.a: $(ACPICA_OBJS)
	@echo "[ACPICA] Packaging into $(ACPICA_OBJ)/libacpica.a"
	$(AR) rcu $@ $(ACPICA_OBJS)

# Copy over the headers & mod acenv to include it instead of Linux
MOD_HEADERS:
	-cp $(HEXAHEDRON_ACPICA_HEADER) $(ACPICA_HEADERS_DIRECTORY)/platform/
	sed -i -e 's/aclinux/achexahedron/' $(ACPICA_HEADERS_DIRECTORY)/platform/acenv.h
	sed -i -e 's/_LINUX/__HEXAHEDRON__/' $(ACPICA_HEADERS_DIRECTORY)/platform/acenv.h

install: CHECK_DIRECTORY MAKE_DIRECTORIES MOD_HEADERS $(ACPICA_OBJ)/libacpica.a
	cp -r $(ACPICA_OBJ)/libacpica.a $(DESTDIR)$(LIB_DIRECTORY)/
	@echo "[ACPICA] Finished building libacpica.a"


install-headers: CHECK_DIRECTORY MOD_HEADERS
	-mkdir -pv $(DESTDIR)$(INCLUDE_DIRECTORY)/acpica/
	cp -rp $(ACPICA_HEADERS_DIRECTORY)/* $(DESTDIR)$(INCLUDE_DIRECTORY)/acpica/
	@echo "[ACPICA] Finished installing ACPICA headers"

clean:
	-rm -rf $(ACPICA_OBJ)
	-rm -rf $(DESTDIR)$(LIB_DIRECTORY)/libacpica.a