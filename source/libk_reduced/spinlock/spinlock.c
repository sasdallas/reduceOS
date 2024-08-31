// spinlock.c - Implements spinlocks to lock/unlock resources

#include <libk_reduced/spinlock.h> // Main header file

/* TODO: This sucks */


// spinlock_init() - Creates a new spinlock
spinlock_t *spinlock_init() {
    spinlock_t *lock = kmalloc(sizeof(spinlock_t));
    lock = SPINLOCK_RELEASED;
    return lock;
}


// spinlock_lock(spinlock_t *lock) - Locks the spinlock
void spinlock_lock(spinlock_t *lock) {
    while (lock == SPINLOCK_LOCKED) {
        asm volatile ("pause"); // IA-32 pause instruction, should prevent P-4s and Xeons from corrupting stuff
    }
    lock = SPINLOCK_LOCKED;
}

// spinlock_release(spinlock_t *lock) - Releases the spinlock
void spinlock_release(spinlock_t *lock) {
    lock = SPINLOCK_RELEASED;
}

