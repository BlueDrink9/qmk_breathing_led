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
    start_breathing_with_pattern(channel, period, breathing_table_sin);
}

void start_breathing_with_pattern(uint8_t channel, int8_t period, const uint16_t step_table[BREATHING_STEPS]) {
    if (channel >= MAX_BREATHING_CHANNELS) {
        return; // Invalid channel
    }
    breathing_channels[channel].active = true;
    breathing_channels[channel].period = period;
    breathing_channels[channel].step_table = step_table;
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
        uint32_t percent = 10 * cie_lightness_correction(breathing_channels[channel].step_table[index]);

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

void pwm_on(uint8_t channel){
    pwmEnableChannelI(BREATHING_LED_PWM, channel, PWM_PERCENTAGE_TO_WIDTH(BREATHING_LED_PWM, 10000));
}

void pwm_set(uint8_t channel, uint8_t percent){
    pwmEnableChannelI(BREATHING_LED_PWM, channel, PWM_PERCENTAGE_TO_WIDTH(BREATHING_LED_PWM, percent));
}


void pwm_off(uint8_t channel){
    pwmEnableChannelI(BREATHING_LED_PWM, channel, PWM_PERCENTAGE_TO_WIDTH(BREATHING_LED_PWM, 0));
    stop_breathing(channel);
}
