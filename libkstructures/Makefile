# libkstructures Makefile

include ./make.config

# OUT DIRECTORIES
OUT_OBJ = $(OBJ_OUTPUT_DIRECTORY)/libkstructures

# SOURCE DIRECTORIES
SOURCE_DIRECTORIES = list tree json hashmap circbuf

OUTPUT_DIRECTORIES = $(addprefix $(OUT_OBJ)/,$(SOURCE_DIRECTORIES))

# Construct a basic list of C sources
C_SOURCES = $(shell find $(SOURCE_DIRECTORIES) -maxdepth 1 -name "*.c")
C_OBJECTS = $(patsubst %.c, $(OUT_OBJ)/%.o, $(C_SOURCES))

# Include the architecture Make configuration to allow it to add to DIRECTORIES
include arch/$(BUILD_ARCH)/make.config

# Compile a C file
$(OUT_OBJ)/%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

# Build target for libkstructures
$(DESTDIR)$(LIB_DIRECTORY)/libkstructures.a: $(C_OBJECTS)
	$(AR) rcs $@ $(C_OBJECTS)

# Target disabled
all:
	@echo "=== TARGET DISABLED ==="
	@echo "Please use make install"

# Makes output directories
CREATE_BUILD_DIRECTORIES:
	@-mkdir -pv $(OUT_OBJ)
	@-mkdir -pv $(OUTPUT_DIRECTORIES)
	@-mkdir -pv $(DESTDIR)$(LIB_DIRECTORY)

# Cosmetic
PRINT_HEADER:
	@echo
	@echo
	@echo "[ Building libkstructures, please wait... ]"
	@echo
	@echo

install-headers:
	@-mkdir -pv $(DESTDIR)$(INCLUDE_DIRECTORY)
	@-cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDE_DIRECTORY)/

install: PRINT_HEADER CREATE_BUILD_DIRECTORIES $(DESTDIR)$(LIB_DIRECTORY)/libkstructures.a

clean:
	@-rm -r $(OUT_OBJ)
	@-rm -r $(DESTDIR)$(LIB_DIRECTORY)/libkstructures.a