// Position actor - Horizontal position hold control
//
// Subscribes to state bus, computes desired roll/pitch angles to hold
// target XY position, publishes angle setpoints.

#ifndef POSITION_ACTOR_H
#define POSITION_ACTOR_H

#include "types.h"
#include "hive_bus.h"

void position_actor_init(bus_id state_bus, bus_id angle_setpoint_bus);
void position_actor(void *arg);

#endif // POSITION_ACTOR_H
