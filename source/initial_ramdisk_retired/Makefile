include ./make.config

# These variables are for the host machine's compiler, as it will need to be used in place of the default_host one.
# We need to build the image generator and run it on initrd/

GCC_HOST = gcc
LD_HOST = ld


# List of directories
INITRD_CONTENT = initrd
INITRD_OUT_OBJ = obj
INITRD_SOURCE = src
EXECUTABLE_OUTPUT_DIR = .

# List of source files
INITRD_SRCS = $(wildcard $(INITRD_SOURCE)/*.c)
INITRD_OBJS = $(patsubst $(INITRD_SOURCE)/%.c, $(INITRD_OUT_OBJ)/%.o, $(INITRD_SRCS))

all: TARGET_DISABLED

TARGET_DISABLED:
	@echo "!! THIS TARGET IS DISABLED !!"
	@echo "Please use make install."

initrd.img: $(EXECUTABLE_OUTPUT_DIR)/generate_initrd
	@echo "-- Creating initial ramdisk using generate_initrd..."
	./generate_initrd -d $(INITRD_CONTENT) -o initrd.img


$(EXECUTABLE_OUTPUT_DIR)/generate_initrd: $(INITRD_OBJS)
	$(GCC_HOST) $(INITRD_OBJS) -o $(EXECUTABLE_OUTPUT_DIR)/generate_initrd

$(INITRD_OUT_OBJ)/%.o: $(INITRD_SOURCE)/%.c
	$(GCC_HOST) -std=c99 -D_BSD_SOURCE -c $< -o $@

install: initrd.img
	@echo "-- Copying initial ramdisk to ${BOOT_OUTPUT}..."
	-mkdir -p $(DESTDIR)$(BOOT_OUTPUT)
	cp initrd.img $(DESTDIR)$(BOOT_OUTPUT)
	rm initrd.img

clean:
	-rm obj/*.o
	-rm generate_initrd
	-rm initrd.img