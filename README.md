# Actor Runtime - Minimalistic Implementation

A minimalistic actor-based runtime for embedded systems with cooperative multitasking and message passing, inspired by Erlang's actor model.

This is a minimal x86-64 Linux implementation demonstrating the core concepts. See `spec.md` for the full specification including STM32/ARM Cortex-M support.

## Features Implemented

- ✅ Cooperative multitasking with manual x86-64 context switching
- ✅ Priority-based round-robin scheduler (4 priority levels)
- ✅ Actor lifecycle management (spawn, exit)
- ✅ IPC with COPY and BORROW modes
- ✅ Blocking and non-blocking message receive
- ⏳ Bus (pub-sub) - not yet implemented
- ⏳ Timers - not yet implemented
- ⏳ Network I/O - not yet implemented
- ⏳ File I/O - not yet implemented

## Building

```bash
make
```

## Running the Example

```bash
make run-pingpong
```

Or directly:

```bash
./build/pingpong
```

## Project Structure

```
.
├── include/          # Public headers
│   ├── rt_types.h       # Core types and error handling
│   ├── rt_context.h     # Context switching interface
│   ├── rt_actor.h       # Actor management
│   ├── rt_ipc.h         # Inter-process communication
│   ├── rt_scheduler.h   # Scheduler interface
│   └── rt_runtime.h     # Runtime initialization and public API
├── src/              # Implementation
│   ├── rt_actor.c       # Actor table and lifecycle
│   ├── rt_context.c     # Context initialization
│   ├── rt_context_asm.S # x86-64 context switch assembly
│   ├── rt_ipc.c         # Mailbox and message passing
│   ├── rt_runtime.c     # Runtime init and actor spawning
│   └── rt_scheduler.c   # Cooperative scheduler
├── examples/         # Example programs
│   └── pingpong.c       # Classic ping-pong actor example
├── build/            # Build artifacts (generated)
├── Makefile          # Build system
├── spec.md           # Full specification
├── CLAUDE.md         # Development guide for Claude Code
└── README.md         # This file
```

## Quick Start

### Basic Actor Example

```c
#include "rt_runtime.h"
#include "rt_ipc.h"
#include <stdio.h>

void my_actor(void *arg) {
    printf("Hello from actor %u\n", rt_self());
    rt_exit();
}

int main(void) {
    rt_config cfg = RT_CONFIG_DEFAULT;
    rt_init(&cfg);

    rt_spawn(my_actor, NULL);

    rt_run();
    rt_cleanup();

    return 0;
}
```

### Sending Messages

```c
// Send a COPY message (async)
int data = 42;
rt_ipc_send(target_actor, &data, sizeof(data), IPC_COPY);

// Send a BORROW message (zero-copy, blocks until consumed)
int data = 42;
rt_ipc_send(target_actor, &data, sizeof(data), IPC_BORROW);
```

### Receiving Messages

```c
// Blocking receive
rt_message msg;
rt_ipc_recv(&msg, -1);  // Block until message arrives
printf("Received %zu bytes from actor %u\n", msg.len, msg.sender);

// Non-blocking receive
if (rt_ipc_recv(&msg, 0) == RT_OK) {
    // Process message
}
```

## API Overview

### Runtime Initialization

- `rt_init(config)` - Initialize the runtime
- `rt_run()` - Run the scheduler (blocks until all actors exit)
- `rt_cleanup()` - Cleanup and free resources
- `rt_shutdown()` - Request graceful shutdown

### Actor Management

- `rt_spawn(fn, arg)` - Spawn actor with default config
- `rt_spawn_ex(fn, arg, config)` - Spawn actor with custom config
- `rt_exit()` - Terminate current actor
- `rt_self()` - Get current actor's ID
- `rt_yield()` - Voluntarily yield to scheduler

### IPC

- `rt_ipc_send(to, data, len, mode)` - Send message
- `rt_ipc_recv(msg, timeout)` - Receive message
- `rt_ipc_release(msg)` - Release borrowed message
- `rt_ipc_pending()` - Check if messages are available
- `rt_ipc_count()` - Get number of pending messages

## Design Principles

1. **Minimalistic**: Only essential features, no bloat
2. **Predictable**: Cooperative scheduling, no surprises
3. **Modern C11**: Clean, safe, standards-compliant code
4. **No heap in hot paths**: Fixed-size stacks, pre-allocated tables
5. **Explicit control**: Actors yield explicitly, no preemption

## Implementation Notes

### Context Switching

The x86-64 context switch saves and restores callee-saved registers (rbx, rbp, r12-r15) and the stack pointer. This is implemented in hand-written assembly for performance and control.

### Scheduling

The scheduler uses a simple priority-based round-robin algorithm:
1. Find the highest-priority READY actor
2. Context switch to it
3. When it yields, mark it READY again
4. Repeat

### IPC Modes

- **IPC_COPY**: Message data is copied to receiver's mailbox. Sender continues immediately.
- **IPC_BORROW**: Message data remains on sender's stack. Sender blocks until receiver calls `rt_ipc_release()`. Provides automatic backpressure.

## Limitations of This Minimal Version

- No timers (would require timer thread)
- No network/file I/O (would require I/O threads)
- No bus/pub-sub system
- No actor linking or monitoring
- Simple memory management (uses malloc/free)
- No stack overflow detection
- Single-threaded (no FreeRTOS integration)

See `spec.md` for the full feature set planned for production use.

## Future Work

- Implement bus (pub-sub) system
- Add timer support
- Add network and file I/O with async completion queues
- Port to ARM Cortex-M with FreeRTOS integration
- Add memory pools for better performance
- Implement actor linking and monitoring
- Add stack overflow detection

## License

See project license file.
