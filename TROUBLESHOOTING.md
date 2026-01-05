# Troubleshooting Guide

Common issues and solutions for the actor runtime.

📖 See also: [README.md](README.md) | [FAQ](FAQ.md) | [Full Specification](spec.md)

---

### Pool Exhaustion Errors

**Symptom:** Error messages like "Mailbox pool exhausted" or "Message pool exhausted"

**Solution:**
1. Edit `include/rt_static_config.h`
2. Increase the relevant pool size:
   - `RT_MAILBOX_ENTRY_POOL_SIZE` for mailbox exhaustion
   - `RT_MESSAGE_DATA_POOL_SIZE` for message data exhaustion
   - `RT_TIMER_ENTRY_POOL_SIZE` for timer exhaustion
   - `RT_LINK_ENTRY_POOL_SIZE` / `RT_MONITOR_ENTRY_POOL_SIZE` for link/monitor exhaustion
3. Rebuild: `make clean && make all`

**Guideline:** Set pool size to ~1.5x your expected peak usage.

### Examples Hang or Timeout

**Symptom:** Example programs hang indefinitely or timeout

**Possible causes:**
1. **Cooperative scheduling violation** - Actor running without yielding
   - **Solution:** Add `rt_yield()` calls in long-running loops
   - **Example:** See `benchmarks/bench.c` bus benchmark (yields every 10 iterations)

2. **Deadlock** - Actors waiting for each other
   - **Solution:** Check message flow, ensure no circular dependencies

3. **Stack alignment bug** - Crashes with -O2 optimization (should be fixed)
   - **Solution:** Verify `src/rt_context.c` has proper alignment padding

### Segmentation Faults

**Symptom:** Crashes with segmentation fault

**Possible causes:**
1. **Stack overflow** - Actor stack too small
   - **Solution:** Increase stack size in `rt_spawn_ex()`:
     ```c
     actor_config cfg = RT_ACTOR_CONFIG_DEFAULT;
     cfg.stack_size = 256 * 1024;  // 256 KB instead of default 64 KB
     ```

2. **Stack alignment issue** - Only with optimized builds (-O2)
   - **Solution:** Already fixed in `src/rt_context.c`, rebuild with `make clean && make`

3. **Invalid actor ID** - Sending to dead/invalid actor
   - **Solution:** Check `RT_FAILED(rt_ipc_send(...))` and handle errors

### Build Errors

**Symptom:** Compilation or linking failures

**Common issues:**
1. **Missing prerequisites** - Check [Prerequisites](#prerequisites) section
   ```bash
   gcc --version  # Need GCC 4.7+
   uname -m       # Need x86_64
   ```

2. **Wrong architecture** - Building on non-x86-64 system
   - **Solution:** This version only supports x86-64 Linux

3. **Missing pthread** - Linker errors about pthread
   - **Solution:** Ensure pthread is available: `gcc -pthread test.c`

### Performance Issues

**Symptom:** Slower than expected performance

**Debugging:**
1. **Check optimization level** - Ensure using `-O2` in Makefile
2. **Profile with benchmarks** - Run `make bench` to establish baseline
3. **Check priority levels** - Ensure critical actors have priority 0
4. **Reduce context switches** - Batch work in actors to reduce switching overhead

### Debugging Tips

**Enable debug logging:**
- Logs are in `src/rt_log.c` (INFO, WARNING, ERROR levels)
- Check stderr for scheduler and subsystem messages

**Verify pool sizes:**
```bash
# Check static data size
size build/librt.a

# Expected BSS segment: ~231 KB with default config
```

**Common debugging workflow:**
1. Run example: `./build/example_name`
2. Check error messages in stderr
3. If pool exhausted → increase pool size in `rt_static_config.h`
4. If hanging → add debug printf in actor, check for infinite loops
5. If crashing → increase stack size or check for invalid pointers

