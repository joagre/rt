#include "rt_runtime.h"
#include "rt_file.h"
#include "rt_ipc.h"
#include "rt_timer.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// Test results
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_PASS(name) do { printf("  PASS: %s\n", name); tests_passed++; } while(0)
#define TEST_FAIL(name) do { printf("  FAIL: %s\n", name); tests_failed++; } while(0)

// Test file path
static const char *TEST_FILE = "/tmp/rt_file_test.tmp";

static void run_file_tests(void *arg) {
    (void)arg;

    // ========================================================================
    // Test 1: Open file for writing (create)
    // ========================================================================
    printf("\nTest 1: Open file for writing (create)\n");
    int fd = -1;
    {
        rt_status status = rt_file_open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644, &fd);
        if (RT_FAILED(status)) {
            printf("    Error: %s\n", status.msg ? status.msg : "unknown");
            TEST_FAIL("rt_file_open for write");
        } else if (fd < 0) {
            TEST_FAIL("got invalid fd");
        } else {
            TEST_PASS("open file for writing");
        }
    }

    // ========================================================================
    // Test 2: Write to file
    // ========================================================================
    printf("\nTest 2: Write to file\n");
    {
        const char *data = "Hello, RT File System!";
        size_t len = strlen(data);
        size_t actual = 0;

        rt_status status = rt_file_write(fd, data, len, &actual);
        if (RT_FAILED(status)) {
            printf("    Error: %s\n", status.msg ? status.msg : "unknown");
            TEST_FAIL("rt_file_write");
        } else if (actual != len) {
            printf("    Wrote %zu/%zu bytes\n", actual, len);
            TEST_FAIL("incomplete write");
        } else {
            TEST_PASS("write to file");
        }
    }

    // ========================================================================
    // Test 3: Sync file to disk
    // ========================================================================
    printf("\nTest 3: Sync file to disk\n");
    {
        rt_status status = rt_file_sync(fd);
        if (RT_FAILED(status)) {
            printf("    Error: %s\n", status.msg ? status.msg : "unknown");
            TEST_FAIL("rt_file_sync");
        } else {
            TEST_PASS("sync file to disk");
        }
    }

    // ========================================================================
    // Test 4: Close file
    // ========================================================================
    printf("\nTest 4: Close file\n");
    {
        rt_status status = rt_file_close(fd);
        if (RT_FAILED(status)) {
            printf("    Error: %s\n", status.msg ? status.msg : "unknown");
            TEST_FAIL("rt_file_close");
        } else {
            TEST_PASS("close file");
        }
    }

    // ========================================================================
    // Test 5: Open file for reading
    // ========================================================================
    printf("\nTest 5: Open file for reading\n");
    {
        rt_status status = rt_file_open(TEST_FILE, O_RDONLY, 0, &fd);
        if (RT_FAILED(status)) {
            printf("    Error: %s\n", status.msg ? status.msg : "unknown");
            TEST_FAIL("rt_file_open for read");
        } else {
            TEST_PASS("open file for reading");
        }
    }

    // ========================================================================
    // Test 6: Read from file
    // ========================================================================
    printf("\nTest 6: Read from file\n");
    {
        char buf[64] = {0};
        size_t actual = 0;

        rt_status status = rt_file_read(fd, buf, sizeof(buf) - 1, &actual);
        if (RT_FAILED(status)) {
            printf("    Error: %s\n", status.msg ? status.msg : "unknown");
            TEST_FAIL("rt_file_read");
        } else if (strcmp(buf, "Hello, RT File System!") != 0) {
            printf("    Read: '%s'\n", buf);
            TEST_FAIL("data mismatch");
        } else {
            TEST_PASS("read from file");
        }
    }

    // ========================================================================
    // Test 7: pread (read at offset without changing position)
    // ========================================================================
    printf("\nTest 7: pread (read at offset)\n");
    {
        char buf[16] = {0};
        size_t actual = 0;

        // Read "RT" starting at offset 7
        rt_status status = rt_file_pread(fd, buf, 2, 7, &actual);
        if (RT_FAILED(status)) {
            printf("    Error: %s\n", status.msg ? status.msg : "unknown");
            TEST_FAIL("rt_file_pread");
        } else if (strncmp(buf, "RT", 2) != 0) {
            printf("    Read: '%s' (expected 'RT')\n", buf);
            TEST_FAIL("pread data mismatch");
        } else {
            TEST_PASS("pread at offset");
        }
    }

    // Close the read fd
    rt_file_close(fd);

    // ========================================================================
    // Test 8: pwrite (write at offset)
    // ========================================================================
    printf("\nTest 8: pwrite (write at offset)\n");
    {
        rt_status status = rt_file_open(TEST_FILE, O_RDWR, 0, &fd);
        if (RT_FAILED(status)) {
            TEST_FAIL("open for pwrite");
        } else {
            size_t actual = 0;
            // Overwrite "RT" with "XX" at offset 7
            status = rt_file_pwrite(fd, "XX", 2, 7, &actual);
            if (RT_FAILED(status)) {
                printf("    Error: %s\n", status.msg ? status.msg : "unknown");
                TEST_FAIL("rt_file_pwrite");
            } else {
                // Verify by reading back
                char buf[64] = {0};
                rt_file_pread(fd, buf, sizeof(buf) - 1, 0, &actual);
                if (strncmp(buf + 7, "XX", 2) == 0) {
                    TEST_PASS("pwrite at offset");
                } else {
                    printf("    Read back: '%s'\n", buf);
                    TEST_FAIL("pwrite verification failed");
                }
            }
            rt_file_close(fd);
        }
    }

    // ========================================================================
    // Test 9: Open non-existent file without O_CREAT fails
    // ========================================================================
    printf("\nTest 9: Open non-existent file fails\n");
    {
        rt_status status = rt_file_open("/tmp/nonexistent_rt_test_file_xyz.tmp", O_RDONLY, 0, &fd);
        if (RT_FAILED(status)) {
            TEST_PASS("open non-existent file fails");
        } else {
            rt_file_close(fd);
            TEST_FAIL("should fail to open non-existent file");
        }
    }

    // ========================================================================
    // Test 10: Close invalid fd
    // ========================================================================
    printf("\nTest 10: Close invalid fd\n");
    {
        rt_status status = rt_file_close(-1);
        if (RT_FAILED(status)) {
            TEST_PASS("close invalid fd fails");
        } else {
            TEST_FAIL("should fail to close invalid fd");
        }
    }

    // Cleanup test file
    unlink(TEST_FILE);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n%s\n", tests_failed == 0 ? "All tests passed!" : "Some tests FAILED!");

    rt_exit();
}

int main(void) {
    printf("=== File I/O (rt_file) Test Suite ===\n");

    rt_status status = rt_init();
    if (RT_FAILED(status)) {
        fprintf(stderr, "Failed to initialize runtime: %s\n",
                status.msg ? status.msg : "unknown error");
        return 1;
    }

    actor_config cfg = RT_ACTOR_CONFIG_DEFAULT;
    cfg.stack_size = 128 * 1024;

    actor_id runner = rt_spawn_ex(run_file_tests, NULL, &cfg);
    if (runner == ACTOR_ID_INVALID) {
        fprintf(stderr, "Failed to spawn test runner\n");
        rt_cleanup();
        return 1;
    }

    rt_run();
    rt_cleanup();

    return tests_failed > 0 ? 1 : 0;
}
