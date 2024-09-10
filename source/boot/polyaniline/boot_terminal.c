// ==================================================================
// boot_terminal.c - Text output system for BIOS/EFI systems
// ===================================================================
// This file is part of the Polyaniline bootloader. Please credit me if you use this code.

#include <boot_terminal.h>
#include <platform.h>
#include <term-font.h>


#define character_height LARGE_FONT_CELL_HEIGHT
#define character_width LARGE_FONT_CELL_WIDTH



/* 
    TODO: Cleanup is desperately needed for this code.
	This is a cobbled together mess.
*/


// Function prototype for character setter (putch should be used in BIOS/EFI loaders)
static void setch(int x, int y, int val, int attribute);



#ifdef EFI_PLATFORM
#include <efi.h>
extern EFI_SYSTEM_TABLE *ST; // System table for EFI
#endif



static int offset_x = 0; // Offsets for fonts
static int offset_y = 0;
static int center_x = 0; // Centering
static int center_y = 0;


/* EFI PLATFORM CODE */

#ifdef EFI_PLATFORM

EFI_GRAPHICS_OUTPUT_PROTOCOL *gop; // Graphics Output Protocol

// Initialize the Graphics Output Protocol function
int GOP_Init() {
    // todo: rewrite this!
	Print(L"Initializing GOP\n");
    static EFI_GUID efi_graphics_output_protocol_guid = {0x9042a9de,0x23dc,0x4a38,  {0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}};

    // Grab handle buffers
    EFI_HANDLE *handles;
    UINTN handles_count;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics;

    EFI_STATUS status = uefi_call_wrapper(ST->BootServices->LocateHandleBuffer, 5, ByProtocol, &efi_graphics_output_protocol_guid, NULL, &handles_count, &handles);
    if (EFI_ERROR(status)) {
        Print(L"terminal: uefi_call_wrapper(ST->BootServices->LocateHandleBuffer) failed");
        for (;;);
    }
    
    
    status = uefi_call_wrapper(ST->BootServices->HandleProtocol, 3, handles[0], &efi_graphics_output_protocol_guid, (void**)&graphics);
    if (EFI_ERROR(status)) {
        Print(L"terminal: uefi_call_wrapper(ST->BootServices->HandleProtocol) failed\n");
        for (;;);
    }

    Print(L"terminal: Located GOP protocol\n"); // Debug

    gop = graphics;

    // Run some calculations

    int total_width = gop->Mode->Info->HorizontalResolution;
    int total_height = gop->Mode->Info->VerticalResolution;


    // Offsets for font drawing
    offset_x = (total_width - 80 * character_width) / 2;
    offset_y = (total_height - 24 * character_height) / 2;

    center_x = total_width / 2;
    center_y = total_height / 2;

    return 0;
}


// _clearScreen() - Resets the buffer
void _clearScreen() {
    memset(gop->Mode->FrameBufferBase, 0, gop->Mode->FrameBufferSize);
}

// setPixel(int x, int y, uint32_t color) - Similar to VBE function
void setPixel(int x, int y, uint32_t color) {
    ((uint32_t*)gop->Mode->FrameBufferBase)[(x + offset_x) + (y + offset_y) * gop->Mode->Info->PixelsPerScanLine] = color;
}


// putch(unsigned char c, int x, int y, int attribute)
static void putch(unsigned char c, int x, int y, int attribute) {
    setch(x * character_width, y * character_height, c, attribute);
}


#else


// BIOS prototypes
void _clearScreen() {

}

void setPixel(int x, int y, uint32_t color) {

}

static void putch(unsigned char c, int x, int y, int attribute) {
	
}

#endif


/* Main graphics functions, exposed to bootloader core */

// Because the font's implementation is sourced from ToaruOS, we will use its write_char function. (todo: stop stealing graphical elements)
// The function is pretty similar but the impl. is nice, because VGA_TO_VBE is something I didn't consider but will help a lot.
// I'll add my own touch of stuff though, not copy-pasting ;)

