include ./make.config

# Other tools
MKDIR = mkdir
CP = cp -F
RM = rm
GRUB_FILE = grub-file

PYTHON = python3 # Should expose this so the user can modify. Sorry!


# Source directories

# Other Directories
OUT_OBJ = $(PROJECT_ROOT)/obj/libc_reduced
OUT_LIBRARY = . 

# Directories (EDIT ME)
SOURCE_DIRECTORIES = ordered_array sleep spinlock stdio stdlib string time
DIRECTORIES = $(addprefix $(OUT_OBJ)/,$(SOURCE_DIRECTORIES))




# Source files
C_SRCS = $(shell find $(SOURCE_DIRECTORIES) -name "*.c")
C_OBJS = $(patsubst %.c, $(OUT_OBJ)/%.o, $(C_SRCS))




all: TARGET_DISABLED

TARGET_DISABLED:
	@echo "!! THIS TARGET IS DISABLED !!"
	@echo "Please use make install."



libc_reduced.a: MAKE_OUTPUT_DIRS $(C_OBJS)
	@echo "-- Building libc_reduced.a..."
	$(AR) rcs $@ $(C_OBJS)


# Other targets for the Makefile

MAKE_OUTPUT_DIRS:
	@-$(MKDIR) $(OUT_OBJ)
	@-$(MKDIR) $(DIRECTORIES)


# These are just quick hacks to make things look nicer
PRINT_C_HEADER:
	@echo "-- Compiling C files..."


# Actual compilation functions

$(OUT_OBJ)/%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

# Installation functions

install-headers:
	@echo "-- Installing reduced_libk headers..."
	-mkdir -p $(DESTDIR)$(INCLUDE_DIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDE_DIR)

install: libc_reduced.a
	@echo "-- Installing libc_reduced.a..."
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp libc_reduced.a $(DESTDIR)$(LIBDIR)

# Clean function
clean:
	-$(RM) -rf $(DIRECTORIES)
	-$(RM) $(OUT_ASMOBJ)/*.o
	-$(RM) libc_reduced.a