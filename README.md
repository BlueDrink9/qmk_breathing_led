# Breathing LED Control

## Overview

This module provides control over LEDs to create a "breathing" effect, where the LED intensity cycles in a pattern that resembles a breathing rhythm.
This is based off the same effect for the `backlight` feature, but adapted to multiple separate PWM pins (so you can have multiple LEDs breathing at different rates, for example). I use this to express a bigger range of layers with only 2 LEDs.

This is set up for STM32 PWM only.

## Features

- Control the breathing effect on multiple LED channels.
- Configure the breathing period (how fast it breathes) for each channel independently.
- Enable or disable the breathing effect dynamically.

This is provided as-is, you'll probably need to modify it for your
specific setup. Just putting it out in case it's useful.
Currently it assumes all the pins you are using are on the same timer

## Usage

### Installation
Clone this repo into your keymap folder, and add the following to
`rules.mk`

```make
SRC += breathing/breathing.c
```

Then add this to `keymap.c`

```c
#include "breathing/breathing.h"
```

### Initialization

Before using the breathing control functions, initialize the system with:

```c
void keyboard_pre_init_user(void) {
    breathing_init();
}
```

### Starting the Breathing Effect

You need to know the channels for the pins you use. These will noted in the
datasheet for your MCU; for example for the bluepill, A2 and A3 are timer
2 channels 2 and 3.

To start the breathing effect on a specific channel with a specified period, use:

```c
void start_breathing(uint8_t channel, int8_t period);
```

- `channel`: The LED channel to start the breathing effect on. Specific to
  each pin (assuming they use the same timer).
- `period`: The period of the breathing cycle in seconds, i.e., how long it takes to
  breathe a full cycle.

### Stopping the Breathing Effect

To stop the breathing effect on a specific channel, use:

```c
void stop_breathing(uint8_t channel);
```

- `channel`: The LED channel to stop the breathing effect on.

### Checking Breathing Status

To check if any channel is currently running a breathing effect, use:

```c
bool is_breathing(void);
```

### Custom breathing pattern

By default, this uses a sin pattern.
To generate basic sin % breathing curve in python:

```python
from math import sin, pi
breathing_steps = 255
print([int(sin(x/breathing_steps*pi)**4*100) for x in range(breathing_steps)])
```

To pass custom ones:

```c
void start_breathing_with_pattern(uint8_t channel, int8_t period, const uint16_t *step_table);
```

Where `step_table` is a pointer to a declared array of BREATHING_STEPS percentage values
(out of 1000, where 100% = 1000 and 0.5% = 5).

## Configuration

Define these in `config.h`

- `BREATHING_PINS`: Define this macro with the array of pins used for breathing LEDs, e.g. `#define BREATHING_PINS { A2, A3 }; 
- `N_BREATHING_PINS`: Size of array above.
- `BREATHING_LED_PWM`: Name of the timer you are using. E.g. `#define BREATHING_LED_PWM &PWMD2`
- `PWM_OUTPUT_ACTIVE_HL`: 1 if your LED is on when voltage is applied,
  0 if it is on when driven low.
<!-- - `MAX_BREATHING_CHANNELS`: The maximum number of channels that can be controlled. Defaults to 4, which is probably the number of channels that timer has anyway.  -->
<!-- - `BREATHING_STEPS`: The number of steps in the breathing curve. -->

Define these in `halconf.h`

```c
#define HAL_USE_PWM TRUE

#include_next <halconf.h>
```

You probably also need to ensure the timer for your pins is active. For
bluepill, A3 is timer 2 so I need to enable that. However, timer 2 is the
default for other functions on the board, so I need to first change the main timer to 3, to free up timer 2 (and pin A3) for the LED PWM.

```c
#undef STM32_ST_USE_TIMER
#define STM32_ST_USE_TIMER 3
#undef STM32_PWM_USE_TIM2
#define STM32_PWM_USE_TIM2 TRUE
```
