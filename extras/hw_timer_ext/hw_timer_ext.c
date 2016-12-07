/* ========================================================================== *
 *                               Driver Events                                *
 *               Handles Onewire & WS2812 events, from hw_timer.              *
 * -------------------------------------------------------------------------- *
 *         Copyright (C) Jeff Hufford - All Rights Reserved. License:         *
 *                   "THE BEER-WARE LICENSE" (Revision 42):                   *
 * Jeff Hufford (jeffrey92<at>gmail.com) wrote this file. As long as you      *
 * retain this notice you can do whatever you want with this stuff. If we     *
 * meet some day, and you think this stuff is worth it, you can buy me a beer *
 * in return.                                                                 *
 * ========================================================================== */
#include "user_interface.h"
#include "c_types.h"
#include "mem.h"
#include "gpio.h"
#include "osapi.h"
#include "user_config.h"

#include "../app_common/libc/c_stdio.h"

#include "driver/hw_timer.h"
#include "driver/onewire_nb.h"
#include "driver/ws2812.h"
#include "driver/driver_event.h"


os_event_t DRIVER_TASK_QUEUE[DRIVER_TASK_QUEUE_SIZE];

static uint32_t * hw_timer_arg_ptr = NULL;
static ws2812_driver_t * ws_driver = NULL;

static void Driver_Event_Task(os_event_t *e)
{
   if ( !e )
      return;

#if defined(EN_TEMP_SENSOR) && (ONEWIRE_NONBLOCKING==1)
   if ( e->sig == DRIVER_SRC_ONEWIRE)
   {
      onewire_driver_t * one_driver = (onewire_driver_t *)e->par;

      if (one_driver->seq_state == OW_SEQ_STATE_INIT)
      {
         one_driver->seq_state = OW_SEQ_STATE_TRANSITION;

         /* Assign first op (always 0) */
         one_driver->op_type = one_driver->seq_arr[0];
         /* Start with first valid operation (always 1) */
         one_driver->cur_op = 0;
         one_driver->seq_pos = 0;

         OW_DBG("1w seq init: type=%hd, op=%hd\n", one_driver->op_type, one_driver->cur_op);
      }

      OW_doit();
      return;
   }
   else
#endif

#ifdef EN_WS8212_HPSI
   if ( e->sig == DRIVER_SRC_WS2812)
   {
      ws2812_doit();
      return;
   }
#endif
   // unknown message type.
   NODE_DBG("Unkown hw msg type!\n");
}

/* Each time the hw timer fires, advance state machine */
static void ICACHE_RAM_ATTR hw_timer_cb(uint32_t arg)
{
   /* Valid args:
    *    - DRIVER_SRC_ONEWIRE (1)
    *    - DRIVER_SRC_WS2812  (2)
   */
   // system_os_post(EVENT_MON_TASK_PRIO, (os_signal_t) arg, (os_param_t)ws_driver);
   system_os_post(DRIVER_TASK_PRIO, (os_signal_t) DRIVER_SRC_WS2812, (os_param_t)ws_driver);
}

void driver_event_register_ws(ws2812_driver_t * ws)
{
   ws_driver = ws;
}


void driver_event_init()
{
   /* Setup System Task for Events */
   system_os_task(Driver_Event_Task, DRIVER_TASK_PRIO, DRIVER_TASK_QUEUE, DRIVER_TASK_QUEUE_SIZE);

   /* Init hw timer */
   hw_timer_init(FRC1_SOURCE, 0); // disable autoload
   hw_timer_arg_ptr = hw_timer_set_func(hw_timer_cb);
}
