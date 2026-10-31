# Embedded Foundations: From GPIO to an Integrated ATmega2560 System

**English** | [简体中文](README.zh-CN.md)

This repository documents my study of register-level AVR C programming on the ATmega2560. The exercises begin with LEDs and button inputs, then progress through timers, interrupts, serial communication, sensors, displays, and actuator control. The later projects bring these modules together into an integrated system operated through serial commands.

**Platform:** Mega 2560-compatible development board · ATmega2560 · AVR C · PlatformIO

## Project overview

Scroll through all 40 project cards. Click a card to explore its source code. Projects 20 and 23 are hardware exercises without separate source directories.

[Download the complete PDF](docs/project-cards/ATmega2560_Project_Cards_English.pdf) · [Read the text version](docs/project-cards/project-cards.md)

[![Project 01: Blinking the Onboard LED](docs/project-cards/images/project-01.png)](code/01_bare_metal_blink)

[![Project 02: Alternating Onboard and External LEDs](docs/project-cards/images/project-02.png)](code/02_external_led)

[![Project 03: Setting LED Blink Counts and Intervals](docs/project-cards/images/project-03.png)](code/03_blink_function)

[![Project 04: Cycling Through Three LED Colors](docs/project-cards/images/project-04.png)](code/04_traffic_light)

[![Project 05: Button-Controlled LED with a Reliable Input](docs/project-cards/images/project-05.png)](code/05_button_hold_led)

[![Project 06: Toggling an LED on Each Button Press](docs/project-cards/images/project-06.png)](code/06_button_toggle_led)

[![Project 07: Selecting Four LED Behaviors](docs/project-cards/images/project-07.png)](code/07_button_statemachine)

[![Project 08: Displaying Button Bounce as LED Flashes](docs/project-cards/images/project-08.png)](code/08_measure_button_bounce)

[![Project 09: Reducing Duplicate Button Triggers](docs/project-cards/images/project-09.png)](code/09_nonblocking_debounce_fsm)

[![Project 10: Different Light Responses to Short and Long Presses](docs/project-cards/images/project-10.png)](code/10_short_long_press)

[![Project 11: Controlling Two LEDs with Two Buttons](docs/project-cards/images/project-11.png)](code/11_dual_button_dual_led)

[![Project 12: Viewing Button Events and Counts on a Computer](docs/project-cards/images/project-12.png)](code/12_uart_tx_debug)

[![Project 13: Controlling Two LEDs with Computer Commands](docs/project-cards/images/project-13.png)](code/13_uart_rx_command)

[![Project 14: Switching a Buzzer with Computer Commands](docs/project-cards/images/project-14.png)](code/14_buzzer_square_wave)

[![Project 15: Detecting Button Changes While an LED Blinks](docs/project-cards/images/project-15.png)](code/15_nonblocking_multitask)

[![Project 16: Timing an LED with a Hardware Timer](docs/project-cards/images/project-16.png)](code/16_timer_1ms_tick)

[![Project 17: Continuously Reporting System Uptime](docs/project-cards/images/project-17.png)](code/17-isr-volatile-atomicity)

[![Project 18: Running LED, Button, and Reporting Tasks Together](docs/project-cards/images/project-18.png)](code/18_fixed_tick_tack_scheduler)

[![Project 19: Generating Stepper Pulses and Direction Changes](docs/project-cards/images/project-19.png)](code/19_step_dir_pulse_generator)

![Project 20: First Stepper Motor Operation](docs/project-cards/images/project-20.png)

[![Project 21: Receiving Commands During Motor Operation](docs/project-cards/images/project-21.png)](code/21_nonblocking_stepper_control)

[![Project 22: Stepper Acceleration, Deceleration, and an Automatic Demo](docs/project-cards/images/project-22.png)](code/22_stepper_acceleration_ramp)

![Project 23: Comparing Motor Smoothness, Load, and Heating](docs/project-cards/images/project-23.png)

[![Project 24: Stepper Homing, Limits, and Emergency Stop](docs/project-cards/images/project-24.png)](code/24_stepper_limits_homing_estop)

[![Project 25: Positioning and Repeated Round Trips](docs/project-cards/images/project-25.png)](code/25_stepper_safe_system_integration)

[![Project 26: Receiving Ammonia Sensor Data and Reporting Its Status](docs/project-cards/images/project-26.png)](code/26_ze801_nh3_uart_sensor)

[![Project 27: Fixed Brightness and a Fading LED](docs/project-cards/images/project-27.png)](code/27_nonblocking_pwm_breathing_led)

[![Project 28: Knob-Controlled Brightness and Voltage Readout](docs/project-cards/images/project-28.png)](code/28_potentiometer_led_brightness)

[![Project 29: Reading Two Inputs and Selecting the Dimming Source](docs/project-cards/images/project-29.png)](code/29_adc_register_dual_channel)

[![Project 30: Smoothing and Calibrating Knob Readings](docs/project-cards/images/project-30.png)](code/30_adc_filter_hysteresis_calibration)

