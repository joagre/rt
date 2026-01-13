// Sensor actor - Timer-driven sensor reading
//
// Periodically reads raw sensors via HAL, publishes to sensor bus.
// Sensor fusion is done by the estimator actor.

#include "sensor_actor.h"
#include "config.h"
#include "hal/hal.h"
#include "hive_runtime.h"
#include "hive_bus.h"
#include "hive_timer.h"
#include "hive_ipc.h"
#include <assert.h>

#define SENSOR_INTERVAL_US  (TIME_STEP_MS * 1000)

static bus_id s_sensor_bus;

void sensor_actor_init(bus_id sensor_bus) {
    s_sensor_bus = sensor_bus;
}

void sensor_actor(void *arg) {
    (void)arg;

    timer_id timer;
    hive_status status = hive_timer_every(SENSOR_INTERVAL_US, &timer);
    (void)status;

    while (1) {
        hive_message msg;
        hive_ipc_recv_match(HIVE_SENDER_ANY, HIVE_MSG_TIMER, timer, &msg, -1);

        sensor_data_t sensors;
        hal_read_sensors(&sensors);
        hive_bus_publish(s_sensor_bus, &sensors, sizeof(sensors));
    }
}
