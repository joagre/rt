// Sensor actor - Timer-driven sensor reading
//
// Periodically reads sensors from platform layer, publishes to IMU bus.

#include "sensor_actor.h"
#include "config.h"
#include "hive_runtime.h"
#include "hive_bus.h"
#include "hive_timer.h"
#include "hive_ipc.h"
#include <assert.h>

#define SENSOR_INTERVAL_US  (TIME_STEP_MS * 1000)

static bus_id s_imu_bus;
static read_imu_fn s_read_imu;

void sensor_actor_init(bus_id imu_bus, read_imu_fn read_imu) {
    assert(read_imu != NULL);
    s_imu_bus = imu_bus;
    s_read_imu = read_imu;
}

void sensor_actor(void *arg) {
    (void)arg;

    timer_id timer;
    hive_status status = hive_timer_every(SENSOR_INTERVAL_US, &timer);
    assert(HIVE_SUCCEEDED(status));

    while (1) {
        hive_message msg;
        hive_ipc_recv_match(HIVE_SENDER_ANY, HIVE_MSG_TIMER, timer, &msg, -1);

        imu_data_t imu;
        s_read_imu(&imu);
        hive_bus_publish(s_imu_bus, &imu, sizeof(imu));
    }
}
