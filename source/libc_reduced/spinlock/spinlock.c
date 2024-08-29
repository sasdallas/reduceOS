// spinlock.c - Implements spinlocks to lock/unlock resources

#include <libk_reduced/spinlock.h> // Main header file

// This spinlock implementation is very basic and needs to be improved, but it does work.


// spinlock_init() - Creates a new spinlock
atomic_flag *spinlock_init() {
    atomic_flag *lock = kmalloc(sizeof(atomic_flag));
    atomic_flag_clear(lock);
    return lock;
}


// spinlock_lock(atomic_flag *lock) - Locks the spinlock
void spinlock_lock(atomic_flag *lock) {
    while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire)) {
        asm volatile ("pause"); // IA-32 pause instruction, should prevent P-4s and Xeons from corrupting stuff
    }
}

// spinlock_release(atomic_flag *lock) - Releases the spinlock
void spinlock_release(atomic_flag *lock) {
    atomic_flag_clear_explicit(lock, memory_order_release);
}

