/**
 * QEMU Test for Hive Runtime (STM32/Cortex-M target)
 *
 * Tests:
 *   1. Runtime initialization
 *   2. Actor spawn and context switching
 *   3. IPC message passing (without timers)
 *
 * Run with: make qemu-test
 */

#include "semihosting.h"
#include "hive_runtime.h"
#include "hive_actor.h"
#include "hive_ipc.h"
#include "hive_scheduler.h"
#include <stdint.h>
#include <stdbool.h>

/* Test counters */
static volatile int g_test_passed = 0;
static volatile int g_test_failed = 0;
static volatile int g_pong_count = 0;
static volatile int g_context_switches = 0;
static actor_id g_pong_actor = 0;

/* Test macros */
#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        semihosting_printf("[PASS] %s\n", msg); \
        g_test_passed++; \
    } else { \
        semihosting_printf("[FAIL] %s\n", msg); \
        g_test_failed++; \
    } \
} while(0)

/* Pong actor - responds to ping messages, no timeout */
static void pong_actor(void *arg) {
    (void)arg;
    semihosting_printf("Pong actor started (id=%u)\n", (unsigned)hive_self());
    g_context_switches++;

    /* Receive 3 pings and reply to each */
    for (int i = 0; i < 3; i++) {
        hive_message msg;
        hive_status s = hive_ipc_recv(&msg, 0);  /* Non-blocking - message should already be there */

        if (HIVE_FAILED(s)) {
            /* Yield and try again */
            hive_yield();
            g_context_switches++;
            i--;  /* Retry this iteration */
            continue;
        }

        semihosting_printf("Pong: received ping #%d, replying\n", i + 1);

        /* Reply with pong */
        const char *pong = "PONG";
        hive_ipc_reply(&msg, pong, 5);
        g_pong_count++;
    }

    semihosting_printf("Pong actor exiting\n");
}

/* Ping actor - sends pings immediately (no sleep) */
static void ping_actor(void *arg) {
    (void)arg;
    semihosting_printf("Ping actor started (id=%u)\n", (unsigned)hive_self());
    g_context_switches++;

    /* Send 3 pings */
    for (int i = 0; i < 3; i++) {
        const char *ping = "PING";
        hive_message reply;

        semihosting_printf("Ping: sending request #%d to actor %u\n", i + 1, (unsigned)g_pong_actor);

        /* Use notify + recv instead of request to avoid timeout dependency */
        hive_ipc_notify(g_pong_actor, ping, 5);

        /* Yield to let pong process */
        hive_yield();
        g_context_switches++;
    }

    semihosting_printf("Ping actor exiting\n");
}

/* Simple yield test actor */
static void yield_actor(void *arg) {
    int id = (int)(intptr_t)arg;
    semihosting_printf("Yield actor %d started\n", id);
    g_context_switches++;

    for (int i = 0; i < 3; i++) {
        semihosting_printf("Yield actor %d: iteration %d\n", id, i);
        hive_yield();
        g_context_switches++;
    }

    semihosting_printf("Yield actor %d exiting\n", id);
}

/* Main test entry point */
int main(void) {
    semihosting_printf("\n");
    semihosting_printf("=== Hive Runtime QEMU Test ===\n");
    semihosting_printf("Testing: init, spawn, context switch, IPC\n");
    semihosting_printf("\n");

    /* Test 1: Runtime initialization */
    hive_status s = hive_init();
    TEST_ASSERT(!HIVE_FAILED(s), "Runtime initialization");

    if (HIVE_FAILED(s)) {
        semihosting_printf("FATAL: Failed to initialize runtime\n");
        semihosting_exit(1);
    }

    /* Test 2: Spawn yield test actors */
    semihosting_printf("\n--- Test: Context Switching ---\n");
    actor_id yield1, yield2;

    s = hive_spawn(yield_actor, (void *)1, &yield1);
    TEST_ASSERT(!HIVE_FAILED(s), "Spawn yield actor 1");

    s = hive_spawn(yield_actor, (void *)2, &yield2);
    TEST_ASSERT(!HIVE_FAILED(s), "Spawn yield actor 2");

    /* Run scheduler steps until actors complete */
    for (int i = 0; i < 20; i++) {
        hive_status step_s = hive_scheduler_step();
        if (HIVE_FAILED(step_s)) {
            break;  /* No more ready actors */
        }
    }

    TEST_ASSERT(g_context_switches >= 6, "Context switches occurred");
    semihosting_printf("Context switches: %d\n", g_context_switches);

    /* Test 3: IPC test */
    semihosting_printf("\n--- Test: IPC Message Passing ---\n");
    g_context_switches = 0;

    actor_id pong_id, ping_id;

    s = hive_spawn(pong_actor, NULL, &pong_id);
    TEST_ASSERT(!HIVE_FAILED(s), "Spawn pong actor");
    g_pong_actor = pong_id;

    s = hive_spawn(ping_actor, NULL, &ping_id);
    TEST_ASSERT(!HIVE_FAILED(s), "Spawn ping actor");

    semihosting_printf("Spawned: pong=%u, ping=%u\n", (unsigned)pong_id, (unsigned)ping_id);

    /* Run scheduler */
    for (int i = 0; i < 50; i++) {
        hive_status step_s = hive_scheduler_step();
        if (HIVE_FAILED(step_s)) {
            break;
        }
    }

    TEST_ASSERT(g_pong_count >= 1, "IPC messages exchanged");
    semihosting_printf("Pong replies: %d\n", g_pong_count);

    /* Cleanup */
    hive_cleanup();

    /* Summary */
    semihosting_printf("\n");
    semihosting_printf("=== Test Summary ===\n");
    semihosting_printf("Passed: %d\n", g_test_passed);
    semihosting_printf("Failed: %d\n", g_test_failed);
    semihosting_printf("\n");

    if (g_test_failed > 0) {
        semihosting_printf("TESTS FAILED\n");
        semihosting_exit(1);
    } else {
        semihosting_printf("ALL TESTS PASSED\n");
        semihosting_exit(0);
    }

    return 0;  /* Never reached */
}
