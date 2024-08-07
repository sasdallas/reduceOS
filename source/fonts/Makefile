include ./make.config

.PHONY: all clean install
.SUFFIXES: .o .psf

# Tools
OBJCOPY = $(HOST)-objcopy

# Directories
OUT_OBJ = $(PROJECT_ROOT)/obj
OUT_FONT = $(OUT_OBJ)/fonts

# Font sources
FONT_SOURCES = $(wildcard *.psf)
FONT_OBJS = $(patsubst %.psf, $(OUT_FONT)/%.o, $(FONT_SOURCES))


all: PRINT_HEADER MAKE_OUTPUT_DIRS $(FONT_OBJS)

PRINT_HEADER:
	@echo "-- Creating font binary objects..."

MAKE_OUTPUT_DIRS:
	-mkdir -p $(OUT_FONT)


$(OUT_FONT)/%.o: %.psf | $(OUT_FONT)
	$(OBJCOPY) -O elf32-i386 -B i386 -I binary $< $@

install: $(FONT_OBJS)
	@echo "-- Installing fonts..."
	-mkdir -p $(DESTDIR)$(PREFIX)/fonts/
	-cp -r $(OUT_FONT)/*.o $(DESTDIR)$(PREFIX)/fonts/ 

clean:
	rm -f $(OUT_FONT)/*.o