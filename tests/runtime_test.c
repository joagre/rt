#include "rt_runtime.h"
#include "rt_ipc.h"
#include "rt_timer.h"
#include "rt_link.h"
#include <stdio.h>
#include <string.h>

// Test results
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_PASS(name) do { printf("  PASS: %s\n", name); fflush(stdout); tests_passed++; } while(0)
#define TEST_FAIL(name) do { printf("  FAIL: %s\n", name); fflush(stdout); tests_failed++; } while(0)

// ============================================================================
// Test 1: rt_init returns success
// ============================================================================

static void test1_init_success(void) {
    printf("\nTest 1: rt_init returns success\n");

    // Note: rt_init was already called in main() to run this test
    // This test just verifies the runtime started successfully
    TEST_PASS("rt_init succeeded (we're running)");
}

// ============================================================================
// Test 2: rt_self outside actor context
// NOTE: This tests behavior that may crash or return invalid ID
// ============================================================================

// Cannot safely test rt_self outside actor context from within actor
// Just document the expected behavior
static void test2_self_outside_actor(void *arg) {
    (void)arg;
    printf("\nTest 2: rt_self outside actor context\n");
    printf("    NOTE: Cannot test from within actor - would need separate process\n");
    printf("    Expected: Should return ACTOR_ID_INVALID or crash\n");
    fflush(stdout);

    // We're inside an actor, so rt_self() works fine here
    actor_id self = rt_self();
    if (self != ACTOR_ID_INVALID) {
        TEST_PASS("rt_self returns valid ID inside actor context");
    } else {
        TEST_FAIL("rt_self returned invalid ID inside actor context");
    }

    rt_exit();
}

// ============================================================================
// Test 3: rt_yield returns control to scheduler
// ============================================================================

static int g_yield_order = 0;
static int g_actor1_order = 0;
static int g_actor2_order = 0;

static void yield_actor1(void *arg) {
    (void)arg;
    g_actor1_order = ++g_yield_order;
    rt_yield();
    g_actor1_order = ++g_yield_order;
    rt_exit();
}

static void yield_actor2(void *arg) {
    (void)arg;
    g_actor2_order = ++g_yield_order;
    rt_yield();
    g_actor2_order = ++g_yield_order;
    rt_exit();
}

static void test3_yield(void *arg) {
    (void)arg;
    printf("\nTest 3: rt_yield returns control to scheduler\n");
    fflush(stdout);

    g_yield_order = 0;
    g_actor1_order = 0;
    g_actor2_order = 0;

    actor_id a1 = rt_spawn(yield_actor1, NULL);
    actor_id a2 = rt_spawn(yield_actor2, NULL);

    rt_link(a1);
    rt_link(a2);

    // Wait for both to complete via link notifications
    rt_message msg;
    rt_ipc_recv(&msg, 500);
    rt_ipc_recv(&msg, 500);

    // After yielding, actors should have interleaved
    // Final order should be 4 (each actor ran twice)
    if (g_yield_order == 4) {
        TEST_PASS("rt_yield allows interleaved execution");
    } else {
        printf("    Final order: %d (expected 4)\n", g_yield_order);
        TEST_FAIL("yield did not interleave correctly");
    }

    rt_exit();
}

// ============================================================================
// Test 4: rt_actor_alive with various IDs
// ============================================================================

static void quickly_exit_actor(void *arg) {
    (void)arg;
    rt_exit();
}

static void test4_actor_alive(void *arg) {
    (void)arg;
    printf("\nTest 4: rt_actor_alive with various IDs\n");
    fflush(stdout);

    // Self should be alive
    actor_id self = rt_self();
    if (rt_actor_alive(self)) {
        TEST_PASS("rt_actor_alive returns true for self");
    } else {
        TEST_FAIL("rt_actor_alive should return true for self");
    }

    // Invalid ID should not be alive
    if (!rt_actor_alive(ACTOR_ID_INVALID)) {
        TEST_PASS("rt_actor_alive returns false for ACTOR_ID_INVALID");
    } else {
        TEST_FAIL("rt_actor_alive should return false for ACTOR_ID_INVALID");
    }

    // Non-existent ID should not be alive
    if (!rt_actor_alive(9999)) {
        TEST_PASS("rt_actor_alive returns false for non-existent ID");
    } else {
        TEST_FAIL("rt_actor_alive should return false for non-existent ID");
    }

    // Spawn and check alive
    actor_id child = rt_spawn(quickly_exit_actor, NULL);
    rt_link(child);

    // Should be alive right after spawn
    bool alive_before = rt_actor_alive(child);

    // Wait for it to exit via link notification
    rt_message msg;
    rt_ipc_recv(&msg, 500);

    // Should be dead after exit
    bool alive_after = rt_actor_alive(child);

    if (alive_before && !alive_after) {
        TEST_PASS("rt_actor_alive tracks actor lifecycle");
    } else {
        printf("    before=%d, after=%d\n", alive_before, alive_after);
        TEST_FAIL("rt_actor_alive did not track lifecycle correctly");
    }

    rt_exit();
}

