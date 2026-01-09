// Motor actor - Mixer and safety layer
//
// Subscribes to torque bus, applies mixer, enforces limits, implements watchdog,
// writes to platform layer.

#ifndef MOTOR_ACTOR_H
#define MOTOR_ACTOR_H

#include "hive_bus.h"
#include "types.h"

// Platform write function type - provided by platform layer
typedef void (*motor_write_fn)(const motor_cmd_t *cmd);

// Initialize the motor actor module with bus ID and platform function.
// Must be called before spawning the actor.
void motor_actor_init(bus_id torque_bus, motor_write_fn write_fn);

// Actor entry point - spawn this with hive_spawn()
void motor_actor(void *arg);

#endif // MOTOR_ACTOR_H
