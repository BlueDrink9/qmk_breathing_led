#include "breathing.h"
#include "cie1931.h"

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
    // Turn LED off
    pwmEnableChannelI(BREATHING_LED_PWM, channel, PWM_PERCENTAGE_TO_WIDTH(BREATHING_LED_PWM, 0));

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

// See
// https://web.archive.org/web/20230906171704/https://jared.geek.nz/2013/feb/linear-led-pwm
// Set at input and output level 1000, representing 100.0 percent.
static uint16_t cie_lightness_correction(uint16_t v) {
    return cie[v];
}

/* To generate basic sin % breathing curve in python:
 * from math import sin, pi; breathing_steps = 255; print([int(sin(x/breathing_steps*pi)**4*100) for x in range(breathing_steps)])
 */
static const uint16_t breathing_table[BREATHING_STEPS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13, 14, 17, 19, 21, 24, 27, 30, 33, 37, 41, 45, 50, 54, 59, 65, 70, 76, 83, 89, 96, 103, 111, 119, 127, 136, 145, 154, 164, 174, 184, 195, 205, 217, 228, 240, 253, 265, 278, 291, 304, 318, 332, 346, 361, 375, 390, 405, 420, 436, 451, 467, 482, 498, 514, 530, 546, 562, 578, 594, 610, 626, 642, 657, 673, 688, 704, 719, 734, 748, 763, 777, 791, 804, 818, 831, 843, 855, 867, 879, 889, 900, 910, 920, 929, 937, 945, 953, 960, 966, 972, 978, 983, 987, 990, 993, 996, 998, 999, 999, 999, 999, 998, 996, 993, 990, 987, 983, 978, 972, 966, 960, 953, 945, 937, 929, 920, 910, 900, 889, 879, 867, 855, 843, 831, 818, 804, 791, 777, 763, 748, 734, 719, 704, 688, 673, 657, 642, 626, 610, 594, 578, 562, 546, 530, 514, 498, 482, 467, 451, 436, 420, 405, 390, 375, 361, 346, 332, 318, 304, 291, 278, 265, 253, 240, 228, 217, 205, 195, 184, 174, 164, 154, 145, 136, 127, 119, 111, 103, 96, 89, 83, 76, 70, 65, 59, 54, 50, 45, 41, 37, 33, 30, 27, 24, 21, 19, 17, 14, 13, 11, 9, 8, 7, 6, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static void breathing_callback(virtual_timer_t *vtp, void *p);

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
        // Percent is set with 100% = 10000. We are using a resolution
        // of 1000 for cie correction, so * 10 to get to actual
        // percent.
        uint32_t percent = 10 * cie_lightness_correction(breathing_table[index]);

        if (percent == 0){
            // Keep a tiny amount of power in to prevent abrupt blink
            // off.
            percent = 1;
        }
        chSysLockFromISR();
        pwmEnableChannelI(BREATHING_LED_PWM, channel, PWM_PERCENTAGE_TO_WIDTH(BREATHING_LED_PWM, percent * 10));
        chSysUnlockFromISR();
    }
}

bool is_breathing(void) {
    return chVTIsArmed(&breathing_vt);
}

void breathing_enable(void) {
    /* Update frequency is 256Hz -> 3906us intervals */
    chVTSetContinuous(&breathing_vt, TIME_US2I(3906), breathing_callback, NULL);
}

void breathing_disable(void) {
    chVTReset(&breathing_vt);
}
