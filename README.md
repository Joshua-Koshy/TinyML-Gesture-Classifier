# TinyML Embedded Gesture Classifier

A real-time embedded gesture-recognition system built around TM4C123 microcontrollers, LSM6DSOX inertial sensors, ESP8266 wireless links, and fixed-point TinyML inference. The project captures motion from handheld controller modules, segments raw IMU data into gesture windows, extracts engineered motion features, runs lightweight gesture classifiers on-device, and streams classification results to a central hub and host display.

This repository is structured as a complete embedded system rather than a standalone ML notebook. It includes firmware for multiple TM4C-based modules, wireless communication support, sensor-processing code, fixed-point neural-network inference, timing/synchronization logic, host-side visualization, and KiCad hardware design files.

> Large `.mp4` demo/media files are intentionally excluded from version control. The firmware and classifier logic do not depend on storing those video files in Git.

---

## Project Overview

The system is designed to classify human arm and body gestures using constrained embedded hardware. Each controller samples a 6-axis LSM6DSOX IMU, filters and calibrates the sensor readings, detects active motion segments, computes a compact feature vector, and classifies the movement using a mixture of rule-based scoring and fixed-point TinyML models.

A central TM4C-based hub coordinates system timing, receives controller outputs through ESP8266 communication, and forwards gesture events to a PC over UART. The host-side Python application can display real-time score/state feedback, but the core gesture recognition pipeline runs on the embedded controller firmware.

---

## Key Features

- **Real-time IMU gesture classification** on TM4C123 microcontrollers
- **LSM6DSOX accelerometer/gyroscope integration** over I2C
- **Deterministic timing** using a free-running hardware timer
- **Five-second calibration stage** for sensor bias and baseline estimation
- **Motion segmentation engine** with tunable per-gesture thresholds
- **Feature extraction from raw motion segments** using 32 engineered gesture metrics
- **Fixed-point TinyML inference** with small neural networks compiled into C arrays
- **Rule-based fallback scoring** for gestures that are easier to detect analytically
- **ESP8266 wireless communication** between controller modules and the hub
- **UART event protocol** for forwarding detections to external tools
- **EEPROM-backed mode/song selection** so the selected routine persists across reset
- **Multi-module architecture** with separate controller, hub/audio, and host display software
- **KiCad hardware files** for embedded board design and supporting circuitry

---

## System Architecture

```text
+--------------------------+         ESP8266 wireless         +---------------------------+
| Controller Module 1      |  ----------------------------->  |                           |
| TM4C123 + LSM6DSOX IMU   |                                 |                           |
|                          |                                 |                           |
| - IMU sampling           |                                 |                           |
| - calibration            |                                 | Central Hub / Brain       |
| - filtering              |                                 | TM4C123 + ESP8266         |
| - segmentation           |                                 |                           |
| - feature extraction     |                                 | - start coordination      |
| - TinyML inference       |                                 | - audio / timing control  |
| - detection messages     |                                 | - UART bridge to PC       |
+--------------------------+                                 |                           |
                                                               |                           |
+--------------------------+         ESP8266 wireless         |                           |
| Controller Module 2      |  ----------------------------->  |                           |
| TM4C123 + LSM6DSOX IMU   |                                 +-------------+-------------+
|                          |                                               |
| Same embedded pipeline   |                                               | UART
+--------------------------+                                               v
                                                               +---------------------------+
                                                               | Host Display / Logger     |
                                                               | Python + serial interface |
                                                               +---------------------------+
```

The important part: gesture classification is not performed on the PC. The PC is mainly a visualization/logging endpoint. The controller firmware performs the sensor processing, feature extraction, and inference on embedded hardware.

---

## Repository Layout

```text
.
├── hw/
│   ├── ECE319K_Starter/
│   ├── ECE445L_Lab6/
│   ├── ECE445L_Lab7/
│   ├── ECE445L_RSLK_V2/
│   ├── ECE445L_Starter/
│   └── Part Libraries/
│
├── sw/
│   ├── sw_Player1_Controller/
│   │   ├── inc/
│   │   └── src/
│   │       └── Lab1.c
│   │
│   ├── sw_Player2_Controller/
│   │   └── sw/
│   │       ├── inc/
│   │       └── src/
│   │           └── Lab1.c
│   │
│   ├── sw_Audio/
│   │   └── sw/
│   │       ├── inc/
│   │       └── src_latest/
│   │           └── Lab5.c
│   │
│   └── sw_Display/
│       └── justdance.py
│
└── Resources/
    └── component datasheets and references
```

### Major software components