// This is a list of terminal colors that are used.
// See the VGA_TO_VBE macro in the reduceOS kernel to understand how this works (VGA->ANSI, color conversion)
static uint32_t terminalColors[] = {
    0xFF000000,             // Black
    0xFFCC0000,             // Red
    0xFF4E9A06,             // Green
    0xFFC4A000,             // Brown
    0xFF3465A4,             // Blue
    0xFF75507B,             // Purple
    0xFF06989A,             // Cyan
    0xFFD3D7CF,             // Gray

    // Light Colors
    0xFF555753,             // Dark Gray
    0xFFEF2929,             // Light Red
    0xFF8AE234,             // Light Green
    0xFFFCE94F,             // Yellow
    0xFF729FCF,             // Light Blue
    0xFFAD7FA8,             // Light Purple
    0xFF34E2E2,             // Light Cyan
    0xFFEEEEEC              // White
};

// VGA-ANSI conversion. TODO: horrid
// These are indexes
char VGA_TO_TERMCOLOR[] = {
    0, 4, 2, 6, 1, 5, 3, 7,
    8, 12, 10, 14, 9, 13, 11, 15
};

void setch(int x, int y, int val, int attribute) {
    if (val > 128) {
		val = 4;
	}

	uint32_t fg_color = terminalColors[VGA_TO_TERMCOLOR[attribute & 0xF]];
	uint32_t bg_color = terminalColors[VGA_TO_TERMCOLOR[(attribute >> 4) & 0xF]];

	uint32_t colors[] = {bg_color, fg_color};

	uint8_t * c = large_font[val];
	for (uint8_t i = 0; i < character_height; ++i) {
		for (uint8_t j = 0; j < character_width; ++j) {
			setPixel(x+j,y+i,colors[!!(c[i] & (1 << LARGE_FONT_MASK-j))]);
		}
	}
}


/* LOGO DRAWING FUNCTIONS */

static void draw_square(int x, int y, unsigned int color) {
	for (int _y = 0; _y < 7; ++_y) {
		for (int _x = 0; _x < 7; ++_x) {
			setPixel(center_x - 32 - offset_x + x * 8 + _x, center_y - 32 - offset_y + y * 8 + _y, color);
		}
	}
}

void draw_polyaniline_testTube() {
	uint64_t logo_tube = 0x3C24242424242400UL; 	// This is a clever trick - the binary values are encoded and bitshifted.
    uint64_t logo_fill = 0x18181800000000UL; 	// This is the filled part of our logo (the funny green liquid)

	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			if (logo_tube & (1 << x)) {
				draw_square(x, y, 0xFFD3D7CF);
			}
		}
		logo_tube >>= 8;
	}

    for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			if (logo_fill & (1 << x)) {
				draw_square(x, y, 0xFF8AE234);
			}
		}
		logo_fill >>= 8;
	}
}




/* PRINTF/TEXT RELATED FUNCTIONS */

int curX = 0;
int curY = 0;
int attr = 0x07;

// putchar(int ch) - Place a char at curX and curY of attribute attr
int putchar(int ch) {
	// Before anything, check if curY is greater than 80, if so, clear screen and reset (we're back to those days)
	if (curY >= 80) {
		_clearScreen();
		draw_polyaniline_testTube(); // Gotta have the branding
		curY = 0;
		curX = 0;
	}

	// Check if we're trying to print a newline, if so, increment curY and return
	if (ch == '\n') {
		curY++;
		curX = 0;
		return 0;
	}

    putch(ch, curX, curY, attr);
    curX++;

	// If we're over the X value, increment Y and reset X.
    if (curX >= 80) {
        curX = 0;
        curY++;
    }

	return 0;
}

// setCursor(int x, int y) - Move curX and curY
void setCursor(int x, int y) {
	curX = x;
	curY = y;
}

// getX() - Returns the current X
int getX() {
	return curX;
}

// getY() - Returns the current Y
int getY() {
	return curY;
}

// setColor(uint8_t color) - Set the color of the terminal
void setColor(uint8_t color) {
	attr = color; // TODO: Add VGA enum
}




/* printf() functions from reduceOS */
/* welcome to the cobbled together hell that is this file!!! */

// printf.c - Main printing function

