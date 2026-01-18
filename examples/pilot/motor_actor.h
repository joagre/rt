// Motor actor - Output layer
//
// Subscribes to torque bus, writes to hardware via HAL.

#ifndef MOTOR_ACTOR_H
#define MOTOR_ACTOR_H

#include "hive_bus.h"

// Initialize the motor actor module with bus ID.
// Must be called before spawning the actor.
void motor_actor_init(bus_id torque_bus);

// Actor entry point - spawn this with hive_spawn()
void motor_actor(void *args, const hive_spawn_info *siblings,
                 size_t sibling_count);

#endif // MOTOR_ACTOR_H
