# Unified hive_wait() API Design Sketch

**Status:** Idea / Future consideration

## Motivation

Currently, actors must choose one blocking primitive at a time:
- `hive_ipc_recv()` / `hive_ipc_recv_matches()` - wait for messages
- `hive_bus_read_wait()` - wait for bus data
- `hive_net_recv()` / `hive_net_send()` - wait for network I/O

This forces awkward patterns when an actor needs to respond to multiple event sources (e.g., timer ticks AND sensor bus data AND commands). A unified wait would enable clean event loops.

## Proposed API

```c
// Wait condition types
typedef enum {
    HIVE_WAIT_IPC,      // Message matching filter
    HIVE_WAIT_BUS,      // Bus data available
    HIVE_WAIT_NET_RECV, // Socket readable
    HIVE_WAIT_NET_SEND, // Socket writable
} hive_wait_type;

// Wait condition (tagged union)
typedef struct {
    hive_wait_type type;
    union {
        hive_recv_filter ipc;    // For HIVE_WAIT_IPC
        bus_id bus;              // For HIVE_WAIT_BUS
        int fd;                  // For HIVE_WAIT_NET_*
    };
} hive_wait_cond;

// Wait result
typedef struct {
    size_t index;               // Which condition triggered (into conds array)
    hive_wait_type type;        // Convenience copy of triggered type
    union {
        hive_message ipc;       // For HIVE_WAIT_IPC
        struct {                // For HIVE_WAIT_BUS
            void *data;
            size_t len;
        } bus;
        int fd;                 // For HIVE_WAIT_NET_*
    };
} hive_wait_result;

// Unified wait - blocks until any condition is satisfied
hive_status hive_wait(const hive_wait_cond *conds, size_t num_conds,
                      hive_wait_result *result, int32_t timeout_ms);
```

## Usage Example

```c
enum { WAIT_TIMER, WAIT_SENSOR_BUS, WAIT_COMMAND };
hive_wait_cond conds[] = {
    [WAIT_TIMER] = {HIVE_WAIT_IPC, .ipc = {HIVE_SENDER_ANY, HIVE_MSG_TIMER, my_timer}},
    [WAIT_SENSOR_BUS] = {HIVE_WAIT_BUS, .bus = sensor_bus},
    [WAIT_COMMAND] = {HIVE_WAIT_IPC, .ipc = {HIVE_SENDER_ANY, HIVE_MSG_NOTIFY, CMD_TAG}},
};

hive_wait_result result;
hive_wait(conds, 3, &result, -1);

switch (result.index) {
    case WAIT_TIMER:
        handle_tick();
        break;
    case WAIT_SENSOR_BUS:
        process_sensor(result.bus.data, result.bus.len);
        break;
    case WAIT_COMMAND:
        handle_command(&result.ipc);
        break;
}
```

## Implementation Notes

- Internally scans IPC mailbox and bus buffers first (non-blocking check)
- If nothing ready, registers all conditions with scheduler
- Wakes on first match from any source
- Timers still delivered as IPC messages (no change needed)
- Zero heap allocation - all structures caller-provided
- O(n) scan where n = number of conditions + mailbox depth + bus entries

## Open Questions

1. **Bus data lifetime:** Should bus data be copied into result, or use same lifetime rules as current APIs? Current bus API has "valid until next read" semantics.

2. **Network buffer management:** Network conditions might need caller-provided buffers and size hints for the actual recv/send operation.

3. **Simpler alternative:** Should there be a `hive_wait_any()` variant that takes separate arrays per type? Less flexible but simpler struct layout:
   ```c
   hive_status hive_wait_any(
       const hive_recv_filter *ipc_filters, size_t num_ipc,
       const bus_id *buses, size_t num_buses,
       const int *fds, size_t num_fds,
       hive_wait_result *result, int32_t timeout_ms);
   ```

4. **Priority:** If multiple conditions are satisfied simultaneously, which wins? First in array? By type priority?

5. **STM32 support:** Network I/O not yet implemented on STM32. Should `HIVE_WAIT_NET_*` be compile-time excluded on that platform?

## Efficiency Considerations

