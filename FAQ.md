# Frequently Asked Questions (FAQ)

Common questions about the actor runtime for embedded systems.

📖 See also: [README.md](README.md) | [Full Specification](spec.md)

---

### General Questions

**Q: Can I use this in production?**
A: The x86-64 Linux version is feature-complete and tested. For embedded/MCU deployment, wait for the ARM Cortex-M port (see [Future Work](#future-work)).

**Q: Is this thread-safe?**
A: The actor runtime itself is single-threaded (cooperative). I/O subsystems (file, network, timer) use separate worker threads but communicate via lock-free queues. Actors themselves should not use threads.

**Q: How does this compare to FreeRTOS?**
A: FreeRTOS provides preemptive multitasking with tasks and queues. This runtime provides cooperative multitasking with actors and message passing. FreeRTOS is lower-level; this runtime provides higher-level abstractions (linking, monitoring, pub-sub). You can run this runtime ON TOP of FreeRTOS (planned).

**Q: Can actors share memory?**
A: Actors **should not** share memory. While technically possible to pass pointers to heap-allocated memory between actors, this is **strongly discouraged** as it:
- Violates the actor model (introduces race conditions and shared-state bugs)
- Creates unclear ownership (who frees the memory?)
- Requires manual synchronization (mutexes), defeating the purpose of actors

**Instead, use:**
- `IPC_COPY` - Data is safely copied to receiver's mailbox (no sharing)
- `IPC_BORROW` - Zero-copy with proper ownership semantics (sender blocks until receiver releases)
- `Bus` - Pub-sub with controlled data retention policies

**Q: What happens when a pool is exhausted?**
A: Operations return `RT_ERR_NOMEM`. For critical operations (like exit notifications), messages may be dropped with error logging. Increase pool sizes in `rt_static_config.h`.

### Performance Questions

**Q: Why cooperative instead of preemptive?**
A: Cooperative multitasking is simpler, more predictable, and avoids preemption-related bugs. It's perfect for embedded systems where actors cooperate (sensors, controllers, etc.). Critical actors get CRITICAL priority (0) and run first.

**Q: What's the context switch overhead?**
A: ~1.1 µs per switch on x86-64 (see [Performance](#performance)). This allows ~900K switches/sec, more than enough for typical embedded workloads.

**Q: Should I use COPY or BORROW mode?**
A:
- **COPY** - For small messages (<256 bytes), fire-and-forget, sender doesn't care when receiver processes
- **BORROW** - For large messages, when you need backpressure (sender blocks until receiver processes), zero-copy efficiency

**Q: How many actors can I have?**
A: Configured at compile time with `RT_MAX_ACTORS` (default: 64). Each actor needs a stack (~64KB default). Total actors limited by available memory.

### Configuration Questions

**Q: How do I change pool sizes?**
A: Edit `include/rt_static_config.h`, modify the relevant `#define`, and rebuild with `make clean && make all`. See [Memory Configuration](#memory-configuration).

**Q: How do I set actor priority?**
A: Use `rt_spawn_ex()` with `actor_config`:
```c
actor_config cfg = RT_ACTOR_CONFIG_DEFAULT;
cfg.priority = 0;  // CRITICAL (0-3, lower is higher priority)
rt_spawn_ex(my_actor, arg, &cfg);
```

**Q: How much memory does this use?**
A: Default config: ~231 KB static (BSS) + N actors × stack size. For 20 actors with 64KB stacks: ~1.5 MB total. See [Memory Configuration](#memory-configuration) for calculation details.

**Q: Can I run this on Arduino/ESP32/STM32?**
A: Not yet. Current version targets x86-64 Linux. ARM Cortex-M port (STM32) is planned. ESP32 (Xtensa) not currently planned.

### Debugging Questions

**Q: Why is my actor not running?**
A: Possible causes:
1. Lower priority than another busy actor (check priority levels)
2. Actor is WAITING (blocked on `rt_ipc_recv()`, `rt_file_read()`, etc.)
3. No messages in mailbox (if blocking on receive)

**Q: How do I debug deadlocks?**
A: Add debug printf at actor boundaries:
```c
printf("[Actor %u] Waiting for message...\n", rt_self());
rt_ipc_recv(&msg, -1);
printf("[Actor %u] Received message\n", rt_self());
```
Check for circular dependencies (A waits for B, B waits for A).

**Q: Can I use GDB with this?**
A: Yes! Compile with `-g` (already in Makefile), run with GDB:
```bash
gdb ./build/your_example
(gdb) break my_actor
(gdb) run
(gdb) bt  # Show actor call stack
```

**Q: Why can't I use printf in actors?**
A: You CAN use printf! It's cooperative, so printf won't block the runtime. Just be aware that excessive printf can slow down your application.