// WARNING: I DO NOT OWN THIS IMPLEMENTATION AT ALL. I DID NOT CREATE IT OR CODE IT AT ALL. THE ORIGINAL HEADER OF THE IMPLEMENTATION WILL BE PUT HERE:
/**
 * @file  kprintf.c
 * @brief Kernel printf implementation.
 *
 * This is the newer, 64-bit-friendly printf originally implemented
 * for EFI builds of Kuroko. It was merged into the ToaruOS libc
 * and later became the kernel printf in Misaka. It supports the
 * standard set of width specifiers, '0' or ' ' padding, left or
 * right alignment, and the usermode version has a (rudimentary,
 * inaccurate) floating point printer. This kernel version excludes
 * float support. printf output is passed to callback functions
 * which can write the output to a string buffer or spit them
 * directly at an MMIO port. Support is also present for bounded
 * writes, and we've left @c sprintf out of the kernel entirely
 * in favor of @c snprintf.
 *
 * @copyright
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2011-2021 K. Lange
 */


#ifdef PLATFORM_EFI
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#else
#include <libk_reduced/stdint.h>
#include <libk_reduced/stdlib.h>
#include <libk_reduced/stdarg.h>
#endif

size_t (*printf_output)(size_t, uint8_t *) = NULL;

#define OUT(c) do { callback(userData, (c)); written++; } while (0)
static size_t print_dec(unsigned long long value, unsigned int width, int (*callback)(void*,char), void * userData, int fill_zero, int align_right, int precision) {
	size_t written = 0;
	unsigned long long n_width = 1;
	unsigned long long i = 9;
	if (precision == -1) precision = 1;

	if (value == 0) {
		n_width = 0;
	} else {
		unsigned long long val = value;
		while (val >= 10UL) {
			val /= 10UL;
			n_width++;
		}
	}

	if (n_width < (unsigned long long)precision) n_width = precision;

	int printed = 0;
	if (align_right) {
		while (n_width + printed < width) {
			OUT(fill_zero ? '0' : ' ');
			printed += 1;
		}

		i = n_width;
		char tmp[100];
		while (i > 0) {
			unsigned long long n = value / 10;
			long long r = value % 10;
			tmp[i - 1] = r + '0';
			i--;
			value = n;
		}
		while (i < n_width) {
			OUT(tmp[i]);
			i++;
		}
	} else {
		i = n_width;
		char tmp[100];
		while (i > 0) {
			unsigned long long n = value / 10;
			long long r = value % 10;
			tmp[i - 1] = r + '0';
			i--;
			value = n;
			printed++;
		}
		while (i < n_width) {
			OUT(tmp[i]);
			i++;
		}
		while (printed < (long long)width) {
			OUT(fill_zero ? '0' : ' ');
			printed += 1;
		}
	}

	return written;
}

/*
 * Hexadecimal to string
 */
static size_t print_hex(unsigned long long value, unsigned int width, int (*callback)(void*,char), void* userData, int fill_zero, int alt, int caps, int align) {
	size_t written = 0;
	int i = width;

	unsigned long long n_width = 1;
	unsigned long long j = 0x0F;
	while (value > j && j < UINT64_MAX) {
		n_width += 1;
		j *= 0x10;
		j += 0x0F;
	}

	if (!fill_zero && align == 1) {
		while (i > (long long)n_width + 2*!!alt) {
			OUT(' ');
			i--;
		}
	}

	if (alt) {
		OUT('0');
		OUT(caps ? 'X' : 'x');
	}

	if (fill_zero && align == 1) {
		while (i > (long long)n_width + 2*!!alt) {
			OUT('0');
			i--;
		}
	}

	i = (long long)n_width;
	while (i-- > 0) {
		char c = (caps ? "0123456789ABCDEF" : "0123456789abcdef")[(value>>(i*4))&0xF];
		OUT(c);
	}

	if (align == 0) {
		i = width;
		while (i > (long long)n_width + 2*!!alt) {
			OUT(' ');
			i--;
		}
	}

	return written;
}

/*
 * vasprintf()
 */
