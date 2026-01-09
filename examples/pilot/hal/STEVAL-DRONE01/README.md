# STEVAL-DRONE01 Hardware Abstraction Layer

Platform layer for the STMicroelectronics STEVAL-DRONE01 mini drone kit.

## Hardware Overview

| Component | Part Number | Interface | Description |
|-----------|-------------|-----------|-------------|
| MCU | STM32F401 | - | ARM Cortex-M4, 84 MHz, DSP+FPU |
| IMU | LSM6DSL | SPI1 | 6-axis accel + gyro |
| Magnetometer | LIS2MDL | I2C | 3-axis compass |
| Barometer | LPS22HD | I2C | Pressure/altitude |
| Bluetooth | SPBTLE-RF | SPI2 | BLE 4.1 for RC |
| Motors | 85x20mm | TIM4 PWM | 3.7V brushed, x4 |

## Specifications

**Flight Controller (STEVAL-FCU001V1):**
- STM32F401 @ 84 MHz (Cortex-M4F)
- 256 KB Flash, 64 KB RAM
- Hardware FPU for fast sensor fusion

**Sensors:**
- LSM6DSL: ±2/4/8/16g accel, ±125/250/500/1000/2000 dps gyro
- LIS2MDL: ±50 gauss magnetometer
- LPS22HD: 260-1260 hPa barometer

**Motors:**
- Type: Brushed DC, 85x20mm
- Voltage: 3.7V (direct LiPo drive)
- 2x CW, 2x CCW rotation
- Propellers: 65mm

**Battery:**
- LiPo 3.7V / 600mAh
- Max discharge: 30C (18A)

## Pin Mapping

```
Motors (TIM4 PWM):
  M1 (rear-left)   - TIM4_CH1
  M2 (front-left)  - TIM4_CH2
  M3 (front-right) - TIM4_CH3
  M4 (rear-right)  - TIM4_CH4

Sensors:
  LSM6DSL - SPI1 (accel/gyro)
  LIS2MDL - I2C1 (magnetometer)
  LPS22HD - I2C1 (barometer)

Bluetooth:
  SPBTLE-RF - SPI2

Debug:
  UART1 - Serial debug
  JTAG  - Programming/debug
```

## Differences from Webots Simulation

| Feature | Webots | STEVAL-DRONE01 |
|---------|--------|----------------|
| Position | GPS (perfect) | None (need external tracking) |
| Attitude | Inertial unit (fused) | Raw IMU (need sensor fusion) |
| Altitude | GPS Z | Barometer (relative only) |
| Heading | Direct yaw | Magnetometer (calibration needed) |
| Timing | Simulation step | Real-time (hardware timers) |

## Sensor Fusion Requirements

The Webots `inertial_unit` provides pre-fused roll/pitch/yaw. On real hardware, the estimator actor must implement:

1. **Complementary filter** or **Kalman filter** for attitude
2. **Gyro integration** for angular rates
3. **Magnetometer fusion** for yaw (with tilt compensation)
4. **Barometer filtering** for altitude hold

## Resources

- [STEVAL-DRONE01 Product Page](https://www.st.com/en/evaluation-tools/steval-drone01.html)
- [Datasheet (PDF)](https://www.mouser.com/datasheet/2/389/steval-drone01-1500114.pdf)
- [STSW-FCU001 Firmware](https://www.st.com/en/embedded-software/stsw-fcu001.html)

## TODO

- [ ] LSM6DSL driver (SPI)
- [ ] LIS2MDL driver (I2C)
- [ ] LPS22HD driver (I2C)
- [ ] Motor PWM driver (TIM4)
- [ ] Complementary filter for attitude
- [ ] Platform init and main loop
- [ ] hive runtime port for STM32F4
