#pragma once
#include "quantum.h"


#ifdef __cplusplus
extern "C" {
#endif

// Define the maximum number of breathing channels
#define MAX_BREATHING_CHANNELS 4

// Define the PWM output mode if not already defined
#ifndef PWM_OUTPUT_ACTIVE_HL
#    define PWM_OUTPUT_ACTIVE_HL PWM_OUTPUT_ACTIVE_LOW
#endif

// Define the number of steps in the breathing curve
#define BREATHING_STEPS 256

// Default breathing pattern.
/* To generate basic sin % breathing curve in python:
 * from math import sin, pi; breathing_steps = 255; print([int(sin(x/breathing_steps*pi)**4*100) for x in range(breathing_steps)])
 */
static const uint16_t breathing_table_sin[BREATHING_STEPS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 14, 17, 19, 21, 24, 27, 30, 33, 37, 41, 45, 50, 54, 59, 65, 70, 76, 83, 89, 96, 103, 111, 119, 127, 136, 145, 154, 164, 174, 184, 195, 205, 217, 228, 240, 253, 265, 278, 291, 304, 318, 332, 346, 361, 375, 390, 405, 420, 436, 451, 467, 482, 498, 514, 530, 546, 562, 578, 594, 610, 626, 642, 657, 673, 688, 704, 719, 734, 748, 763, 777, 791, 804, 818, 831, 843, 855, 867, 879, 889, 900, 910, 920, 929, 937, 945, 953, 960, 966, 972, 978, 983, 987, 990, 993, 996, 998, 999, 999, 999, 999, 998, 996, 993, 990, 987, 983, 978, 972, 966, 960, 953, 945, 937, 929, 920, 910, 900, 889, 879, 867, 855, 843, 831, 818, 804, 791, 777, 763, 748, 734, 719, 704, 688, 673, 657, 642, 626, 610, 594, 578, 562, 546, 530, 514, 498, 482, 467, 451, 436, 420, 405, 390, 375, 361, 346, 332, 318, 304, 291, 278, 265, 253, 240, 228, 217, 205, 195, 184, 174, 164, 154, 145, 136, 127, 119, 111, 103, 96, 89, 83, 76, 70, 65, 59, 54, 50, 45, 41, 37, 33, 30, 27, 24, 21, 19, 17, 14, 13, 11, 9, 8, 7, 6, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// Type definition for PWM channel
typedef uint8_t pwmchannel_t;

// Structure to hold the state for each breathing channel
typedef struct {
    bool active; // Whether the breathing effect is active on this channel
    int8_t period; // Breathing period for this channel in seconds
    uint16_t counter; // Counter for the breathing effect
    const uint16_t *step_table; // Pointer to the step table
} breathing_channel_state_t;

// Function declarations
void breathing_init(void);
void start_breathing_with_pattern(uint8_t channel, int8_t period, const uint16_t step_table[BREATHING_STEPS]);
void start_breathing(uint8_t channel, int8_t period);
void stop_breathing(uint8_t channel);
bool is_breathing(void);
void breathing_enable(void);
void breathing_disable(void);

#ifdef __cplusplus
}
#endif
