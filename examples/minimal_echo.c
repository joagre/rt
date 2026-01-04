#include "rt_runtime.h"
#include "rt_net.h"
#include <stdio.h>

#define ECHO_PORT 8080

// Simple server actor that just tries to listen
static void server_actor(void *arg) {
    (void)arg;

    printf("Server actor started (ID: %u)\n", rt_self());

    int listen_fd;
    rt_status status = rt_net_listen(ECHO_PORT, &listen_fd);
    if (RT_FAILED(status)) {
        printf("Server: Failed to listen: %s\n",
               status.msg ? status.msg : "unknown error");
        rt_exit();
    }

    printf("Server: Listening on port %d\n", ECHO_PORT);

    rt_net_close(listen_fd);
    printf("Server: Done!\n");
    rt_exit();
}

int main(void) {
    printf("=== Minimal Echo Test ===\n\n");

    // Initialize runtime
    rt_config cfg = RT_CONFIG_DEFAULT;
    cfg.max_actors = 2;

    rt_status status = rt_init(&cfg);
    if (RT_FAILED(status)) {
        fprintf(stderr, "Failed to initialize runtime: %s\n",
                status.msg ? status.msg : "unknown error");
        return 1;
    }

    // Spawn server actor
    actor_config server_cfg = RT_ACTOR_CONFIG_DEFAULT;
    server_cfg.name = "server";

    actor_id server_id = rt_spawn_ex(server_actor, NULL, &server_cfg);
    if (server_id == ACTOR_ID_INVALID) {
        fprintf(stderr, "Failed to spawn server actor\n");
        rt_cleanup();
        return 1;
    }

    // Run scheduler
    rt_run();

    printf("\nScheduler finished\n");

    // Cleanup
    rt_cleanup();

    printf("=== Test completed ===\n");

    return 0;
}