size_t xvasprintf(int (*callback)(void *, char), void * userData, const char * fmt, va_list args) {
	const char * s;
	size_t written = 0;
	for (const char *f = fmt; *f; f++) {
		if (*f != '%') {
			OUT(*f);
			continue;
		}
		++f;
		unsigned int arg_width = 0;
		int align = 1; /* right */
		int fill_zero = 0;
		int big = 0;
		int alt = 0;
		int always_sign = 0;
		int precision = -1;
		while (1) {
			if (*f == '-') {
				align = 0;
				++f;
			} else if (*f == '#') {
				alt = 1;
				++f;
			} else if (*f == '*') {
				arg_width = (int)va_arg(args, int);
				++f;
			} else if (*f == '0') {
				fill_zero = 1;
				++f;
			} else if (*f == '+') {
				always_sign = 1;
				++f;
			} else if (*f == ' ') {
				always_sign = 2;
				++f;
			} else {
				break;
			}
		}
		while (*f >= '0' && *f <= '9') {
			arg_width *= 10;
			arg_width += *f - '0';
			++f;
		}
		if (*f == '.') {
			++f;
			precision = 0;
			if (*f == '*') {
				precision = (int)va_arg(args, int);
				++f;
			} else  {
				while (*f >= '0' && *f <= '9') {
					precision *= 10;
					precision += *f - '0';
					++f;
				}
			}
		}
		if (*f == 'l') {
			big = 1;
			++f;
			if (*f == 'l') {
				big = 2;
				++f;
			}
		}
		if (*f == 'j') {
			big = (sizeof(uintmax_t) == sizeof(unsigned long long) ? 2 :
				   sizeof(uintmax_t) == sizeof(unsigned long) ? 1 : 0);
			++f;
		}
		if (*f == 'z') {
			big = (sizeof(size_t) == sizeof(unsigned long long) ? 2 :
				   sizeof(size_t) == sizeof(unsigned long) ? 1 : 0);
			++f;
		}
		if (*f == 't') {
			big = (sizeof(ptrdiff_t) == sizeof(unsigned long long) ? 2 :
				   sizeof(ptrdiff_t) == sizeof(unsigned long) ? 1 : 0);
			++f;
		}
		/* fmt[i] == '%' */
		switch (*f) {
			case 's': /* String pointer -> String */
				{
					size_t count = 0;
					if (big) {
						return written;
					} else {
						s = (char *)va_arg(args, char *);
						if (s == NULL) {
							s = "(null)";
						}
						if (precision >= 0) {
							while (*s && precision > 0) {
								OUT(*s++);
								count++;
								precision--;
								if (arg_width && count == arg_width) break;
							}
						} else {
							while (*s) {
								OUT(*s++);
								count++;
								if (arg_width && count == arg_width) break;
							}
						}
					}
					while (count < arg_width) {
						OUT(' ');
						count++;
					}
				}
				break;
			case 'c': /* Single character */
				OUT((char)va_arg(args,int));
				break;
			case 'p':
				alt = 1;
				if (sizeof(void*) == sizeof(long long)) big = 2;
				/* fallthrough */
			case 'X':
			case 'x': /* Hexadecimal number */
				{
					unsigned long long val;
					if (big == 2) {
						val = (unsigned long long)va_arg(args, unsigned long long);
					} else if (big == 1) {
						val = (unsigned long)va_arg(args, unsigned long);
					} else {
						val = (unsigned int)va_arg(args, unsigned int);
					}
					written += print_hex(val, arg_width, callback, userData, fill_zero, alt, !(*f & 32), align);
				}
				break;
			case 'i':
			case 'd': /* Decimal number */
				{
					long long val;
					if (big == 2) {
						val = (long long)va_arg(args, long long);
					} else if (big == 1) {
						val = (long)va_arg(args, long);
					} else {
						val = (int)va_arg(args, int);
					}
					if (val < 0) {
						OUT('-');
						val = -val;
					} else if (always_sign) {
						OUT(always_sign == 2 ? ' ' : '+');
					}
					written += print_dec(val, arg_width, callback, userData, fill_zero, align, precision);
				}
				break;
			case 'u': /* Unsigned ecimal number */
				{
					unsigned long long val;
					if (big == 2) {
						val = (unsigned long long)va_arg(args, unsigned long long);
					} else if (big == 1) {
						val = (unsigned long)va_arg(args, unsigned long);
					} else {
						val = (unsigned int)va_arg(args, unsigned int);
					}
					written += print_dec(val, arg_width, callback, userData, fill_zero, align, precision);
				}
				break;
			case 'G':
			case 'F':
			case 'g':
			case 'f':
				{
					if (precision == -1) precision = 8; // To 8 decimal points
					double val = (double)va_arg(args, double);
					uint64_t asBits;
					memcpy(&asBits, &val, sizeof(double));
#define SIGNBIT(d) (d & 0x8000000000000000UL)

					/* Extract exponent */
					int64_t exponent = (asBits & 0x7ff0000000000000UL) >> 52;

					/* Fraction part */
					uint64_t fraction = (asBits & 0x000fffffffffffffUL);

					if (exponent == 0x7ff) {
						if (!fraction) {
							if (SIGNBIT(asBits)) OUT('-');
							OUT('i');
							OUT('n');
							OUT('f');
						} else {
							OUT('n');
							OUT('a');
							OUT('n');
						}
						break;
					} else if ((*f == 'g' || *f == 'G') && exponent == 0 && fraction == 0) {
						if (SIGNBIT(asBits)) OUT('-');
						OUT('0');
						break;
					}

					/* Okay, now we can do some real work... */

					int isNegative = !!SIGNBIT(asBits);
					if (isNegative) {
						OUT('-');
						val = -val;
					}

					written += print_dec((unsigned long long)val, arg_width, callback, userData, fill_zero, align, 1);
					OUT('.');
					for (int j = 0; j < ((precision > -1 && precision < 16) ? precision : 16); ++j) {
						if ((unsigned long long)(val * 100000.0) % 100000 == 0 && j != 0) break;
						val = val - (unsigned long long)val;
						val *= 10.0;
						double roundy = ((double)(val - (unsigned long long)val) - 0.99999);
						if (roundy < 0.00001 && roundy > -0.00001 && ((unsigned long long)(val) % 10) != 9) {
							written += print_dec((unsigned long long)(val) % 10 + 1, 0, callback, userData, 0, 0, 1);
							break;
						}
						written += print_dec((unsigned long long)(val) % 10, 0, callback, userData, 0, 0, 1);
					}
				}
				break;
			case '%': /* Escape */
				OUT('%');
				break;
			default: /* Nothing at all, just dump it */
				OUT(*f);
				break;
		}
	}
	return written;
}

