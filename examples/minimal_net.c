#include "rt_runtime.h"
#include <stdio.h>

int main(void) {
    printf("=== Minimal Network Test ===\n\n");

    // Initialize runtime
    rt_config cfg = RT_CONFIG_DEFAULT;
    cfg.max_actors = 2;

    rt_status status = rt_init(&cfg);
    if (RT_FAILED(status)) {
        fprintf(stderr, "Failed to initialize runtime: %s\n",
                status.msg ? status.msg : "unknown error");
        return 1;
    }

    printf("Runtime initialized successfully\n");

    // Cleanup immediately
    rt_cleanup();

    printf("\n=== Test completed ===\n");

    return 0;
}
