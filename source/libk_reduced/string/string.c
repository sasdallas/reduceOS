// string.c - replacement for standard C string file. Contains certain useful functions like strlen.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.
// Note that while I try my best to adhere to C standards, none of the libc function for the kernel will ever be used by an application, it's to make the code more easier to read.

#include <libk_reduced/string.h>

// Macros for assistance (ToaruOS)
#define BITOP(A, B, OP) ((A)[(size_t)(B)/(8*sizeof *(A))] OP (size_t)1<<((size_t)(B)%(8*sizeof *(A))))
#define ONES ((size_t)-1/UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX/2+1))
#define HASZERO(X) (((X)-ONES) & ~(X) & HIGHS)
#define ALIGN (sizeof(size_t))
#define MAX(X, Y)  ((X > Y) ? X : Y)



// memcmp() - Comparing addresses/values in memory
// Three parameters - objects 1 and 2(const void*) and the amount of bytes to compare(size_t)

int memcmp(const void *s1, const void *s2, size_t n) {
    unsigned char *p1 = s1, *p2 = s2; // Changing them to types to avoid errors with the compiler
    
    if (s1 == s2) return 0; // Both pointers are pointing towards the same memory block. We don't need to check them.

    while (n > 0) {
        if (*p1 != *p2) return (*p1 > *p2) ? 1:-1; // If they are not the same, return either 1 or -1 depending on whether the mismatching character was greater or smaller than the other.
        n--; // Decrement n and increment p1 and p2 to move on to the next char
        p1++;
        p2++;
    }
}

// memcpy() - Copy a block of data from a source address to a destination address.
// Three parameters - destination block(void*), source block(void*),  and amount of bytes to copy(size_t)

void* memcpy(void* __restrict destination, const void* __restrict source, size_t n) {
    // Typecast both addresses to char* and assign them to csrc and cdest.
    char *csrc = (char *)source, *cdest = (char *)destination;

    // Now copy the contents of source to destination.
    for (int i = 0; i < n; i++) { cdest[i] = csrc[i]; }
}



// memmove() - Move a block data from a source address to a destination address
// Three parameters - source block(void*), destination block(void*), and amount of bytes to move(size_t)

void* memmove(void* destination, const void* source, size_t n) {
    // First, we need to typecast destination to an unsigned char*, as well as typecast source to a constant unsigned char.
    unsigned char* dst = (unsigned char*) dst;
    const unsigned char* src = (const unsigned char*) source;

    // If the destination is smaller than the source, we can do a normal for loop with i < n.
    if (dst < src) {
        for (size_t i = 0; i < n; i++) 
            dst[i] = src[i];
        
    } else {
        // If that's not the case, the for loop can decrement i.
        for (size_t i = n; i != 0; i--)
            dst[i-1] = src[i-1]; // i-1 accounts for the indexes.
    }

    // Now just simply return destination (same type as function return, void*)
    return destination;
}


// memset() - Set a buffer in memory to a given value.
// Three parameters - buffer, value, amount of times to set

void* memset(void *buf, char c, size_t n) {
    unsigned char *temp = (unsigned char *)buf;
    for (; n != 0; n--) temp[n-1] = c;
    return buf;
}

// strlen() - Returns the length of a string(size_t)
// One parameter - str
int strlen(char *str) {
    int i = 0;
    while (*str++) {
        i++;
    }
    return i;
}


