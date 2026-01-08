// Sensor actor - Hardware sensor reading
//
// Reads sensors from platform layer and publishes to IMU bus.

#include "sensor_actor.h"
#include "config.h"
#include "hive_runtime.h"
#include "hive_bus.h"

static bus_id s_imu_bus;
static imu_read_fn s_read_fn;

void sensor_actor_init(bus_id imu_bus, imu_read_fn read_fn) {
    s_imu_bus = imu_bus;
    s_read_fn = read_fn;
}

void sensor_actor(void *arg) {
    (void)arg;

    while (1) {
        if (s_read_fn) {
            imu_data_t imu;
            s_read_fn(&imu);
            hive_bus_publish(s_imu_bus, &imu, sizeof(imu));
        }

        hive_yield();
    }
}