[![Project 31: Automatic LED Control from Ambient Light](docs/project-cards/images/project-31.png)](code/31_light_sensor_auto_led)

[![Project 32: Playing Tones, a Scale, and a Melody](docs/project-cards/images/project-32.png)](code/32_nonblocking_buzzer_notes)

[![Project 33: Counting External Events and Comparing Detection Methods](docs/project-cards/images/project-33.png)](code/33_external_interrupt_event_counter)

[![Project 34: Ultrasonic Distance Measurement and Proximity Alerts](docs/project-cards/images/project-34.png)](code/34_nonblocking_ultrasonic_distance)

[![Project 35: Scanning Connected Device Addresses](docs/project-cards/images/project-35.png)](code/35_nonblocking_i2c_scanner)

[![Project 36: MPU-6050 Identification and Three-Axis Acceleration](docs/project-cards/images/project-36.png)](code/36_nonblocking_i2c_sensor_registers)

[![Project 37: SPI Data Verification and Voltage Output](docs/project-cards/images/project-37.png)](code/37_nonblocking_spi_loopback_dac)

[![Project 38: A Multi-Page OLED Status Display](docs/project-cards/images/project-38.png)](code/38_nonblocking_oled_dashboard)

[![Project 39: Servo Angle, Speed, and Knob Tracking](docs/project-cards/images/project-39.png)](code/39_nonblocking_servo_control)

[![Project 40: Automatic Round Trips and Integrated System Control](docs/project-cards/images/project-40.png)](code/40_mega_integrated_control_system)

## Why I started

During Cooper Union's six-week Summer STEM program, I was introduced to embedded development and worked on a sensor project. The pace was fast. Afterward, I realized I needed more time to revisit the foundations and connect registers, program state, communication, and hardware behavior.

While exploring Cornell's ECE curriculum, I discovered **ECE 4760: Digital Systems Design Using Microcontrollers**. Its birdsong synthesis, digital Galton board, and PID-controlled one-dimensional helicopter projects caught my attention. Sound, physical phenomena, and mechanical motion offered compelling ways to investigate how software and hardware work together. [Course and laboratory overview](https://ece4760.github.io/)

To build toward that material, I used ECE 4760's expected skills as a reference and worked with GPT to break the learning goals into progressive ATmega2560 exercises. I wanted to study the course more deeply during twelfth grade, so I began with the Mega 2560 I already had, strengthening my foundations in C, peripheral control, and real-time programming.

My approach is to search for resources, read datasheets, explain the principles, and then write code. When I encounter problems, I use GPT for code review and ask further questions about the parts I do not yet understand.

## Learning progression

| Stage | Main topics | Developing understanding |
|---|---|---|
| GPIO and inputs | LEDs, buttons, edge detection, state changes, debouncing | Distinguishing signal levels, events, and system state |
| Timing and interaction | UART, timers, interrupts, atomic access, periodic tasks | Keeping time while remaining responsive to inputs |
| Motion control | STEP/DIR, speed changes, acceleration and deceleration, limits, homing, emergency stop | Designing motion and exception handling together |
| Peripherals and data | ADC, filtering, PWM, I²C, SPI, sensors, displays | Handling data integrity, freshness, and communication errors |
| System integration | Automatic round trips, status display, task coordination, run records | Examining how modules behave in combination |

The repository currently contains **38 separate project directories, with numbering extending to 40**. The numbers follow my learning project cards. Some exercises continued inside an existing project: the first motor experiment in Project 20 used the Project 19 workspace, so it does not have a separate source directory.

## Three projects to explore

### Project 25: Keeping motion control and serial interaction working together

**Question: While a motor is moving, how can the program keep receiving commands, checking limits, and reporting status?**

This project divides the work among several modules. A timer generates STEP pulses, the motion module manages speed and position, the safety module handles limits and emergency stops, and the serial modules receive commands and send messages. The main loop advances these tasks while each module retains its own state.

One instructive debugging case involved the `help` command.

The help text is relatively long. When the serial transmit buffer filled, a send function waiting for space could hold up the main loop. The hardware continued producing motor pulses, but the main loop could no longer update the final deceleration on time. Reaching the correct step count therefore did not necessarily mean the motion had followed the intended profile.

The revised reporting module records help and status requests, then processes them in portions according to available transmit space. If space is insufficient, the remaining output waits for a later loop iteration while motion and safety tasks continue.

This problem led me to ask: **When a function works on its own, can adding other tasks still disrupt the system's timing?**

[Explore Project 25](code/25_stepper_safe_system_integration) · [Report scheduling code](code/25_stepper_safe_system_integration/src/report.c)

### Project 36: Identifying, configuring, and reading an MPU-6050 from its datasheet

**Question: How should a program decide that a sensor has actually recovered?**

This project separates general I²C communication, MPU-6050 register operations, and the sampling task. The program checks the device identity, resets it, sets its parameters, and reads the configuration back before beginning periodic acceleration measurements.

Measurements must also be committed as complete results. If a read fails, the program retains the previous complete sample and tracks its age, preventing a mixture of old and new bytes from appearing to be a valid measurement.

