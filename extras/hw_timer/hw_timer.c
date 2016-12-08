/* Implementation of PWM support for the Espressif SDK.
 *
 * Part of esp-open-rtos
 * Copyright (C) 2015 Guillem Pascual Ginovart (https://github.com/gpascualg)
 * Copyright (C) 2015 Javier Cardona (https://github.com/jcard0na)
 * BSD Licensed as described in the file LICENSE
 */
#include <espressif/esp_common.h>
#include <espressif/sdk_private.h>
#include <FreeRTOS.h>
#include <esp8266.h>

#include "ws2812.h"
#include "onewire.h"
#include "hw_timer.h"

#define HW_TIMER_DEBUG 0
#define HW_TIMER_AUTOLOAD 0

/* Uncoment to use clock div16 */
// #define CLK_DIV_SET_1

#define DIV1_LOADVAL  700
#define DIV16_LOADVAL 44

/* Espressif-provided macro to get ticks from us */
#define US_TO_RTC_TIMER_TICKS(t)    \
   ((t) ?                           \
   (((t) > 0x35A) ?                 \
   (((t)>>2) * ((APB_CLK_FREQ>>4)/250000) + ((t)&0x3) * ((APB_CLK_FREQ>>4)/1000000)) : \
   (((t) *(APB_CLK_FREQ>>4)) / 1000000)) : \
   0)

/* Keeps track of the current state of the HW timer. The wrapper methods      *
 * available through hw_timer.h use this to ensure everything is okay before  *
 * arming/disarming/changing callbacks. Otherwise we'd crash most likely.     */
static HW_TIMER_STATE_T hw_timer_state = HW_TIMER_DISABLED;

// static int next_call = DOIT_WS2812;
static int next_call = DOIT_ONEWIRE;

static uint32_t testcout = 0;
static uint32_t loadval = 0;

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

static ow_hw_t ow_hw;

static void IRAM frc1_interrupt_handler(void)
{
#ifndef CLK_DIV_SET_1
   if (next_call == DOIT_ONEWIRE)
   {
      OW_doit();
   }
   else
   {
      ws2812_doit();
   }
   timer_set_load(FRC1, DIV16_LOADVAL);
#else
   timer_set_load(FRC1, DIV1_LOADVAL);
   if (next_call == DOIT_ONEWIRE)
   {
      OW_doit();
   }
   else
   {
      ws2812_doit();
   }
#endif
}

/*******************************************************************************
 * Externally accessible wrapper methods
*******************************************************************************/
HW_TIMER_STATE_T hw_timer_get_state(void)
{
   return hw_timer_state;
}

void hw_timer_init(void)
{
    /* Initialize */
    ow_hw._maxLoad = 0;
    ow_hw._onLoad = 0;
    ow_hw._offLoad = 0;
    ow_hw._step = OW_HW_OFF;

    /* Stop timers and mask interrupts */
   //  hw_timer_stop();

    /* set up ISRs */
    _xt_isr_attach(INUM_TIMER_FRC1, frc1_interrupt_handler);

    /* Flag not running */
    ow_hw.running = 0;

    hw_timer_state = HW_TIMER_READY;
}

void hw_timer_restart(void)
{
    if (ow_hw.running)
    {
        hw_timer_stop();
        hw_timer_start();
    }
}

void hw_timer_start(void)
{
   if (hw_timer_state == HW_TIMER_ACTIVE ) return;

#if HW_TIMER_AUTOLOAD!=0
   loadval = timer_time_to_count(FRC1, 20, TIMER_CLKDIV_16);
   // loadval = US_TO_RTC_TIMER_TICKS(10UL);
#else
   #ifndef CLK_DIV_SET_1
   loadval = DIV16_LOADVAL;
   timer_set_divider(FRC1, TIMER_CLKDIV_16);
   #else
   loadval = DIV1_LOADVAL;
   timer_set_divider(FRC1, TIMER_CLKDIV_1);
   #endif
#endif

#if HW_TIMER_DEBUG==1
   printf("loadval = %u\n", loadval);
#endif



   timer_set_reload(FRC1, (bool)HW_TIMER_AUTOLOAD);
   timer_set_interrupts(FRC1, true);

   timer_set_load(FRC1, loadval);
   timer_set_run(FRC1, true);

   hw_timer_state = HW_TIMER_ACTIVE;
}

void hw_timer_stop(void)
{
    timer_set_interrupts(FRC1, false);
    timer_set_run(FRC1, false);
    ow_hw.running = 0;

    ow_hw.running = HW_TIMER_STOPPED;
#if HW_TIMER_DEBUG==1
    printf("hw timer stopped\n");
#endif
}
