/*
 * IPC_SYNC Example - Synchronous Message Passing with Backpressure
 *
 * This example demonstrates IPC_SYNC mode, which provides flow control
 * by blocking the sender until the receiver explicitly releases the message.
 *
 * KEY CONCEPTS:
 * - Sender blocks until receiver calls rt_ipc_release()
 * - Provides natural backpressure (fast sender waits for slow receiver)
 * - Message data is copied to a pinned runtime buffer (safe if sender dies)
 * - Receiver MUST call rt_ipc_release() to unblock sender
 *
 * DEADLOCK WARNING:
 * - NEVER do circular sync sends: A sends SYNC to B, B sends SYNC to A
 * - NEVER send SYNC to self: rt_ipc_send(rt_self(), ..., IPC_SYNC)
 * - NEVER nest sync sends without releasing first
 *
 * USE CASES:
 * - Flow control between fast producer and slow consumer
 * - Request-response patterns where sender needs confirmation
 * - Backpressure in pipelines to prevent buffer overflow
 */

#include "rt_runtime.h"
#include "rt_ipc.h"
#include <stdio.h>
#include <string.h>

// Work request sent from producer to consumer
typedef struct {
    int job_id;
    int data;
} work_request;

// Slow consumer that processes work requests
// Demonstrates: receiving SYNC messages and releasing them
static void consumer_actor(void *arg) {
    (void)arg;

    printf("Consumer: Started (ID: %u)\n", rt_self());
    printf("Consumer: I process slowly to demonstrate backpressure\n\n");

    for (int jobs_processed = 0; jobs_processed < 5; jobs_processed++) {
        // Wait for work request
        rt_message msg;
        rt_status status = rt_ipc_recv(&msg, 5000);  // 5 second timeout

        if (status.code == RT_ERR_TIMEOUT) {
            printf("Consumer: Timeout waiting for work, exiting\n");
            break;
        }

        if (RT_FAILED(status)) {
            printf("Consumer: Receive failed: %s\n", status.msg);
            break;
        }

        work_request *req = (work_request *)msg.data;
        printf("Consumer: Received job #%d (data=%d) from producer %u\n",
               req->job_id, req->data, msg.sender);

        // Simulate processing (producer is BLOCKED during this time)
        printf("Consumer: Processing job #%d...\n", req->job_id);

        // Do some "work" - in real code this would be actual computation
        volatile int sum = 0;
        for (int i = 0; i < 1000000; i++) {
            sum += i;
        }
        (void)sum;

        printf("Consumer: Finished job #%d, releasing message\n", req->job_id);

        // CRITICAL: Release the message to unblock the sender
        // Note: rt_ipc_release() explicitly unblocks the sender
        // If we called rt_ipc_recv() again, it would auto-release,
        // but explicit release is clearer and recommended for SYNC
        rt_ipc_release(&msg);

        printf("Consumer: Producer is now unblocked\n\n");
    }

    printf("Consumer: Done processing, exiting\n");
    rt_exit();
}

// Fast producer that sends work requests
// Demonstrates: sending SYNC messages and being blocked until release
static void producer_actor(void *arg) {
    actor_id consumer_id = (actor_id)(uintptr_t)arg;

    printf("Producer: Started (ID: %u)\n", rt_self());
    printf("Producer: Sending 5 jobs with IPC_SYNC (will block on each)\n\n");

    for (int i = 1; i <= 5; i++) {
        work_request req = {
            .job_id = i,
            .data = i * 100
        };

        printf("Producer: Sending job #%d (will block until consumer releases)...\n", i);

        // Send with IPC_SYNC - this BLOCKS until consumer calls rt_ipc_release()
        rt_status status = rt_ipc_send(consumer_id, &req, sizeof(req), IPC_SYNC);

        if (RT_FAILED(status)) {
            if (status.code == RT_ERR_CLOSED) {
                printf("Producer: Consumer died before releasing! (job #%d)\n", i);
            } else {
                printf("Producer: Send failed: %s\n", status.msg);
            }
            break;
        }

        // We only reach here AFTER consumer has released the message
        printf("Producer: Job #%d acknowledged (consumer released)\n\n", i);
    }

    printf("Producer: All jobs sent and acknowledged, exiting\n");
    rt_exit();
}

// Demonstrate what happens with improper SYNC usage (deadlock scenarios)
static void deadlock_demo_actor(void *arg) {
    (void)arg;

    printf("\n--- Deadlock Prevention Demo ---\n");

    // Example 1: Self-send with SYNC is forbidden (detected and rejected)
    printf("Demo: Attempting self-send with IPC_SYNC...\n");
    int data = 42;
    rt_status status = rt_ipc_send(rt_self(), &data, sizeof(data), IPC_SYNC);

    if (RT_FAILED(status)) {
        printf("Demo: Self-send correctly rejected: %s\n", status.msg);
    }

    // Example 2: ASYNC self-send works fine
    printf("Demo: Self-send with IPC_ASYNC works...\n");
    status = rt_ipc_send(rt_self(), &data, sizeof(data), IPC_ASYNC);
    if (!RT_FAILED(status)) {
        rt_message msg;
        rt_ipc_recv(&msg, 0);
        printf("Demo: Received self-sent ASYNC message: %d\n", *(int *)msg.data);
    }

    printf("--- End Deadlock Demo ---\n\n");
    rt_exit();
}

int main(void) {
    printf("=== IPC_SYNC Example - Synchronous Message Passing ===\n\n");

    printf("This example shows:\n");
    printf("1. Producer sends jobs with IPC_SYNC (blocks until acknowledged)\n");
    printf("2. Consumer processes slowly, creating natural backpressure\n");
    printf("3. Producer can only send next job after consumer releases previous\n\n");

    rt_status status = rt_init();
    if (RT_FAILED(status)) {
        fprintf(stderr, "Failed to initialize runtime: %s\n", status.msg);
        return 1;
    }

    // First, run the deadlock prevention demo
    actor_id demo = rt_spawn(deadlock_demo_actor, NULL);
    if (demo == ACTOR_ID_INVALID) {
        fprintf(stderr, "Failed to spawn demo actor\n");
        rt_cleanup();
        return 1;
    }

    // Spawn consumer first (it will wait for messages)
    actor_id consumer = rt_spawn(consumer_actor, NULL);
    if (consumer == ACTOR_ID_INVALID) {
        fprintf(stderr, "Failed to spawn consumer\n");
        rt_cleanup();
        return 1;
    }

    // Spawn producer with consumer's ID
    actor_id producer = rt_spawn(producer_actor, (void *)(uintptr_t)consumer);
    if (producer == ACTOR_ID_INVALID) {
        fprintf(stderr, "Failed to spawn producer\n");
        rt_cleanup();
        return 1;
    }

    printf("Spawned actors: demo=%u, consumer=%u, producer=%u\n\n", demo, consumer, producer);

    // Run scheduler
    rt_run();

    printf("\nScheduler finished\n");
    rt_cleanup();

    printf("\n=== Example completed ===\n");
    printf("\nKey takeaways:\n");
    printf("- IPC_SYNC provides natural flow control (backpressure)\n");
    printf("- Sender blocks until receiver explicitly releases\n");
    printf("- Always call rt_ipc_release() for SYNC messages\n");
    printf("- Never do circular SYNC sends (deadlock)\n");

    return 0;
}
