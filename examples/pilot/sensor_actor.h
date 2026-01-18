// Sensor actor - Timer-driven sensor reading
//
// Periodically reads raw sensors via HAL, publishes to sensor bus.
// Sensor fusion is done by the estimator actor.

#ifndef SENSOR_ACTOR_H
#define SENSOR_ACTOR_H

#include "hive_bus.h"

// Initialize the sensor actor module with bus ID.
// Must be called before spawning the actor.
void sensor_actor_init(bus_id sensor_bus);

// Actor entry point - spawn this with hive_spawn()
void sensor_actor(void *args, const hive_spawn_info *siblings,
                  size_t sibling_count);

#endif // SENSOR_ACTOR_H
