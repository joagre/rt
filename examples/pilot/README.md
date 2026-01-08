# Pilot Example

Minimal quadcopter example using hive actor runtime with Webots simulator.

## What it does

1. Main loop reads IMU sensors from Webots
2. Publishes sensor data to a bus
3. Calls `hive_step()` to run actors
4. Sensor actor reads from bus and prints roll/pitch/yaw every second

## Prerequisites

- Webots installed (https://cyberbotics.com/)
- hive runtime built: `cd ../.. && make`

## Build and Run

```bash
export WEBOTS_HOME=/usr/local/webots  # adjust path
make
make install
```

Then open `worlds/hover_test.wbt` in Webots.

## Files

- `pilot.c` - Main loop and sensor actor (~90 lines)
- `Makefile` - Build configuration
- `worlds/hover_test.wbt` - Webots world with Crazyflie

## How Webots Controllers Work

Understanding the Webots execution model is crucial for writing controllers.

### Process Model

Webots runs your controller as a **separate process**. When you start a simulation:

1. Webots spawns your executable (`pilot`)
2. Your `main()` runs
3. Webots and your controller communicate via IPC

### The Step Function

The key function is `wb_robot_step(TIME_STEP)`. It synchronizes your controller with the simulator:

```
Your controller                    Webots simulator
───────────────                    ────────────────
main() starts
  │
  ├─► wb_robot_init()  ──────────► "Controller connected"
  │
  ├─► enable sensors
  │
  └─► while loop:
        │
        ├─► wb_robot_step(4) ────► BLOCKS here
        │         │                    │
        │         │                    ▼
        │         │               Simulate 4ms of physics
        │         │               Update sensor values
        │         │               Apply motor commands
        │         │                    │
        │         ◄────────────────────┘ Returns when done
        │
        ├─► read sensors (values from previous step)
        ├─► your logic (hive_step runs actors)
        ├─► set motors (applied on next step)
        │
        └─► loop back to wb_robot_step()
```

### Key Points

1. **`wb_robot_step()` blocks** until Webots advances simulation by TIME_STEP milliseconds
2. **Sensors read values from the previous step** (there's a 1-step delay)
3. **Motor commands take effect on the next step** (another 1-step delay)
4. **Return value of -1** means simulation ended (user pressed stop)
5. **If your code is slow**, simulation slows down to wait for you

### Timing

With `TIME_STEP=4`:
- Your loop runs **250 times per simulated second**
- If your code takes <4ms real time, simulation runs at real-time speed
- If your code takes >4ms, simulation runs slower than real-time

### Integration with Hive Runtime

In this example, we use `hive_step()` instead of `hive_run()`:

```c
while (wb_robot_step(TIME_STEP) != -1) {
    // Called every 4ms of simulation time

    // 1. Read sensors from Webots, publish to bus
    const double *rpy = wb_inertial_unit_get_roll_pitch_yaw(imu);
    hive_bus_publish(imu_bus, &data, sizeof(data));

    // 2. Run each ready actor exactly once
    hive_step();

    // 3. (Future: read motor commands from bus, write to Webots)
}
```

This works because:
- **Webots controls time**, not the hive scheduler
- **No hive timers needed** - the loop rate is determined by `wb_robot_step()`
- **Actors yield after processing** - `hive_step()` runs each once and returns
- **Deterministic** - same inputs produce same outputs (no timing races)
