#include "hive_select.h"
#include "hive_internal.h"
#include "hive_static_config.h"
#include "hive_actor.h"
#include "hive_scheduler.h"
#include "hive_timer.h"
#include "hive_bus.h"
#include "hive_log.h"
#include <string.h>

// Static buffer for bus data (single-threaded, one actor at a time)
static uint8_t s_bus_data_buffer[HIVE_MAX_MESSAGE_SIZE];
static size_t s_bus_data_len = 0;

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------

// Scan sources for ready data (non-blocking)
// Returns true if data found, populates result
// Priority: bus sources first (in array order), then IPC sources (in array order)
static bool scan_sources(const hive_select_source *sources, size_t num_sources,
                         hive_select_result *result) {
    // First pass: check all bus sources (higher priority)
    for (size_t i = 0; i < num_sources; i++) {
        if (sources[i].type == HIVE_SEL_BUS) {
            if (hive_bus_has_data(sources[i].bus)) {
                // Read bus data into static buffer
                size_t actual_len = 0;
                hive_status status =
                    hive_bus_read(sources[i].bus, s_bus_data_buffer,
                                  sizeof(s_bus_data_buffer), &actual_len);
                if (HIVE_SUCCEEDED(status)) {
                    result->index = i;
                    result->type = HIVE_SEL_BUS;
                    result->bus.data = s_bus_data_buffer;
                    result->bus.len = actual_len;
                    s_bus_data_len = actual_len;
                    HIVE_LOG_TRACE("select: bus source %zu ready, %zu bytes", i,
                                   actual_len);
                    return true;
                }
            }
        }
    }

    // Second pass: check all IPC sources (lower priority)
    for (size_t i = 0; i < num_sources; i++) {
        if (sources[i].type == HIVE_SEL_IPC) {
            size_t matched_idx = 0;
            mailbox_entry *entry =
                hive_ipc_scan_mailbox(&sources[i].ipc, 1, &matched_idx);
            if (entry) {
                // Found matching IPC message
                hive_ipc_consume_entry(entry, &result->ipc);
                result->index = i;
                result->type = HIVE_SEL_IPC;
                HIVE_LOG_TRACE("select: IPC source %zu ready", i);
                return true;
            }
        }
    }

    return false;
}

// Clear all bus blocked flags for sources
static void clear_bus_blocked_flags(const hive_select_source *sources,
                                    size_t num_sources) {
    for (size_t i = 0; i < num_sources; i++) {
        if (sources[i].type == HIVE_SEL_BUS) {
            hive_bus_set_blocked(sources[i].bus, false);
        }
    }
}

// Set all bus blocked flags for sources
static void set_bus_blocked_flags(const hive_select_source *sources,
                                  size_t num_sources) {
    for (size_t i = 0; i < num_sources; i++) {
        if (sources[i].type == HIVE_SEL_BUS) {
            hive_bus_set_blocked(sources[i].bus, true);
        }
    }
}

// -----------------------------------------------------------------------------
// hive_select implementation
// -----------------------------------------------------------------------------

hive_status hive_select(const hive_select_source *sources, size_t num_sources,
                        hive_select_result *result, int32_t timeout_ms) {
    HIVE_REQUIRE_ACTOR_CONTEXT();
    actor *current = hive_actor_current();

    // Validate inputs
    if (!sources) {
        return HIVE_ERROR(HIVE_ERR_INVALID, "NULL sources array");
    }
    if (num_sources == 0) {
        return HIVE_ERROR(HIVE_ERR_INVALID, "No sources specified");
    }
    if (!result) {
        return HIVE_ERROR(HIVE_ERR_INVALID, "NULL result pointer");
    }

    // Validate bus sources are subscribed
    for (size_t i = 0; i < num_sources; i++) {
        if (sources[i].type == HIVE_SEL_BUS) {
            if (!hive_bus_is_subscribed(sources[i].bus)) {
                return HIVE_ERROR(HIVE_ERR_INVALID,
                                  "Bus source not subscribed");
            }
        }
    }

    HIVE_LOG_TRACE("select: actor %u waiting on %zu sources", current->id,
                   num_sources);

    // Non-blocking scan
    if (scan_sources(sources, num_sources, result)) {
        return HIVE_SUCCESS;
    }

    // Nothing ready
    if (timeout_ms == 0) {
        return HIVE_ERROR(HIVE_ERR_WOULDBLOCK, "No data available");
    }

    // Set up for blocking
    current->select_sources = sources;
    current->select_source_count = num_sources;

    // Mark bus subscribers as blocked
    set_bus_blocked_flags(sources, num_sources);

    // Create timeout timer if needed
    timer_id timeout_timer = TIMER_ID_INVALID;
    if (timeout_ms > 0) {
        hive_status status =
            hive_timer_after((uint32_t)timeout_ms * 1000, &timeout_timer);
        if (HIVE_FAILED(status)) {
            current->select_sources = NULL;
            current->select_source_count = 0;
            clear_bus_blocked_flags(sources, num_sources);
            return status;
        }
    }

    // Block and wait
    current->state = ACTOR_STATE_WAITING;
    hive_scheduler_yield();

    // Woken up - clear state
    current->select_sources = NULL;
    current->select_source_count = 0;
    clear_bus_blocked_flags(sources, num_sources);

    // Check for timeout
    if (timeout_timer != TIMER_ID_INVALID) {
        hive_status timeout_status = hive_mailbox_handle_timeout(
            current, timeout_timer, "Select timeout");
        if (HIVE_FAILED(timeout_status)) {
            return timeout_status;
        }
    }

    // Re-scan for data
    if (scan_sources(sources, num_sources, result)) {
        return HIVE_SUCCESS;
    }

    // Spurious wakeup - no matching data found
    return HIVE_ERROR(HIVE_ERR_WOULDBLOCK, "No data available after wakeup");
}