| Path | Purpose |
| --- | --- |
| `sw/sw_Player1_Controller/src/Lab1.c` | Main firmware for one IMU controller module |
| `sw/sw_Player2_Controller/sw/src/Lab1.c` | Main firmware for the second IMU controller module |
| `sw/sw_Audio/sw/src_latest/Lab5.c` | Central hub firmware for coordination, UART/ESP communication, and audio/timing control |
| `sw/sw_Display/justdance.py` | Python host UI/logger that reads serial output from the hub |
| `hw/` | KiCad board files, schematics, footprints, and hardware support files |
| `Resources/` | Datasheets and reference documents |

---

## Embedded Controller Pipeline

Each controller runs the same core gesture-recognition pipeline:

```text
IMU read
   ↓
axis remapping
   ↓
low-pass filtering
   ↓
stationary calibration / bias tracking
   ↓
active-motion segmentation
   ↓
segment feature extraction
   ↓
rule-based scoring or TinyML inference
   ↓
cumulative step scoring
   ↓
UART / ESP detection output
```

### 1. IMU sampling

The controller firmware communicates with the LSM6DSOX IMU over I2C. The firmware configures the accelerometer and gyroscope, reads raw accelerometer/gyro registers, and remaps sensor axes into the project’s real-world coordinate convention.

The firmware preserves this logical axis mapping:

```text
IRL X = sensor Y
IRL Y = sensor Z
IRL Z = sensor X
```

### 2. Calibration

At startup, the controller performs a fixed calibration stage. The calibration window is designed to last exactly 5000 ms, using hardware-timer-based scheduling instead of delay loops that could drift with UART or I2C activity.

Calibration is used to establish stable baseline values and reduce false positives from gravity, IMU bias, and small idle movement.

### 3. Real-time scheduling

The controller uses Timer0A as a free-running hardware timer. This matters because the gesture timeline should not depend on how long debug printing, I2C transactions, or classification math happens to take.

The firmware advances sampling based on timestamps rather than simply waiting in a loop. This makes the runtime behavior much more deterministic on bare-metal embedded hardware.

### 4. Motion segmentation

The segmentation engine detects when a meaningful gesture starts and ends. It uses energy thresholds, quiet-sample counts, minimum/maximum segment sizes, and cooldown timing.

Each gesture can override the default segmentation parameters through the move registry. This is important because a fast punch, slow arm sweep, spin-like motion, and small wrist gesture do not produce the same energy shape.

The move registry stores:

```text
move enum
human-readable move name
score function pointer
per-move segmentation tuning
```

This makes the system easier to extend: adding a gesture does not require rewriting the segmentation engine.

---

## Feature Extraction

For each detected motion segment, the firmware computes a compact feature vector. The code includes 32 segment-level features that describe timing, acceleration, gyroscope activity, travel direction, peak timing, oscillation behavior, and energy distribution.

Representative features include:

- segment duration
- number of samples
- net vertical rise
- total vertical travel
- ending height estimate
- starting low-position estimate
- cross-body horizontal movement
- small dip magnitude
- finish drop from peak
- oscillation count
- slope flip count
- total absolute acceleration
- total absolute gyro motion
- vertical acceleration area
- vertical gyro area
- peak acceleration
- normalized fractional versions of major features
- spin-like motion fraction
- energy per sample

These engineered features are useful because the TM4C123 does not have the memory or compute budget for heavyweight ML pipelines. Instead of feeding long time-series windows into a large model, the firmware compresses each gesture segment into a small fixed-size feature vector.

---

## TinyML Inference

Several gestures use embedded neural-network classifiers. The models are small, fixed-point networks compiled directly into C arrays.

The main pattern is:

```text
16 input features → 3 hidden ReLU units → 1 output logit
```

The firmware stores model parameters as scaled integers:

```c
#define MODEL_INPUTS 16
#define MODEL_HIDDEN 3
#define MODEL_SCALE 1000
```

The inference path uses integer arithmetic instead of runtime floating-point math. This is important for predictable performance on the TM4C123 and for avoiding unnecessary overhead in the real-time loop.

Supported TinyML-style classifiers in the controller firmware include models for gestures such as:

- arc motion
- bent arm sway
- right arm wave
- right arm throw
- tall scoop
- flex throw hands
- right arm flex

Some other gestures are detected using rule-based scoring when the motion has a simpler or more deterministic feature signature.

---

## Detection Output Protocol

The controller firmware emits machine-readable UART messages so external tools can parse results reliably.

### Segment feature dump

```text
SEG,<MOVE>,<32 comma-separated feature values>
```

This is useful for collecting training data, debugging feature quality, and comparing good/bad gesture examples.

### Final detection output

```text
DETECT,<MOVE>,<score>
```

This is the main classification output. The score is produced after per-segment scores are accumulated across a gesture step and mapped into a final output score.

The distinction matters:

- `SEG` = raw feature/debug/training data for one detected segment
- `DETECT` = final classified gesture result intended for the rest of the system

