// Estimator actor - Attitude estimation and state computation
//
// Subscribes to sensor bus, runs complementary filter for attitude,
// computes velocities, publishes state estimate.
//
// Uses portable complementary filter from fusion/complementary_filter.c
// that fuses accelerometer and gyroscope for roll/pitch, and optionally
// magnetometer for yaw.

#ifndef ESTIMATOR_ACTOR_H
#define ESTIMATOR_ACTOR_H

#include "hive_bus.h"
#include "hive_types.h"

// Initialize the estimator actor module with bus IDs.
// Must be called before spawning the actor.
void estimator_actor_init(bus_id sensor_bus, bus_id state_bus);

// Actor entry point - spawn this with hive_spawn()
void estimator_actor(void *args, const hive_spawn_info *siblings,
                     size_t sibling_count);

#endif // ESTIMATOR_ACTOR_H
