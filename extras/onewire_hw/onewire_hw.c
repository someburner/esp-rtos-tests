/* Implementation of PWM support for the Espressif SDK.
 *
 * Part of esp-open-rtos
 * Copyright (C) 2015 Guillem Pascual Ginovart (https://github.com/gpascualg)
 * Copyright (C) 2015 Javier Cardona (https://github.com/jcard0na)
 * BSD Licensed as described in the file LICENSE
 */
#include "onewire_hw.h"

#include <espressif/esp_common.h>
#include <espressif/sdk_private.h>
#include <FreeRTOS.h>
#include <esp8266.h>

typedef enum {
    OW_HW_OFF      = -1,
    OW_HW_NO_DELAY = 0,
    OW_HW_10_US    = 1,
    OW_HW_20_US    = 2,
    OW_HW_40_US    = 4,
    OW_HW_50_US    = 5,
    OW_HW_60_US    = 6,
    OW_HW_250_US   = 25,
    OW_HW_500_US   = 50
} ow_hw_step_t;

typedef struct ow_hw_def
{
    uint8_t running;

    uint16_t freq;
    uint16_t dutyCicle;

    /* private */
    uint32_t _maxLoad;
    uint32_t _onLoad;
    uint32_t _offLoad;
    ow_hw_step_t _step;

    uint16_t usedPins;
} ow_hw_t;

// static PWMInfo pwmInfo;
static ow_hw_t ow_hw;

static void frc1_interrupt_handler(void)
{
    uint32_t load = ow_hw._onLoad;
    ow_hw_step_t step = OW_HW_NO_DELAY;

    if (ow_hw._step != OW_HW_OFF)
    {
        load = ow_hw._offLoad;
        step = OW_HW_NO_DELAY;
    }

    timer_set_load(FRC1, load);
    ow_hw._step = step;
}

void ow_hw_init()
{
    /* Initialize */
    ow_hw._maxLoad = 0;
    ow_hw._onLoad = 0;
    ow_hw._offLoad = 0;
    ow_hw._step = OW_HW_OFF;

    /* Stop timers and mask interrupts */
    ow_hw_stop();

    /* set up ISRs */
    _xt_isr_attach(INUM_TIMER_FRC1, frc1_interrupt_handler);

    /* Flag not running */
    ow_hw.running = 0;
}

// void pwm_set_freq(uint16_t freq)
// {
//     ow_hw.freq = freq;
//
//     /* Stop now to avoid load being used */
//     if (ow_hw.running)
//     {
//         pwm_stop();
//         ow_hw.running = 1;
//     }
//
//     timer_set_frequency(FRC1, freq);
//     ow_hw._maxLoad = timer_get_load(FRC1);
//
//     if (ow_hw.running)
//     {
//         pwm_start();
//     }
// }

void ow_hw_restart()
{
    if (ow_hw.running)
    {
        ow_hw_stop();
        ow_hw_start();
    }
}

void ow_hw_start()
{
    timer_set_load(FRC1, ow_hw._onLoad);
    timer_set_reload(FRC1, false);
    timer_set_interrupts(FRC1, true);
    timer_set_run(FRC1, true);

    ow_hw.running = 1;
}

void ow_hw_stop()
{
    timer_set_interrupts(FRC1, false);
    timer_set_run(FRC1, false);
    ow_hw.running = 0;
}
