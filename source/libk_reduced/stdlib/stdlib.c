// stdlib.c - replacement for standard C stdlib file. Contains certain useful functions like abs().
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.
// Note that while I try my best to adhere to C standards, none of the libc function for the kernel will ever be used by an application, it's to make the code more easier to read.


#include <libk_reduced/stdlib.h>

// abs(int x) - absolute value
int abs(int x) {
    if (x < 0) x = -x;
    return x;
}

// min(double n1, double n2) - Returns the minimum number
double min(double n1, double n2) {
    if (n1 > n2) return n2;
    return n1; // Just in case the numbers are equal, return n1, because they're the same anyway.
}

// max(double n1, double n2) - Returns the minimum number
double max(double n1, double n2) {
    if (n1 > n2) return n1;
    return n2; // Just in case the numbers are equal, return n2, because they're the same anyway.
}

// sqrt(double n) - Returns square root of a number
double sqrt(double n) {
    // Binary search implementation of square root
    double low = min(1, n);
    double high = max(1, n);
    double mid;

    // Update bounds to be off the target by a factor of 10
    while (100 * low * low < n) low *= 10;
    while (0.01 * high * high > n) high *= 0.1;

    for (int i = 0; i < 100; i++) {
        mid += (low + high) / 2;
        if (mid*mid == n) return mid;
        if (mid*mid > n) high = mid;
        else low = mid;
    }

    return mid;
}
