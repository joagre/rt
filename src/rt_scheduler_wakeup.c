#include "rt_scheduler_wakeup.h"
#include "rt_log.h"

#ifdef __linux__
    #include <sys/eventfd.h>
    #include <unistd.h>
    #include <stdint.h>

    static int g_wakeup_fd = -1;

    rt_status rt_scheduler_wakeup_init(void) {
        g_wakeup_fd = eventfd(0, EFD_SEMAPHORE);
        if (g_wakeup_fd < 0) {
            return RT_ERROR(RT_ERR_IO, "Failed to create scheduler wakeup eventfd");
        }
        RT_LOG_DEBUG("Scheduler wakeup initialized (eventfd=%d)", g_wakeup_fd);
        return RT_SUCCESS;
    }

    void rt_scheduler_wakeup_cleanup(void) {
        if (g_wakeup_fd >= 0) {
            close(g_wakeup_fd);
            g_wakeup_fd = -1;
        }
    }

    void rt_scheduler_wakeup_signal(void) {
        if (g_wakeup_fd >= 0) {
            uint64_t val = 1;
            ssize_t n = write(g_wakeup_fd, &val, sizeof(val));
            (void)n;  // Ignore errors - scheduler will wake up eventually via timeout
        }
    }

    void rt_scheduler_wakeup_wait(void) {
        if (g_wakeup_fd >= 0) {
            uint64_t val;
            ssize_t n = read(g_wakeup_fd, &val, sizeof(val));
            (void)n;  // Ignore errors
        }
    }

#else
    // FreeRTOS implementation (future)
    #include <FreeRTOS.h>
    #include <semphr.h>

    static SemaphoreHandle_t g_wakeup_sem = NULL;

    rt_status rt_scheduler_wakeup_init(void) {
        g_wakeup_sem = xSemaphoreCreateBinary();
        if (g_wakeup_sem == NULL) {
            return RT_ERROR(RT_ERR_NOMEM, "Failed to create scheduler wakeup semaphore");
        }
        RT_LOG_DEBUG("Scheduler wakeup initialized (semaphore)");
        return RT_SUCCESS;
    }

    void rt_scheduler_wakeup_cleanup(void) {
        if (g_wakeup_sem != NULL) {
            vSemaphoreDelete(g_wakeup_sem);
            g_wakeup_sem = NULL;
        }
    }

    void rt_scheduler_wakeup_signal(void) {
        if (g_wakeup_sem != NULL) {
            BaseType_t higher_prio_woken = pdFALSE;
            xSemaphoreGiveFromISR(g_wakeup_sem, &higher_prio_woken);
            portYIELD_FROM_ISR(higher_prio_woken);
        }
    }

    void rt_scheduler_wakeup_wait(void) {
        if (g_wakeup_sem != NULL) {
            xSemaphoreTake(g_wakeup_sem, portMAX_DELAY);
        }
    }
#endif