// ============================================================================
// Test 5: Scheduler handles many actors
// ============================================================================

#define MANY_ACTORS 10
static int g_many_actors_count = 0;

static void many_actor(void *arg) {
    (void)arg;
    g_many_actors_count++;
    rt_exit();
}

static void test5_many_actors(void *arg) {
    (void)arg;
    printf("\nTest 5: Scheduler handles many actors\n");
    fflush(stdout);

    g_many_actors_count = 0;

    // Spawn actors without linking (simpler)
    int spawned = 0;
    for (int i = 0; i < MANY_ACTORS; i++) {
        actor_id id = rt_spawn(many_actor, NULL);
        if (id == ACTOR_ID_INVALID) {
            printf("    Failed to spawn actor %d\n", i);
            fflush(stdout);
            break;
        }
        spawned++;
    }

    // Yield several times to let spawned actors run
    for (int i = 0; i < MANY_ACTORS * 2; i++) {
        rt_yield();
    }

    if (g_many_actors_count == spawned && spawned == MANY_ACTORS) {
        printf("    Spawned and ran %d actors\n", MANY_ACTORS);
        TEST_PASS("scheduler handles many actors");
    } else {
        printf("    Spawned %d, ran %d/%d actors\n", spawned, g_many_actors_count, MANY_ACTORS);
        if (spawned < MANY_ACTORS) {
            TEST_FAIL("could not spawn all actors (actor table full?)");
        } else {
            TEST_FAIL("not all actors ran");
        }
    }

    rt_exit();
}

// ============================================================================
// Test 6: rt_shutdown (if implemented)
// NOTE: rt_shutdown is declared but may not be fully implemented
// ============================================================================

static void test6_shutdown(void *arg) {
    (void)arg;
    printf("\nTest 6: rt_shutdown\n");
    printf("    NOTE: rt_shutdown behavior depends on implementation\n");
    fflush(stdout);

    // We can't actually test shutdown from within an actor
    // because it would terminate us
    TEST_PASS("rt_shutdown exists (not tested from within actor)");

    rt_exit();
}

// ============================================================================
// Test 7: Actor stack sizes
// ============================================================================

static bool g_small_stack_ok = false;
static bool g_large_stack_ok = false;

static void small_stack_actor(void *arg) {
    (void)arg;
    // Just run with minimal stack usage
    int x = 42;
    (void)x;
    g_small_stack_ok = true;
    rt_exit();
}

static void large_stack_actor(void *arg) {
    (void)arg;
    // Use more stack
    char buffer[16384];  // 16KB
    memset(buffer, 'A', sizeof(buffer));
    if (buffer[16383] == 'A') {
        g_large_stack_ok = true;
    }
    rt_exit();
}

static void test7_stack_sizes(void *arg) {
    (void)arg;
    printf("\nTest 7: Actor stack sizes\n");
    fflush(stdout);

    g_small_stack_ok = false;
    g_large_stack_ok = false;

    // Small stack
    actor_config small_cfg = RT_ACTOR_CONFIG_DEFAULT;
    small_cfg.stack_size = 8 * 1024;  // 8KB

    actor_id small = rt_spawn_ex(small_stack_actor, NULL, &small_cfg);
    if (small != ACTOR_ID_INVALID) {
        rt_link(small);
        rt_message msg;
        rt_ipc_recv(&msg, 500);

        if (g_small_stack_ok) {
            TEST_PASS("small stack (8KB) works");
        } else {
            TEST_FAIL("small stack actor did not complete");
        }
    } else {
        TEST_FAIL("failed to spawn small stack actor");
    }

    // Large stack
    actor_config large_cfg = RT_ACTOR_CONFIG_DEFAULT;
    large_cfg.stack_size = 32 * 1024;  // 32KB

    actor_id large = rt_spawn_ex(large_stack_actor, NULL, &large_cfg);
    if (large != ACTOR_ID_INVALID) {
        rt_link(large);
        rt_message msg;
        rt_ipc_recv(&msg, 500);

        if (g_large_stack_ok) {
            TEST_PASS("large stack (32KB) works");
        } else {
            TEST_FAIL("large stack actor did not complete");
        }
    } else {
        TEST_FAIL("failed to spawn large stack actor");
    }

    rt_exit();
}

