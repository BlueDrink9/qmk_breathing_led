#pragma once
#include "quantum.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifndef BACKLIGHT_LIMIT_VAL
#    define BACKLIGHT_LIMIT_VAL 255
#endif

// Define the maximum number of breathing channels
#define MAX_BREATHING_CHANNELS 4

// Define the PWM output mode if not already defined
#ifndef PWM_OUTPUT_ACTIVE_HL
#    define PWM_OUTPUT_ACTIVE_HL PWM_OUTPUT_ACTIVE_LOW
#endif

// Define the number of steps in the breathing curve
#define BREATHING_STEPS 128

// Type definition for PWM channel
typedef uint8_t pwmchannel_t;

// Structure to hold the state for each breathing channel
typedef struct {
    bool active; // Whether the breathing effect is active on this channel
    int8_t period; // Breathing period for this channel
    uint16_t counter; // Counter for the breathing effect
} breathing_channel_state_t;

// Function declarations
void breathing_init(void);
void start_breathing(uint8_t channel, int8_t period);
void stop_breathing(uint8_t channel);
bool is_breathing(void);
void breathing_enable(void);
void breathing_disable(void);

#ifdef __cplusplus
}
#endif
