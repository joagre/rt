# Troubleshooting

**Pool exhausted**
Edit `rt_static_config.h`, increase pool size, rebuild. Set to ~1.5x peak usage.
- `RT_MAILBOX_ENTRY_POOL_SIZE` - mailbox
- `RT_MESSAGE_DATA_POOL_SIZE` - messages
- `RT_TIMER_ENTRY_POOL_SIZE` - timers
- `RT_LINK_ENTRY_POOL_SIZE` / `RT_MONITOR_ENTRY_POOL_SIZE` - links/monitors

**Hang/timeout**
- Cooperative violation: Add `rt_yield()` in long loops (see `benchmarks/bench.c`)
- Deadlock: Check for circular dependencies
- Stack alignment: Should be fixed, rebuild if using -O2

**Segfault**
- Stack overflow: Increase in `rt_spawn_ex()`: `cfg.stack_size = 256 * 1024;`
- Stack alignment: Already fixed in `src/rt_context.c`
- Invalid actor ID: Check `RT_FAILED(rt_ipc_send(...))`

**Slow performance**
Check `-O2` in Makefile, run `make bench`, ensure critical actors have priority 0, batch work to reduce context switches.

**Debug**
Logs in `src/rt_log.c`. Check stderr for errors. Verify pool sizes: `size build/librt.a` (expect ~231 KB BSS).