**Why O(n) scanning is acceptable:**
- Condition counts are tiny (typically 2-5 in an event loop)
- Mailbox/bus depths bounded by pool sizes (256 default)
- Current `hive_ipc_recv_matches()` already does O(n) scan - same pattern
- The alternative (busy-polling between sources) is far worse

**Complexity by operation:**

| API | Complexity |
|-----|------------|
| `hive_ipc_recv()` | O(1) - first entry always matches ANY wildcards |
| `hive_ipc_recv_match()` | O(mailbox_depth) - scan for match |
| `hive_ipc_recv_matches()` | O(filters × mailbox_depth) |
| `hive_wait()` with 1 "any IPC" cond | O(1) - short-circuits on first match |
| `hive_wait()` with N conditions | O(N × sources_depth) |

Key insight: "any" wildcards (SENDER_ANY, MSG_ANY, TAG_ANY) short-circuit immediately. Only specific filters require scanning.

**The real efficiency win** - avoiding busy-polling:
```c
// WITHOUT unified wait - must poll or use awkward patterns:
while (1) {
    if (hive_ipc_recv(&msg, 0) == HIVE_OK) { handle_msg(); continue; }
    if (hive_bus_read(&bus, &data, 0) == HIVE_OK) { handle_bus(); continue; }
    hive_yield();  // Busy loop or arbitrary sleep - wasteful
}

// WITH unified wait - single efficient block:
hive_wait(conds, 3, &result, -1);  // Blocks in scheduler, zero CPU
```

## Relationship to Existing APIs

Existing APIs remain unchanged as convenience wrappers - no breaking changes:

```c
// Simple cases still use simple APIs (unchanged)
hive_ipc_recv(&msg, -1);                    // Wait for any message
hive_ipc_recv_match(from, class, tag, ...); // Single filter
hive_ipc_recv_matches(filters, n, ...);     // Multiple IPC filters
hive_bus_read_wait(bus, &data, -1);         // Single bus

// Complex cases use unified wait (new capability)
hive_wait(conds, n, &result, -1);           // IPC + bus + net combined
```

**Implementation as wrappers:**

```c
hive_status hive_ipc_recv(hive_message *msg, int32_t timeout_ms) {
    hive_wait_cond cond = {HIVE_WAIT_IPC, .ipc = {HIVE_SENDER_ANY, HIVE_MSG_ANY, HIVE_TAG_ANY}};
    hive_wait_result result;
    hive_status s = hive_wait(&cond, 1, &result, timeout_ms);
    if (HIVE_SUCCEEDED(s)) *msg = result.ipc;
    return s;
}

hive_status hive_bus_read_wait(bus_id bus, void **data, size_t *len, int32_t timeout_ms) {
    hive_wait_cond cond = {HIVE_WAIT_BUS, .bus = bus};
    hive_wait_result result;
    hive_status s = hive_wait(&cond, 1, &result, timeout_ms);
    if (HIVE_SUCCEEDED(s)) { *data = result.bus.data; *len = result.bus.len; }
    return s;
}
```

This follows the POSIX pattern: `read()`/`write()` are simple, `select()`/`poll()`/`epoll()` handle multiple sources. Most code uses the simple versions.

## Network I/O Integration

The `HIVE_WAIT_NET_RECV` and `HIVE_WAIT_NET_SEND` conditions enable waiting for IPC and network simultaneously:

```c
// Wait for command message OR incoming network data
hive_wait_cond conds[] = {
    {HIVE_WAIT_IPC, .ipc = {HIVE_SENDER_ANY, HIVE_MSG_NOTIFY, CMD_SHUTDOWN}},
    {HIVE_WAIT_NET_RECV, .fd = client_socket},
};
hive_wait(conds, 2, &result, -1);

if (result.type == HIVE_WAIT_IPC) {
    // Got shutdown command
} else {
    // Socket readable - now call hive_net_recv() to read data
}
```

Currently impossible - `hive_net_recv()` blocks on one socket, `hive_ipc_recv()` blocks on mailbox. No way to wait for both.

**Note:** The wait condition indicates the fd is ready. Actual data transfer still uses `hive_net_recv()`/`hive_net_send()` afterward.