// itoa() - converts an integer to a string
// Three parameters - Integer, string buffer, integer base (1-16)
void itoa(void *n, char *buffer, int base) {
    static char bchars[] = {"FEDCBA9876543210123456789ABCDEF"};
    char tbuf[32]; // Temporary buffer
    int pos = 0;
    int opos = 0;
    int top = 0;

    int num = (int)n;

    if (num < 0 && base == 10) { // We need to do a little extra work if the value is negative.
        *buffer++ = '-';
    }

    if (num == 0 || base > 16) { // Don't even bother if the base is greater than 16 or the number is 0.
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    // First, get the character
    while (num != 0) {
        tbuf[pos] = bchars[15 + num % base];
        pos++;
        num /= base;
    }

    top = pos--;

    for (opos = 0; opos < top; pos--,opos++) 
        buffer[opos] = tbuf[pos];
    
    buffer[opos] = 0;
}


// itoa_long(uint64_t value, char *str, int base) - Disgusting hack to make longs work with itoa()
void itoa_long(uint64_t value, char* str, int base) {
    if (base < 2 || base > 36) {
        *str = '\0'; // Invalid base; return an empty string
        return str;
    }

    char* digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char buffer[65]; // Enough to handle uint64_t in binary (base 2)
    char* ptr = buffer + sizeof(buffer) - 1;
    *ptr = '\0'; // Null-terminate the buffer

    // Handle zero explicitly
    if (value == 0) {
        *--ptr = digits[0];
    } else {
        // Convert the number to the specified base
        while (value != 0) {
            *--ptr = digits[value % base];
            value /= base;
        }
    }

    // Copy the result to the output string
    while (*ptr) {
        *str++ = *ptr++;
    }
    *str = '\0'; // Null-terminate the result string
}



// strcpy() - copies a string to a string.
// Two parameters - destination and source.
char *strcpy(char *dest, const char *src) {
    char *dest_p = dest;
    while (*dest++ = *src++); // Copy all the contents of src to destination.
    return dest_p;
}

// toupper() - turns a char uppercase
// One parameter - char.
char toupper(char c) {
    if ((c >= 'a') && (c <= 'z'))
        return (c - 32);
    return c;
}


// tolower() - turns a char lowercase
// One parameter - char.
char tolower(char c) {
    if ((c >= 'A') && (c <= 'Z'))
        return (c + 32);
    return c;
}

// isalpha() - returns if a char is in the alphabet
int isalpha(char ch) {
    return (((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z')));
}

// strcmp() - compares s1 to s2
// Two parameters - strings #1 and #2.
int strcmp (const char* str1, char *str2) {
    int i = 0;

    while ((str1[i] == str2[i])) {
        if (str2[i++] == 0)
            return 0;
    }
    return 1;
}

// strncmp() - Compares str1 to str2 for x amount of chars.
// Three parameters - strings #1 and #2 and the # of chars
int strncmp(const char *str1, char *str2, int length) {
    int i = 0;

    while ((str1[i] == str2[i])) {
        if (str2[i++] == 0)
            return 0;
    }
    return 1;
}

// atoi() - Converts a string to an integer
// Needs more parameters and isn't complete.
int atoi(char *str) {
    int ret = 0;
    for (int i = 0; i < strlen(str); i++) {
        ret = ret * 10 + str[i] - '0';
    }

    return ret;
}

// strtok() - Splits a string into tokens, seperated by characters in delim.
char *strtok(char *str, const char *delim) {
    static int currentIndex = 0;

    if (!str || !delim || str[currentIndex] == '\0') return NULL;

    // Allocate memory for character.
    char *w = (char*)kmalloc(sizeof(char)*100);

    int i = currentIndex, k = 0, j = 0;
    while (str[i] != '\0') {
        j = 0;
        while (delim[j] != '\0') {
            if (str[i] != delim[j]) w[k] = str[i];
            else goto Done;
            j++;
        }

        i++;
        k++;
    }

Done:
    w[i] = 0;
    currentIndex = i+1;
    return w;
}


// Strtol implementation that needs some cleaning up and possibly refactoring
// EXACT Source: https://github.com/gcc-mirror/gcc/blob/master/libiberty/strtol.c

#define ISDIGIT(c) ((c) >= '0' && (c) <= '9')
#define ISUPPER(c) ((c) >= 'A' && (c) <= 'Z')
#define ISALPHA(c) (((c) >= 'a' && (c) <= 'z') | ((c) >= 'A' && (c) <= 'Z'))

long strtol(const char *nptr, char **endptr, int base) {
    const char *s = nptr;
    unsigned long acc;
    int c;
    unsigned long cutoff;
    int neg = 0, any, cutlim;

    /*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (c == ' ');
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

    /*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long)base;
	cutoff /= (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (ISDIGIT(c))
			c -= '0';
		else if (ISALPHA(c))
			c -= ISUPPER(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
		panic("string.c", "strtol", "Out of range exception");
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}

// strchr(char *str, int ch) - Locate the first occurance of a character in a string (credit to BrokenThorn Entertainment)
char *strchr(char *str, int ch) {
    do {
        if (*str == ch) {
            return (char*)str;
        }
    } while (*str++);

    return 0; // Nothing.
}

// strchrnul(const char *str, int ch) - Locates a character in a string (why do we need this?)
char *strchrnul(const char *str, int ch) {
    size_t *w;
    size_t k;

    ch = (uint8_t)ch;
    if (!ch) return (char*)str + strlen(str); // Return basically what is a NULL

    for (; (uintptr_t)str % sizeof(size_t); str++) {
        if (!*str || *(unsigned char *)str == ch) return (char*)str;
    }

    k = ONES * ch;
    for (w = (void*)str; !HASZERO(*w) && !HASZERO(*w^k); w++);
    for (str = (void*)w; *str && *(unsigned char*)str != ch; str++);

    return (char*)str;
}


// strcspn(const char *str1, const char *reject) - Scans str1 for the first occurence of any of the characters that are NOT part of reject.
size_t strcspn(const char *str1, const char *reject) {
    const char *a = str1;
    if (reject[0] && reject[1]) {
        size_t byteset[32 / sizeof(size_t)] = {0};
        for (; *reject && BITOP(byteset, *(unsigned char *)reject, |=); reject++);
        for (; *str1 && !BITOP(byteset, *(unsigned char *)str1, &); str1++);
        return str1 - a;
    }

    return strchrnul(str1, *reject) - a;
}

// strspn(const char *str1, const char *accept) - Scans str1 for the first occurence of any of the characters that are part of accept.
size_t strspn(const char *str1, const char *accept) {
    const char *a = str1;
    size_t byteset[32 / sizeof(size_t)]  = {0};

    if (!accept[0]) return 0;

    if (!accept[1]) {
        for (; *str1 == *accept; str1++);
        return str1 - a;
    }

    for (; *accept && BITOP(byteset, *(unsigned char *)accept, |=); accept++);
    for (; *str1 && BITOP(byteset, *(unsigned char *)str1, &); str1++);
    
    return str1 - a;
}

// strpbrk(const char *s, const char *b) - Returns a pointer to the first occurence of b within s.
char *strpbrk(const char *s, const char *b) {
    s += strcspn(s, b);
    return *s ? (char*)s : 0;
}

// strtok_r(char *str, const char *delim, char **saveptr) - Thread-safe version of strtok
char *strtok_r(char *str, const char *delim, char **saveptr) {
    char *token;
    if (str == NULL) str = *saveptr;

    str += strspn(str, delim);
    if (*str == '\0') {
        *saveptr = str;
        return NULL; // No occurence
    }

    token = str;
    str = strpbrk(token, delim);
    if (str == NULL) {
        *saveptr = (char*)((size_t)strchr(token, '\0'));
    } else {
        *str = '\0';
        *saveptr = str + 1;
    }

    return token;
}


// strncat(char *dest, const char *src, size_t n) - Concatenate strings together (ToaruOS impl.)
char *strncat(char *dest, const char *src, size_t n) {
    char *end = dest;
    while (*end != '\0') ++end;

    size_t i = 0;
    while (*src && i < n) {
        *end = *src;
        end++;
        src++;
        i++;
    }

    *end = '\0';
    return dest;
}


/* THE FOLLOWING FUNCTIONS, STRSTR SPECIFICALLY ARE COPY-PASTED FROM TOARUOS. I OWN NONE OF THEM. */
static char *strstr_2b(const unsigned char * h, const unsigned char * n) {
	uint16_t nw = n[0] << 8 | n[1];
	uint16_t hw = h[0] << 8 | h[1];
	for (h++; *h && hw != nw; hw = hw << 8 | *++h);
	return *h ? (char *)h-1 : 0;
}

static char *strstr_3b(const unsigned char * h, const unsigned char * n) {
	uint32_t nw = n[0] << 24 | n[1] << 16 | n[2] << 8;
	uint32_t hw = h[0] << 24 | h[1] << 16 | h[2] << 8;
	for (h += 2; *h && hw != nw; hw = (hw|*++h) << 8);
	return *h ? (char *)h-2 : 0;
}

static char *strstr_4b(const unsigned char * h, const unsigned char * n) {
	uint32_t nw = n[0] << 24 | n[1] << 16 | n[2] << 8 | n[3];
	uint32_t hw = h[0] << 24 | h[1] << 16 | h[2] << 8 | h[3];
	for (h += 3; *h && hw != nw; hw = hw << 8 | *++h);
	return *h ? (char *)h-3 : 0;
}

static char *strstr_twoway(const unsigned char * h, const unsigned char * n) {
	size_t mem;
	size_t mem0;
	size_t byteset[32 / sizeof(size_t)] = { 0 };
	size_t shift[256];
	size_t l;

	/* Computing length of needle and fill shift table */
	for (l = 0; n[l] && h[l]; l++) {
		BITOP(byteset, n[l], |=);
		shift[n[l]] = l+1;
	}

	if (n[l]) {
		return 0; /* hit the end of h */
	}

	/* Compute maximal suffix */
	size_t ip = -1;
	size_t jp = 0;
	size_t k = 1;
	size_t p = 1;
	while (jp+k<l) {
		if (n[ip+k] == n[jp+k]) {
			if (k == p) {
				jp += p;
				k = 1;
			} else {
				k++;
			}
		} else if (n[ip+k] > n[jp+k]) {
			jp += k;
			k = 1;
			p = jp - ip;
		} else {
			ip = jp++;
			k = p = 1;
		}
	}
	size_t ms = ip;
	size_t p0 = p;

	/* And with the opposite comparison */
	ip = -1;
	jp = 0;
	k = p = 1;
	while (jp+k<l) {
		if (n[ip+k] == n[jp+k]) {
			if (k == p) {
				jp += p;
				k = 1;
			} else {
				k++;
			}
		} else if (n[ip+k] < n[jp+k]) {
			jp += k;
			k = 1;
			p = jp - ip;
		} else {
			ip = jp++;
			k = p = 1;
		}
	}
	if (ip+1 > ms+1) {
		ms = ip;
	} else {
		p = p0;
	}

	/* Periodic needle? */
	if (memcmp(n, n+p, ms+1)) {
		mem0 = 0;
		p = MAX(ms, l-ms-1) + 1;
	} else {
		mem0 = l-p;
	}
	mem = 0;

	/* Initialize incremental end-of-haystack pointer */
	const unsigned char * z = h;

	/* Search loop */
	for (;;) {
		/* Update incremental end-of-haystack pointer */
		if ((size_t)(z-h) < l) {
			/* Fast estimate for MIN(l,63) */
			size_t grow = l | 63;
			const unsigned char *z2 = memchr(z, 0, grow);
			if (z2) {
				z = z2;
				if ((size_t)(z-h) < l) {
					return 0;
				}
			} else {
				z += grow;
			}
		}

		/* Check last byte first; advance by shift on mismatch */
		if (BITOP(byteset, h[l-1], &)) {
			k = l-shift[h[l-1]];
			if (k) {
				if (mem0 && mem && k < p) k = l-p;
				h += k;
				mem = 0;
				continue;
			}
		} else {
			h += l;
			mem = 0;
			continue;
		}

		/* Compare right half */
		for (k=MAX(ms+1,mem); n[k] && n[k] == h[k]; k++);
		if (n[k]) {
			h += k-ms;
			mem = 0;
			continue;
		}
		/* Compare left half */
		for (k=ms+1; k>mem && n[k-1] == h[k-1]; k--);
		if (k <= mem) {
			return (char *)h;
		}
		h += p;
		mem = mem0;
	}
}


void * memchr(const void * src, int c, size_t n) {
	const unsigned char * s = src;
	c = (unsigned char)c;
	for (; ((uintptr_t)s & (ALIGN - 1)) && n && *s != c; s++, n--);
	if (n && *s != c) {
		const size_t * w;
		size_t k = ONES * c;
		for (w = (const void *)s; n >= sizeof(size_t) && !HASZERO(*w^k); w++, n -= sizeof(size_t));
		for (s = (const void *)w; n && *s != c; s++, n--);
	}
	return n ? (void *)s : 0;
}

char *strstr(const char * h, const char * n) {
	/* Return immediately on empty needle */
	if (!n[0]) {
		return (char *)h;
	}

	/* Use faster algorithms for short needles */
	h = strchr(h, *n);
	if (!h || !n[1]) {
		return (char *)h;
	}

	if (!h[1]) return 0;
	if (!n[2]) return strstr_2b((void *)h, (void *)n);
	if (!h[2]) return 0;
	if (!n[3]) return strstr_3b((void *)h, (void *)n);
	if (!h[3]) return 0;
	if (!n[4]) return strstr_4b((void *)h, (void *)n);

	/* Two-way on large needles */
	return strstr_twoway((void *)h, (void *)n);
}


// strdup(const char *s) - Creates a new version of a string and returns it (requires allocation functions)
char *strdup(const char *s) {
	char *output = kmalloc(strlen(s));
	strcpy(output, s);
	return output;
}