During debugging, a program with a retry limit could still retry indefinitely. The consecutive-failure counter was being cleared whenever initialization succeeded. If initialization always completed but sampling always failed, the counter repeatedly returned to zero and never reached its limit.

The revised logic clears the counter only after receiving a complete, valid sample. Successful initialization allows sampling to begin; valid data establishes that recovery has actually completed.

This project made me define success more carefully: **After an intermediate step succeeds, the program still needs to check whether the final goal has been achieved.**

[Explore Project 36](code/36_nonblocking_i2c_sensor_registers) · [IMU task code](code/36_nonblocking_i2c_sensor_registers/src/imu.c)

### Project 40: Organizing existing modules into a complete operating sequence

**Question: Once motion, sensing, displays, and communication are combined, how should the system start, finish, and recover?**

Project 40 adds an automatic round-trip sequence. The system first homes to establish a position reference, then accepts points A and B and a cycle count. It moves to A first, after which each A → B → A trip counts as one complete cycle. The system also reports status, progress, and the reason a run ended, and coordinates automatic tasks with manual commands.

One debugging case involved limit events during homing.

When a limit is triggered, an interrupt routine first stops the pulses and records the event for the main loop. If the homing logic has already reversed direction based on the switch level and started backing away, an event left over from the previous phase can still be processed on the next iteration, stopping the motor a second time.

Understanding this required putting the event time, current direction, and homing phase on the same timeline. The revised handler uses the current direction to filter the corresponding stale event while backing away during homing, while preserving protection when moving toward a limit.

This project drew my attention to sequencing between modules: **The state when an event occurs may differ from the state when it is processed.**

[Explore Project 40](code/40_mega_integrated_control_system) · [Safety handling code](code/40_mega_integrated_control_system/src/safety.c)

## Two turning points from my learning notes

I kept records of the early exercises in Obsidian. Two experiences helped me connect C language concepts to the behavior of a running program.

**Widening a variable requires checking the entire calculation.**

In Project 17, I changed the system millisecond counter from 16 bits to 32 bits, but larger numbers still appeared incorrectly in serial output. Further investigation showed that an intermediate variable in the decimal conversion was still 16 bits. Fixing this required checking the types used for the source data, calculations, and output conversion.

**A partially received command must survive a function return.**

In Project 21, I rewrote the timer, motor, and serial-related code by hand. The command receiver processed one character at a time, but I initially declared its buffer and index as ordinary local variables, causing them to be reinitialized on the next call. This prompted me to revisit scope, storage duration, and `static`, then apply that understanding to the receiver.

These notes preserve my questions, explanations, and revisions at the time. I did not maintain equally complete notes for the later projects; this repository primarily preserves the corresponding stages of code.

## My work and the help I used

My learning process begins with searching for and reading resources, followed by attempting a first version of the code myself. For some exercises, I retype existing modules to check whether I still understand the register configuration and program structure.

GPT helped break down the learning roadmap, explain concepts, review code, and discuss debugging. After writing a draft, I ask it to look for problems. When I do not understand a proposed change, I ask why and return to the documentation and code to check the explanation. The code in this repository includes revisions made after those reviews and discussions.

Finding resources independently, understanding principles, organizing programs, and explaining errors are skills I aim to keep developing through these exercises.

## Code structure

```text
code/
├── 01_bare_metal_blink/
│   ├── platformio.ini
│   └── src/
├── ... exercises from each stage
├── 40_mega_integrated_control_system/
│   ├── platformio.ini
│   ├── src/
│   └── include/
└── lib/
    ├── adc/
    ├── i2c/
    ├── spi/
    ├── timer/
    ├── usart/
    └── ... shared modules
```

- **`src/`**: Main programs and module implementations.
- **`include/`**: Module interfaces, types, and configuration declarations.
- **`platformio.ini`**: PlatformIO configuration for each project.
- **`lib/`**: Peripheral drivers and utility modules shared by later projects.

Early projects concentrate most of their logic in a single `main.c`. Later projects gradually separate interfaces, drivers, tasks, and reporting. Each directory retains the approach used at that learning stage, so there is repeated code and variation in organization.

## Current boundaries and next steps

Stepper position is primarily based on the number of pulses sent, and servo angles are mapped to target pulse widths. These values must be distinguished from feedback measuring actual mechanical position. The behavior of limits, emergency stops, communication timeouts, and recovery logic also depends on the specific hardware, wiring, and parameters.

After these exercises, I also began exploring STM32 and am still at an early stage. Next, I hope to strengthen my foundations in C, circuits, and real-time systems, study ECE 4760's sound, graphics, and feedback-control material more deeply, and apply that learning to larger projects of my own.

## References

- [Cornell ECE 4760/5730: Course, laboratories, and technical resources](https://ece4760.github.io/)
- [Cornell ECE 4760: Student open-ended design projects](https://ece4760.github.io/Projects/Projects.html)
- ATmega2560 datasheet and the datasheets for the peripherals used
