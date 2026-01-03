#include "rt_runtime.h"
#include "rt_net.h"
#include <stdio.h>

#define ECHO_PORT 8080

// Simple server actor that just tries to listen
static void server_actor(void *arg) {
    (void)arg;

    printf("Server actor started (ID: %u)\n", rt_self());
    fflush(stdout);

    int listen_fd;
    rt_status status = rt_net_listen(ECHO_PORT, &listen_fd);
    if (RT_FAILED(status)) {
        printf("Server: Failed to listen: %s\n",
               status.msg ? status.msg : "unknown error");
        fflush(stdout);
        rt_exit();
    }

    printf("Server: Listening on port %d\n", ECHO_PORT);
    fflush(stdout);

    rt_net_close(listen_fd);
    printf("Server: Done!\n");
    fflush(stdout);
    rt_exit();
}

int main(void) {
    printf("=== Minimal Echo Test ===\n");
    fflush(stdout);

    // Initialize runtime
    rt_config cfg = RT_CONFIG_DEFAULT;
    cfg.max_actors = 2;
    cfg.default_stack_size = 65536;

    printf("Calling rt_init\n");
    fflush(stdout);

    rt_status status = rt_init(&cfg);
    if (RT_FAILED(status)) {
        fprintf(stderr, "Failed to initialize runtime: %s\n",
                status.msg ? status.msg : "unknown error");
        return 1;
    }

    printf("Runtime initialized\n");
    fflush(stdout);

    // Spawn server actor
    actor_config server_cfg = RT_ACTOR_CONFIG_DEFAULT;
    server_cfg.name = "server";
    server_cfg.priority = RT_PRIO_NORMAL;

    printf("Spawning server actor\n");
    fflush(stdout);

    actor_id server_id = rt_spawn_ex(server_actor, NULL, &server_cfg);
    if (server_id == ACTOR_ID_INVALID) {
        fprintf(stderr, "Failed to spawn server actor\n");
        rt_cleanup();
        return 1;
    }

    printf("Spawned server actor (ID: %u)\n", server_id);
    printf("Starting scheduler...\n");
    fflush(stdout);

    // Run scheduler
    rt_run();

    printf("Scheduler finished\n");
    fflush(stdout);

    // Cleanup
    rt_cleanup();

    printf("=== Test completed ===\n");

    return 0;
}
