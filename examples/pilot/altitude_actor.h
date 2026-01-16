// Altitude actor - Altitude hold control
//
// Subscribes to state and position target buses, runs altitude PID controller,
// publishes thrust commands to thrust bus.
//
// Supports controlled landing via NOTIFY_LANDING message. When received,
// descends at a fixed rate until touchdown, then notifies flight manager.

#ifndef ALTITUDE_ACTOR_H
#define ALTITUDE_ACTOR_H

#include "hive_bus.h"
#include "hive_runtime.h"

// Initialize the altitude actor module with bus IDs and flight manager actor.
// Must be called before spawning the actor.
void altitude_actor_init(bus_id state_bus, bus_id thrust_bus,
                         bus_id position_target_bus,
                         actor_id flight_manager_actor);

// Actor entry point - spawn this with hive_spawn()
void altitude_actor(void *arg);

#endif // ALTITUDE_ACTOR_H
