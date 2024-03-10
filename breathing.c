#include "breathing.h"

static pin_t breathing_pins[] = BREATHING_PINS;

breathing_channel_state_t breathing_channels[MAX_BREATHING_CHANNELS];

// Init LEDs.
static PWMConfig pwmCFG = {
    0xFFFF,/* PWM clock frequency  */
    256,/* initial PWM period (in ticks) 1S (1/10kHz=0.1mS 0.1ms*10000 ticks=1S) */
    NULL,
    {
        /* Channels 0 to 3 */
        {PWM_OUTPUT_ACTIVE_HL, NULL},
        {PWM_OUTPUT_ACTIVE_HL, NULL},
        {PWM_OUTPUT_ACTIVE_HL, NULL},
        {PWM_OUTPUT_ACTIVE_HL, NULL},
    },
    0, /* HW dependent part.*/
    0
};

static virtual_timer_t breathing_vt;

void breathing_init(void) {
    pwmStart(BREATHING_LED_PWM, &pwmCFG);
    for (int i = 0; i < MAX_BREATHING_CHANNELS; i++) {
        pwmDisableChannel(BREATHING_LED_PWM, i);
        breathing_channels[i].active = false;
        breathing_channels[i].period = 6;
        breathing_channels[i].counter = 0;
    }
    // Init pins
    for (int i = 0; i < N_BREATHING_PINS; i++) {
        palSetPadMode(PAL_PORT(breathing_pins[i]), PAL_PAD(breathing_pins[i]), PAL_MODE_ALTERNATE_PUSHPULL);
    }
    chVTObjectInit(&breathing_vt);
}

 void start_breathing(uint8_t channel, int8_t period) {
     if (channel >= MAX_BREATHING_CHANNELS) {
         return; // Invalid channel
     }
     breathing_channels[channel].active = true;
     breathing_channels[channel].period = period;
     breathing_channels[channel].counter = 0;

     // If the timer is not already running, start it
     if (!is_breathing()) {
         breathing_enable();
     }
 }

 void stop_breathing(uint8_t channel) {
     if (channel >= MAX_BREATHING_CHANNELS) {
         return; // Invalid channel
     }
     breathing_channels[channel].active = false;

     // Check if all channels are inactive, and if so, stop the timer
     bool any_active = false;
     for (int i = 0; i < MAX_BREATHING_CHANNELS; i++) {
         if (breathing_channels[i].active) {
             any_active = true;
             break;
         }
     }
     if (!any_active) {
         breathing_disable();
     }
 }

// See http://jared.geek.nz/2013/feb/linear-led-pwm
static uint16_t cie_lightness(uint16_t v) {
    if (v <= 5243)    // if below 8% of max
        return v / 9; // same as dividing by 900%
    else {
        uint32_t y = (((uint32_t)v + 10486) << 8) / (10486 + 0xFFFFUL); // add 16% of max and compare
        // to get a useful result with integer division, we shift left in the expression above
        // and revert what we've done again after squaring.
        y = y * y * y >> 8;
        if (y > 0xFFFFUL) { // prevent overflow
            return 0xFFFFU;
        } else {
            return (uint16_t)y;
        }
    }
}

static uint32_t rescale_limit_val(uint32_t val) {
    // rescale the supplied backlight value to be in terms of the value limit
    return (val * (BACKLIGHT_LIMIT_VAL + 1)) / 256;
}

/* To generate breathing curve in python:
 * from math import sin, pi; [int(sin(x/128.0*pi)**4*255) for x in range(128)]
 */
static const uint8_t breathing_table[BREATHING_STEPS] = {1,1,1,1,1,1,1,1,1,1,1, 1, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 17, 20, 24, 28, 32, 36, 41, 46, 51, 57, 63, 70, 76, 83, 91, 98, 106, 113, 121, 129, 138, 146, 154, 162, 170, 178, 185, 193, 200, 207, 213, 220, 225, 231, 235, 240, 244, 247, 250, 252, 253, 254, 255, 254, 253, 252, 250, 247, 244, 240, 235, 231, 225, 220, 213, 207, 200, 193, 185, 178, 170, 162, 154, 146, 138, 129, 121, 113, 106, 98, 91, 83, 76, 70, 63, 57, 51, 46, 41, 36, 32, 28, 24, 20, 17, 15, 12, 10, 8, 6, 5, 4, 3, 2, 1, 1,1,1,1,1,1,1,1,1,1, 1};

static void breathing_callback(virtual_timer_t *vtp, void *p);

bool is_breathing(void) {
    return chVTIsArmed(&breathing_vt);
}

void breathing_enable(void) {
    /* Update frequency is 256Hz -> 3906us intervals */
    chVTSetContinuous(&breathing_vt, TIME_US2I(3906), breathing_callback, NULL);
}

void breathing_disable(void) {
    chVTReset(&breathing_vt);

    // Restore backlight level
    // backlight_set(get_backlight_level());
}

// Use this before the cie_lightness function.
static inline uint16_t scale_backlight(uint16_t v) {
    return v;
    // return v / BACKLIGHT_LEVELS * get_backlight_level();
}

static void breathing_callback(virtual_timer_t *vtp, void *p) {
     for (uint8_t channel = 0; channel < MAX_BREATHING_CHANNELS; channel++) {
         if (!breathing_channels[channel].active) {
             continue; // Skip inactive channels
         }

         int8_t breathing_period = breathing_channels[channel].period;
         uint16_t interval = (uint16_t)breathing_period * 256 / BREATHING_STEPS;

         // Update the counter for this channel
         breathing_channels[channel].counter = (breathing_channels[channel].counter + 1) % (breathing_period * 256);
         uint8_t index = breathing_channels[channel].counter / interval % BREATHING_STEPS;
         uint32_t duty = cie_lightness(rescale_limit_val(scale_backlight(breathing_table[index] * 256)));

         chSysLockFromISR();
         pwmEnableChannelI(BREATHING_LED_PWM, channel, PWM_FRACTION_TO_WIDTH(BREATHING_LED_PWM, 0xFFFF, duty));
         chSysUnlockFromISR();
     }
 }
