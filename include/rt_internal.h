#ifndef RT_INTERNAL_H
#define RT_INTERNAL_H

#include "rt_static_config.h"
#include "rt_types.h"
#include "rt_timer.h"
#include "rt_actor.h"
#include "rt_io_source.h"
#include <stddef.h>
#include <stdbool.h>

// Internal shared types and macros for runtime implementation
// This header is NOT part of the public API

// Message data entry type (shared by IPC, bus, link, timer subsystems)
typedef struct {
    uint8_t data[RT_MAX_MESSAGE_SIZE];
} message_data_entry;

// Macro to extract message_data_entry pointer from data pointer
// Used when freeing message data back to the pool
#define DATA_TO_MSG_ENTRY(data_ptr) \
    ((message_data_entry*)((char*)(data_ptr) - offsetof(message_data_entry, data)))

// -----------------------------------------------------------------------------
// Shared Linked List Macros
// -----------------------------------------------------------------------------

// Remove entry from singly-linked list using pointer-to-pointer pattern
// Usage: SLIST_REMOVE(head, entry_to_remove)
#define SLIST_REMOVE(head, entry_to_remove) \
    do { \
        __typeof__(head) *_prev = &(head); \
        while (*_prev && *_prev != (entry_to_remove)) { \
            _prev = &(*_prev)->next; \
        } \
        if (*_prev) { \
            *_prev = (*_prev)->next; \
        } \
    } while(0)

// Append entry to singly-linked list
// Usage: SLIST_APPEND(head, new_entry)
#define SLIST_APPEND(head, new_entry) \
    do { \
        (new_entry)->next = NULL; \
        if (head) { \
            __typeof__(head) _last = head; \
            while (_last->next) _last = _last->next; \
            _last->next = (new_entry); \
        } else { \
            (head) = (new_entry); \
        } \
    } while(0)

// Find and remove entry matching condition from singly-linked list
// Returns the removed entry or NULL if not found
// Usage: entry = SLIST_FIND_REMOVE(head, entry->field == value)
#define SLIST_FIND_REMOVE(head, condition, removed_entry) \
    do { \
        __typeof__(head) *_prev = &(head); \
        __typeof__(head) _curr = (head); \
        (removed_entry) = NULL; \
        while (_curr) { \
            __typeof__(head) entry = _curr; /* for use in condition */ \
            if (condition) { \
                *_prev = _curr->next; \
                (removed_entry) = _curr; \
                break; \
            } \
            _prev = &_curr->next; \
            _curr = _curr->next; \
        } \
    } while(0)

// Initialization guard macro - early return if already initialized
// Used by: All subsystem init functions
#define RT_INIT_GUARD(initialized_flag) \
    do { \
        if (initialized_flag) { \
            return RT_SUCCESS; \
        } \
    } while(0)

// Cleanup guard macro - early return if not initialized
// Used by: All subsystem cleanup functions
#define RT_CLEANUP_GUARD(initialized_flag) \
    do { \
        if (!(initialized_flag)) { \
            return; \
        } \
    } while(0)

// Actor context requirement macro - returns error if not in actor context
// Used by: IPC, link, bus, timer, network APIs
#define RT_REQUIRE_ACTOR_CONTEXT() \
    do { \
        if (!rt_actor_current()) { \
            return RT_ERROR(RT_ERR_INVALID, "Not called from actor context"); \
        } \
    } while(0)

// Internal helper functions (implemented in rt_ipc.c)

// Free message data back to the shared message pool
// Handles NULL safely. Used by: IPC, bus, link subsystems
void rt_msg_pool_free(void *data);

// Add mailbox entry to actor's mailbox and wake if blocked
// Used by: timer, link subsystems (via rt_ipc_send_ex)
void rt_mailbox_add_entry(actor *recipient, mailbox_entry *entry);

// Check for timeout message in mailbox and dequeue if present
// Returns RT_ERR_TIMEOUT if timeout occurred, otherwise cancels timer and returns RT_SUCCESS
// Used by: IPC recv, network I/O, bus
rt_status rt_mailbox_handle_timeout(actor *current, timer_id timeout_timer, const char *operation);

// Free a mailbox entry and its associated data buffers
// Used by: IPC, mailbox clear, actor cleanup
void rt_ipc_free_entry(mailbox_entry *entry);

// Dequeue the head entry from an actor's mailbox
// Returns NULL if mailbox is empty
// Used by: IPC recv, timeout handling, bus
mailbox_entry *rt_ipc_dequeue_head(actor *a);

// Actor crash handler (called when actor returns without rt_exit)
// Sets RT_EXIT_CRASH and yields to scheduler - never returns
_Noreturn void rt_exit_crash(void);

// Event loop handlers (called by scheduler when I/O sources become ready)

// Handle timer event (timerfd ready)
void rt_timer_handle_event(io_source *source);

// Handle network event (socket ready)
void rt_net_handle_event(io_source *source);

#endif // RT_INTERNAL_H
