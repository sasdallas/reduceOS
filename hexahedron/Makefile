# Hexahedron Makefile
include ./make.config

# Output folders
OUT_OBJ = $(OBJ_OUTPUT_DIRECTORY)/hexahedron/
OUT_BUILD = $(BUILD_OUTPUT_DIRECTORY)/hexahedron

SOURCE_DIRECTORIES = kernel

# Include the architecture Make configuration
include arch/$(BUILD_ARCH)/make.config

# Construct object output directories
OUTPUT_DIRECTORIES = $(addprefix $(OUT_OBJ)/,$(SOURCE_DIRECTORIES))

# Compile a list of C sources and C assembly sources
C_SOURCES = $(shell find $(SOURCE_DIRECTORIES) -name "*.c")
C_ASM_SOURCES = $(shell find $(SOURCE_DIRECTORIES) -name "*.S")

# Construct object lists
C_OBJECTS = $(patsubst %.c, $(OUT_OBJ)/%.o, $(C_SOURCES)) $(patsubst %.S, $(OUT_OBJ)/%.o, $(C_ASM_SOURCES))

# Target disabled
all:
	@echo "=== TARGET DISABLED ==="
	@echo "Please use make install"


# Compilation functions
$(OUT_OBJ)/%.o: %.S Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT_OBJ)/%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@


# Linker
$(DESTDIR)$(BOOT_OUTPUT)/hexahedron-kernel.elf: $(C_OBJECTS)
	@printf "\n-- Linking Hexahedron kernel...\n"
	$(LD) $(LDFLAGS) -T $(ARCH_LINK_LD) $(C_OBJECTS) -o $(DESTDIR)$(BOOT_OUTPUT)/hexahedron-kernel.elf $(LIBS)

	@printf "\n-- Copying debug symbols...\n"
	$(OBJCOPY) --only-keep-debug $(DESTDIR)$(BOOT_OUTPUT)/hexahedron-kernel.elf $(DESTDIR)$(BOOT_OUTPUT)/hexahedron-kernel-symbols.sym
	$(OBJCOPY) --strip-debug $(DESTDIR)$(BOOT_OUTPUT)/hexahedron-kernel.elf

	@printf "\n-- Checking multiboot compatibility (make sure you have the GRUB package)...\n"
	grub-file --is-x86-multiboot $(DESTDIR)$(BOOT_OUTPUT)/hexahedron-kernel.elf
	grub-file --is-x86-multiboot2 $(DESTDIR)$(BOOT_OUTPUT)/hexahedron-kernel.elf


# Create build directories
CREATE_BUILD_DIRECTORIES:
	@-mkdir -pv $(OUT_OBJ)
	@-mkdir -pv $(OUTPUT_DIRECTORIES)
	@-mkdir -pv $(DESTDIR)$(BOOT_OUTPUT)

# Main kernel build targets
install-headers:
	@-mkdir -pv $(DESTDIR)$(INCLUDE_DIRECTORY)
	@-cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDE_DIRECTORY)

install: CREATE_BUILD_DIRECTORIES $(DESTDIR)$(BOOT_OUTPUT)/hexahedron-kernel.elf

clean:
	@-rm -r $(OUT_OBJ)
	@-rm -r $(DESTDIR)$(BOOT_OUTPUT)/hexahedron-kernel.elf