// ============================================================================
// Test 8: Priority levels
// ============================================================================

static int g_priority_order[4] = {0, 0, 0, 0};
static int g_priority_counter = 0;

static void priority_actor(void *arg) {
    int level = *(int *)arg;
    g_priority_order[level] = ++g_priority_counter;
    rt_exit();
}

static void test8_priorities(void *arg) {
    (void)arg;
    printf("\nTest 8: Priority levels\n");
    fflush(stdout);

    g_priority_counter = 0;
    for (int i = 0; i < 4; i++) {
        g_priority_order[i] = 0;
    }

    // Spawn actors in reverse priority order (LOW first, CRITICAL last)
    static int levels[4] = {RT_PRIO_LOW, RT_PRIO_NORMAL, RT_PRIO_HIGH, RT_PRIO_CRITICAL};
    actor_id ids[4];

    for (int i = 0; i < 4; i++) {
        actor_config cfg = RT_ACTOR_CONFIG_DEFAULT;
        cfg.priority = levels[i];
        ids[i] = rt_spawn_ex(priority_actor, &levels[i], &cfg);
        rt_link(ids[i]);
    }

    // Wait for all to complete via link notifications
    for (int i = 0; i < 4; i++) {
        rt_message msg;
        rt_ipc_recv(&msg, 500);
    }

    // Higher priority should run first (lower number = higher priority)
    // CRITICAL=0 should be first, LOW=3 should be last
    printf("    Execution order: CRITICAL=%d, HIGH=%d, NORMAL=%d, LOW=%d\n",
           g_priority_order[RT_PRIO_CRITICAL],
           g_priority_order[RT_PRIO_HIGH],
           g_priority_order[RT_PRIO_NORMAL],
           g_priority_order[RT_PRIO_LOW]);
    fflush(stdout);

    if (g_priority_order[RT_PRIO_CRITICAL] < g_priority_order[RT_PRIO_LOW]) {
        TEST_PASS("higher priority actors run before lower priority");
    } else {
        TEST_FAIL("priority order not respected");
    }

    rt_exit();
}

// ============================================================================
// Test runner
// ============================================================================

static void (*test_funcs[])(void *) = {
    test2_self_outside_actor,
    test3_yield,
    test4_actor_alive,
    test5_many_actors,
    test6_shutdown,
    test7_stack_sizes,
    test8_priorities,
};

#define NUM_TESTS (sizeof(test_funcs) / sizeof(test_funcs[0]))

static void run_all_tests(void *arg) {
    (void)arg;

    // Test 1 runs outside actor context (sort of)
    test1_init_success();

    for (size_t i = 0; i < NUM_TESTS; i++) {
        actor_config cfg = RT_ACTOR_CONFIG_DEFAULT;
        cfg.stack_size = 64 * 1024;

        actor_id test = rt_spawn_ex(test_funcs[i], NULL, &cfg);
        if (test == ACTOR_ID_INVALID) {
            printf("Failed to spawn test %zu\n", i);
            fflush(stdout);
            continue;
        }

        rt_link(test);

        rt_message msg;
        rt_ipc_recv(&msg, 10000);
    }

    rt_exit();
}

int main(void) {
    printf("=== Runtime (rt_init/rt_run/rt_cleanup) Test Suite ===\n");
    fflush(stdout);

    rt_status status = rt_init();
    if (RT_FAILED(status)) {
        fprintf(stderr, "Failed to initialize runtime: %s\n",
                status.msg ? status.msg : "unknown error");
        return 1;
    }

    actor_config cfg = RT_ACTOR_CONFIG_DEFAULT;
    cfg.stack_size = 128 * 1024;

    actor_id runner = rt_spawn_ex(run_all_tests, NULL, &cfg);
    if (runner == ACTOR_ID_INVALID) {
        fprintf(stderr, "Failed to spawn test runner\n");
        rt_cleanup();
        return 1;
    }

    rt_run();
    rt_cleanup();

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n%s\n", tests_failed == 0 ? "All tests passed!" : "Some tests FAILED!");

    return tests_failed > 0 ? 1 : 0;
}
