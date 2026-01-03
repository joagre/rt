#include "rt_runtime.h"
#include <stdio.h>

int main(void) {
    printf("Starting minimal network test\n");
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

    printf("Runtime initialized successfully\n");
    fflush(stdout);

    // Cleanup immediately
    rt_cleanup();

    printf("Test completed\n");

    return 0;
}
