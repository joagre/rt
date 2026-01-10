/**
 * Stack Guard Detection Test
 *
 * This test verifies that the runtime detects stack guard corruption
 * and properly notifies linked actors.
 *
 * Rather than causing an actual stack overflow (which corrupts memory
 * unpredictably), this test directly corrupts the stack guard pattern
 * to verify the detection mechanism works.
 */

#include "hive_runtime.h"
#include "hive_ipc.h"
#include "hive_link.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* TEST_STACK_SIZE caps stack for QEMU builds; passes through on native */
#ifndef TEST_STACK_SIZE
#define TEST_STACK_SIZE(x) (x)
#endif

/* Stack guard pattern and size (must match hive_actor.c) */
#define STACK_GUARD_PATTERN 0xDEADBEEFCAFEBABEULL
#define STACK_GUARD_SIZE 8

/* Test-only function declared in hive_runtime.c */
extern void *hive_test_get_stack_base(void);

static void overflow_actor(void *arg) {
    (void)arg;

    printf("Overflow actor: Getting stack base...\n");
    void *stack_base = hive_test_get_stack_base();

    if (!stack_base) {
        printf("Overflow actor: ERROR - could not get stack base\n");
        hive_exit();
        return;
    }

    /* The low guard is at the very start of the stack allocation */
    volatile uint64_t *guard_low = (volatile uint64_t *)stack_base;

    printf("Overflow actor: Stack base at %p, guard value = 0x%llx\n",
           stack_base, (unsigned long long)*guard_low);

    if (*guard_low != STACK_GUARD_PATTERN) {
        printf("Overflow actor: WARNING - guard pattern doesn't match expected\n");
    }

    printf("Overflow actor: Corrupting low guard...\n");
    *guard_low = 0x0;  /* Corrupt the guard */

    printf("Overflow actor: Yielding to trigger guard check...\n");
    hive_yield();

    /* Should not reach here if guard corruption was detected */
    printf("Overflow actor: ERROR - guard corruption not detected!\n");
    hive_exit();
}

static void linked_actor(void *arg) {
    actor_id overflow_id = *(actor_id *)arg;

    printf("Linked actor: Linking to overflow actor...\n");
    hive_link(overflow_id);

    printf("Linked actor: Waiting for exit notification...\n");
    hive_message msg;
    hive_status status = hive_ipc_recv(&msg, 5000);

    if (HIVE_FAILED(status)) {
        printf("Linked actor: FAIL - No notification received (timeout)\n");
        hive_exit();
    }

    if (hive_is_exit_msg(&msg)) {
        hive_exit_msg exit_info;
        hive_decode_exit(&msg, &exit_info);

        const char *reason_str =
            exit_info.reason == HIVE_EXIT_CRASH_STACK ? "STACK_OVERFLOW" :
            exit_info.reason == HIVE_EXIT_CRASH ? "CRASH" :
            exit_info.reason == HIVE_EXIT_NORMAL ? "NORMAL" : "UNKNOWN";

        printf("Linked actor: Received exit, reason=%s\n", reason_str);

        if (exit_info.reason == HIVE_EXIT_CRASH_STACK) {
            printf("Linked actor: PASS - Stack guard corruption detected\n");
        } else {
            printf("Linked actor: FAIL - Expected STACK_OVERFLOW, got %s\n",
                   reason_str);
        }
    } else {
        printf("Linked actor: FAIL - Not an exit notification\n");
    }

    hive_exit();
}

static void witness_actor(void *arg) {
    (void)arg;
    hive_message msg;
    hive_ipc_recv(&msg, 200);  /* Small delay */

    printf("Witness: PASS - Runtime still functional\n");
    hive_exit();
}

int main(void) {
    printf("=== Stack Guard Detection Test ===\n");
    printf("Tests that guard corruption is detected on yield\n\n");

    hive_init();

    actor_config cfg = HIVE_ACTOR_CONFIG_DEFAULT;
#ifdef QEMU_TEST_STACK_SIZE
    cfg.stack_size = 2048;
#else
    cfg.stack_size = 8192;
#endif

    actor_id overflow;
    hive_spawn_ex(overflow_actor, NULL, &cfg, &overflow);
    printf("Main: Spawned overflow actor (stack=%zu)\n", cfg.stack_size);

    actor_id linked;
    hive_spawn(linked_actor, &overflow, &linked);

    actor_id witness;
    cfg.stack_size = TEST_STACK_SIZE(16 * 1024);
    hive_spawn_ex(witness_actor, NULL, &cfg, &witness);

    hive_run();
    hive_cleanup();

    printf("\n=== Test Complete ===\n");
    return 0;
}
