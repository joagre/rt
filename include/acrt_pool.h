#ifndef ACRT_POOL_H
#define ACRT_POOL_H

#include <stddef.h>
#include <stdbool.h>

// Simple fixed-size object pool allocator
// Used for static allocation of mailbox entries, link entries, etc.

typedef struct acrt_pool {
    void   *entries;       // Pointer to static array of entries
    bool   *used;          // Bitmap of which entries are in use
    size_t  entry_size;    // Size of each entry in bytes
    size_t  capacity;      // Total number of entries
    size_t  allocated;     // Number of currently allocated entries
} acrt_pool;

// Initialize a pool with a static array
// entries: pointer to static array (e.g., static mailbox_entry pool[256])
// used: pointer to static bool array (e.g., static bool used[256])
// entry_size: sizeof(entry_type)
// capacity: number of entries in the array
void acrt_pool_init(acrt_pool *pool, void *entries, bool *used,
                  size_t entry_size, size_t capacity);

// Allocate an entry from the pool
// Returns NULL if pool is exhausted
void* acrt_pool_alloc(acrt_pool *pool);

// Free an entry back to the pool
// entry must have been allocated from this pool
void acrt_pool_free(acrt_pool *pool, void *entry);

#endif // ACRT_POOL_H