---

## Cumulative Step Scoring

Instead of printing every segment as a final result, the firmware can accumulate evidence across an expected gesture window. At the end of the window, cumulative evidence is mapped into a final score.

This helps reduce noisy detections because one accidental segment does not automatically become a final classification. The firmware can require enough accumulated evidence before emitting a `DETECT` line.

This design is especially useful when a gesture naturally breaks into multiple segments or contains repeated motion.

---

## Central Hub Firmware

The hub firmware coordinates the larger system. It uses another TM4C123 along with an ESP8266 communication path and UART output to the host PC.

Responsibilities include:

- receiving start/control messages
- coordinating controller start timing
- communicating with ESP8266 over UART5
- forwarding detection information to the host computer
- managing timing-sensitive output behavior
- supporting DAC/audio-related timing infrastructure

The hub acts as the bridge between embedded controller modules and the PC application.

---

## Host Display / Logger

The Python host program reads serial data from the hub and displays runtime feedback. It uses:

- `pyserial` for UART communication
- `tkinter` for the GUI
- `opencv-python` and `Pillow` for visual display support

The host program is not the classifier. It is a visualization and logging layer built on top of the embedded detection pipeline.

---

## Hardware

The project uses:

- TM4C123 microcontrollers
- LSM6DSOX 6-axis IMU modules
- ESP8266 wireless modules
- DAC/audio circuitry on the hub side
- custom/supporting KiCad hardware designs
- UART links for module-to-module and module-to-PC communication

The `hw/` directory contains KiCad project files, schematics, PCB layouts, footprints, symbols, and 3D models used during hardware design.

---

## Building the Firmware

This repository appears to be organized for embedded C development using TM4C/Tiva-style project files. The source folders include `.uvprojx`, `.ccsproject`, `.cproject`, startup files, linker command files, and TM4C register headers.

Typical workflow:

1. Open the relevant controller or hub project in the embedded IDE being used.
2. Build the project for the TM4C123 target.
3. Flash the firmware to the corresponding board.
4. Connect UART/ESP wiring according to the hardware setup.
5. Start the hub and controller modules.
6. Read `DETECT` and `SEG` messages through the host serial path.

Main firmware entry points:

```text
sw/sw_Player1_Controller/src/Lab1.c
sw/sw_Player2_Controller/sw/src/Lab1.c
sw/sw_Audio/sw/src_latest/Lab5.c
```

---

## Running the Host Program

Install Python dependencies:

```bash
pip install pyserial opencv-python pillow
```

Then update the serial port in the host script if needed:

```python
SERIAL_PORT = 'COM11'
BAUD_RATE = 115200
```

Run:

```bash
python sw/sw_Display/justdance.py
```

If using the display script without local media assets, remove or replace any references to large video files. Those files are intentionally not tracked in Git.

---

## Git / Large File Notes

Do not commit large media files to this repository. GitHub rejects normal Git files larger than 100 MB.

Recommended `.gitignore` additions:

```gitignore
# Large media/demo assets
*.mp4
*.mov
*.avi
*.mkv

# Python cache
__pycache__/
*.pyc

# Keil / embedded build outputs
Objects/
Listings/
*.axf
*.elf
*.hex
*.map
*.o
*.d

# Temporary files
*.tmp
~$*
```

If a large media file was committed accidentally, deleting it in a later commit is not enough. It must be removed from Git history before pushing.

---

## Why This Project Is Interesting

This project combines several embedded-systems problems into one system:

- real-time sensor acquisition
- noisy IMU signal processing
- motion segmentation
- feature engineering
- fixed-point machine learning
- bare-metal timing constraints
- wireless embedded communication
- multi-device synchronization
- host visualization
- custom hardware design

The most technically important part is the embedded ML pipeline: the system turns raw accelerometer/gyroscope data into motion segments, compresses each segment into meaningful features, and runs compact classifiers directly on the microcontroller.

That makes it a true TinyML-style embedded classification project rather than just a PC-side data demo.

---

## Future Improvements

Potential next steps:

- replace duplicated controller code with a shared controller firmware module
- split gesture models into separate header/source files
- add a formal training pipeline directory for dataset collection and model export
- add unit tests for feature extraction and fixed-point inference
- define a cleaner serial protocol document
- add model accuracy notes and confusion matrices
- add a minimal no-media host logger for easier GitHub demos
- add diagrams for hardware wiring and data flow
- add CI checks for formatting or static analysis

---

## Status

This repository represents an active embedded prototype with working firmware structure, controller-side gesture processing, TinyML inference code, wireless communication support, and host-side display/logging support. Some files still reflect development naming and classroom-lab origins, but the core system is best understood as a TinyML embedded gesture-classification platform.
