# FAQ

**Q: Production ready?**
A: x86-64 Linux version is feature-complete and tested. ARM Cortex-M port is planned.

**Q: Thread-safe?**
A: Actor runtime is single-threaded (cooperative). I/O subsystems use worker threads with lock-free queues. Actors should not use threads.

**Q: vs FreeRTOS?**
A: FreeRTOS: preemptive tasks/queues. This: cooperative actors/message-passing with higher-level abstractions (linking, monitoring, pub-sub). Runs on top of FreeRTOS.

**Q: Can actors share memory?**
A: Should not. Violates actor model, creates race conditions, unclear ownership. Use IPC_ASYNC (fire-and-forget), IPC_SYNC (blocking with backpressure), or Bus (pub-sub).

**Q: Pool exhausted?**
A: Returns `RT_ERR_NOMEM`. Critical operations may drop messages with logging. Increase pool sizes in `rt_static_config.h`.

**Q: Cooperative vs preemptive?**
A: Simpler, more predictable, no preemption bugs. Critical actors get priority 0 and run first.

**Q: Context switch overhead?**
A: ~1.1 µs/switch on x86-64.

**Q: ASYNC or SYNC?**
A: ASYNC for small messages (<256 bytes), fire-and-forget. SYNC for flow control, backpressure, trusted cooperating actors.

**Q: Max actors?**
A: `RT_MAX_ACTORS` in `rt_static_config.h` (default: 64). Each actor needs stack (~64KB default).

**Q: Change pool sizes?**
A: Edit `rt_static_config.h`, rebuild with `make clean && make all`.

**Q: Set priority?**
A: `rt_spawn_ex()` with `actor_config`: `cfg.priority = 0;` (0-3, lower is higher).

**Q: Memory usage?**
A: ~231 KB static + N actors × stack size. Example: 20 actors × 64KB = ~1.5 MB total.

**Q: Platforms?**
A: x86-64 Linux. ARM Cortex-M (STM32) planned. ESP32 not planned.

**Q: Actor not running?**
A: Lower priority than busy actor, WAITING (blocked on recv/read), or no messages.

**Q: Debug deadlocks?**
A: Add printf at boundaries, check for circular dependencies.

**Q: Use GDB?**
A: Yes. Compile with `-g`, run `gdb ./build/example`.

