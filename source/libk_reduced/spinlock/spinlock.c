// spinlock.c - Implements spinlocks to lock/unlock resources

#include <libk_reduced/spinlock.h> // Main header file

// This spinlock implementation is very basic and needs to be improved, but it does work.


// spinlock_init() - Creates a new spinlock
spinlock_t *spinlock_init() {
    spinlock_t *lock = kmalloc(sizeof(spinlock_t));
    atomic_flag_clear(lock);
    return lock;
}


// spinlock_lock(spinlock_t *lock) - Locks the spinlock
void spinlock_lock(spinlock_t *lock) {
    while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire)) {
        asm volatile ("pause"); // IA-32 pause instruction, should prevent P-4s and Xeons from corrupting stuff
    }
}

// spinlock_release(spinlock_t *lock) - Releases the spinlock
void spinlock_release(spinlock_t *lock) {
    atomic_flag_clear_explicit(lock, memory_order_release);
}

