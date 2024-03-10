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

### Other convenience functions
```c
// Turn on 100%
void pwm_on(uint8_t channel);
// Turn off
void pwm_off(uint8_t channel);
// Turn to specified percentage (out of 10000)
void pwm_set(uint8_t channel, uint8_t percent);
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
static const uint16_t step_table_sin[BREATHING_STEPS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 14, 17, 19, 21, 24, 27, 30, 33, 37, 41, 45, 50, 54, 59, 65, 70, 76, 83, 89, 96, 103, 111, 119, 127, 136, 145, 154, 164, 174, 184, 195, 205, 217, 228, 240, 253, 265, 278, 291, 304, 318, 332, 346, 361, 375, 390, 405, 420, 436, 451, 467, 482, 498, 514, 530, 546, 562, 578, 594, 610, 626, 642, 657, 673, 688, 704, 719, 734, 748, 763, 777, 791, 804, 818, 831, 843, 855, 867, 879, 889, 900, 910, 920, 929, 937, 945, 953, 960, 966, 972, 978, 983, 987, 990, 993, 996, 998, 999, 999, 999, 999, 998, 996, 993, 990, 987, 983, 978, 972, 966, 960, 953, 945, 937, 929, 920, 910, 900, 889, 879, 867, 855, 843, 831, 818, 804, 791, 777, 763, 748, 734, 719, 704, 688, 673, 657, 642, 626, 610, 594, 578, 562, 546, 530, 514, 498, 482, 467, 451, 436, 420, 405, 390, 375, 361, 346, 332, 318, 304, 291, 278, 265, 253, 240, 228, 217, 205, 195, 184, 174, 164, 154, 145, 136, 127, 119, 111, 103, 96, 89, 83, 76, 70, 65, 59, 54, 50, 45, 41, 37, 33, 30, 27, 24, 21, 19, 17, 14, 13, 11, 9, 8, 7, 6, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
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
