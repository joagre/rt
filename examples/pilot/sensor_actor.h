// Sensor actor - Timer-driven sensor reading
//
// Periodically reads sensors via HAL, publishes to IMU bus.

#ifndef SENSOR_ACTOR_H
#define SENSOR_ACTOR_H

#include "hive_bus.h"

// Initialize the sensor actor module with bus ID.
// Must be called before spawning the actor.
void sensor_actor_init(bus_id imu_bus);

// Actor entry point - spawn this with hive_spawn()
void sensor_actor(void *arg);

#endif // SENSOR_ACTOR_H
