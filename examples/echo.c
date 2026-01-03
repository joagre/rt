#include "rt_runtime.h"
#include "rt_net.h"
#include "rt_ipc.h"
#include <stdio.h>
#include <string.h>

#define ECHO_PORT 8080

// Message to coordinate between client and server
typedef struct {
    bool ready;
} sync_msg;

// Echo server actor
static void server_actor(void *arg) {
    (void)arg;

    printf("Server actor started (ID: %u)\n", rt_self());

    // Listen on port
    int listen_fd;
    rt_status status = rt_net_listen(ECHO_PORT, &listen_fd);
    if (RT_FAILED(status)) {
        printf("Server: Failed to listen: %s\n",
               status.msg ? status.msg : "unknown error");
        rt_exit();
    }

    printf("Server: Listening on port %d (fd=%d)\n", ECHO_PORT, listen_fd);

    // Accept connection (block forever)
    int conn_fd;
    status = rt_net_accept(listen_fd, &conn_fd, -1);
    if (RT_FAILED(status)) {
        printf("Server: Failed to accept: %s\n",
               status.msg ? status.msg : "unknown error");
        rt_net_close(listen_fd);
        rt_exit();
    }

    printf("Server: Accepted connection (fd=%d)\n", conn_fd);

    // Echo loop
    char buffer[256];
    for (int i = 0; i < 3; i++) {
        size_t received;
        status = rt_net_recv(conn_fd, buffer, sizeof(buffer) - 1, &received, -1);
        if (RT_FAILED(status)) {
            printf("Server: Failed to receive: %s\n",
                   status.msg ? status.msg : "unknown error");
            break;
        }

        if (received == 0) {
            printf("Server: Client disconnected\n");
            break;
        }

        buffer[received] = '\0';
        printf("Server: Received %zu bytes: \"%s\"\n", received, buffer);

        // Echo back
        size_t sent;
        status = rt_net_send(conn_fd, buffer, received, &sent, -1);
        if (RT_FAILED(status)) {
            printf("Server: Failed to send: %s\n",
                   status.msg ? status.msg : "unknown error");
            break;
        }

        printf("Server: Echoed %zu bytes\n", sent);
    }

    // Close connection
    rt_net_close(conn_fd);
    rt_net_close(listen_fd);

    printf("Server: Done!\n");
    rt_exit();
}

// Messages to send (static to avoid stack issues)
static const char *messages[] = {
    "Hello, Server!",
    "How are you?",
    "Goodbye!"
};

// Echo client actor
static void client_actor(void *arg) {
    (void)arg;

    printf("Client actor started (ID: %u)\n", rt_self());

    // Wait briefly for server to start listening
    for (int i = 0; i < 100; i++) {
        rt_yield();
    }

    printf("Client: Connecting to server...\n");

    // Connect to server (with timeout)
    int conn_fd;
    rt_status status = rt_net_connect("localhost", ECHO_PORT, &conn_fd, 5000); // 5 second timeout
    if (RT_FAILED(status)) {
        printf("Client: Failed to connect: %s\n",
               status.msg ? status.msg : "unknown error");
        rt_exit();
    }

    printf("Client: Connected (fd=%d)\n", conn_fd);

    for (int i = 0; i < 3; i++) {
        // Send message
        size_t sent;
        status = rt_net_send(conn_fd, messages[i], strlen(messages[i]), &sent, -1);
        if (RT_FAILED(status)) {
            printf("Client: Failed to send: %s\n",
                   status.msg ? status.msg : "unknown error");
            break;
        }

        printf("Client: Sent %zu bytes: \"%s\"\n", sent, messages[i]);

        // Receive echo
        char buffer[256];
        size_t received;
        status = rt_net_recv(conn_fd, buffer, sizeof(buffer) - 1, &received, -1);
        if (RT_FAILED(status)) {
            printf("Client: Failed to receive: %s\n",
                   status.msg ? status.msg : "unknown error");
            break;
        }

        buffer[received] = '\0';
        printf("Client: Received echo: \"%s\"\n", buffer);
    }

    // Close connection
    rt_net_close(conn_fd);

    printf("Client: Done!\n");
    rt_exit();
}

int main(void) {
    printf("=== Actor Runtime Echo Server/Client Example ===\n\n");

    // Initialize runtime
    rt_config cfg = RT_CONFIG_DEFAULT;
    cfg.max_actors = 10;
    cfg.default_stack_size = 65536;

    rt_status status = rt_init(&cfg);
    if (RT_FAILED(status)) {
        fprintf(stderr, "Failed to initialize runtime: %s\n",
                status.msg ? status.msg : "unknown error");
        return 1;
    }

    // Spawn server actor first
    actor_config server_cfg = RT_ACTOR_CONFIG_DEFAULT;
    server_cfg.name = "server";
    server_cfg.priority = RT_PRIO_NORMAL;

    actor_id server_id = rt_spawn_ex(server_actor, NULL, &server_cfg);
    if (server_id == ACTOR_ID_INVALID) {
        fprintf(stderr, "Failed to spawn server actor\n");
        rt_cleanup();
        return 1;
    }

    // Spawn client actor
    actor_config client_cfg = RT_ACTOR_CONFIG_DEFAULT;
    client_cfg.name = "client";
    client_cfg.priority = RT_PRIO_NORMAL;

    actor_id client_id = rt_spawn_ex(client_actor, NULL, &client_cfg);
    if (client_id == ACTOR_ID_INVALID) {
        fprintf(stderr, "Failed to spawn client actor\n");
        rt_cleanup();
        return 1;
    }

    // Run scheduler
    rt_run();

    printf("\nScheduler finished\n");

    // Cleanup
    rt_cleanup();

    printf("\n=== Example completed ===\n");

    return 0;
}
