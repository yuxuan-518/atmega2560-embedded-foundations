# Embedded Foundations: From GPIO to an Integrated ATmega2560 System

**English** | [简体中文](README.zh-CN.md)

This repository documents my study of register-level AVR C programming on the ATmega2560. The exercises begin with LEDs and button inputs, then progress through timers, interrupts, serial communication, sensors, displays, and actuator control. The later projects bring these modules together into an integrated system operated through serial commands.

**Platform:** Mega 2560-compatible development board · ATmega2560 · AVR C · PlatformIO

## Project overview

[**View all 40 project cards in the PDF viewer →**](docs/project-cards/ATmega2560_Project_Cards_English.pdf)

Open the overview and scroll through the project cards in GitHub's PDF viewer.

[Read the text version](docs/project-cards/project-cards.md)

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