struct CBData {
	char * str;
	size_t size;
	size_t written;
};

static int cb_sprintf(void * user, char c) {
	struct CBData * data = user;
	if (data->size > data->written + 1) {
		data->str[data->written] = c;
		data->written++;
		if (data->written < data->size) {
			data->str[data->written] = '\0';
		}
	}
	return 0;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
	struct CBData data = {str,size,0};
	int out = xvasprintf(cb_sprintf, &data, format, ap);
	cb_sprintf(&data, '\0');
	return out;
}

int snprintf(char * str, size_t size, const char * format, ...) {
	struct CBData data = {str,size,0};
	va_list args;
	va_start(args, format);
	int out = xvasprintf(cb_sprintf, &data, format, args);
	va_end(args);
	cb_sprintf(&data, '\0');
	return out;
}

static int cb_sxprintf(void * user, char c) {
	struct CBData * data = user;
	data->str[data->written] = c;
	data->written++;
	return 0;
}


int sprintf(char * str, const char * format, ...) {
	struct CBData data = {str,0,0};
	va_list args;
	va_start(args, format);
	int out = xvasprintf(cb_sxprintf, &data, format, args);
	va_end(args);
	cb_sxprintf(&data, '\0');
	return out;
}

int cb_printf(void * user, char c) {
	putchar(c);
	return 0;
}



// Why do we call this boot_printf()?
// Because GCC is stupid and will try to pull from stdio.h, and cause this function to just not be called.
// It annoys me, I'd rather be more libc-like, but whatever.
int boot_printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int out = xvasprintf(cb_printf, NULL, fmt, args);
	va_end(args);
	return out;
}