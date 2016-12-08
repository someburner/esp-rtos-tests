#include "onewire.h"
#include "string.h"
#include "task.h"
#include "esp/gpio.h"

#include "maxim28.h"

#define vTaskDelayMs(ms)	vTaskDelay((ms)/portTICK_PERIOD_MS)

const int ow_pin = TEMP_SENSOR_PIN;

/* use this to tell above timer which seq it was armed from */
static int ow_task_arg = DS_SEQ_INVALID;

/* Onewire driver struct declaration */
static onewire_driver_t onewire_driver;
static onewire_driver_t * one_driver = &onewire_driver;

/* Temperature data */
static Temperature latest_temp;

static TaskHandle_t ow_seq_int_task_handle = NULL;

static QueueHandle_t * pubTempQueueHandle = NULL;

static void ow_task(void *pxParameter);


/* Onewire Reset state machine */
static void OW_doit_reset(void)
{
   switch (one_driver->cur_op)
   {
      case OW_RESET_OP_INIT:
      {
         one_driver->delay_count = 48;
         one_driver->cur_op = OW_RESET_OP_WAIT_480_LOW;

         /* Line low */
         gpio_enable(ow_pin, GPIO_OUT_OPEN_DRAIN);
         gpio_write(ow_pin, 0);
      } break;

      case OW_RESET_OP_WAIT_480_LOW:
      {
         if ( !(--one_driver->delay_count) )
         {
            /* release */
            gpio_disable(ow_pin);

            one_driver->cur_op = OW_RESET_OP_WAIT_60_RESP; // give 60us for resp
            one_driver->delay_count = 6;
         }
      } break;

      case OW_RESET_OP_WAIT_60_RESP:
      {
         --one_driver->delay_count;

         /* Response Detected */
         if ( !gpio_read(ow_pin) )
         {
            one_driver->delay_count = 47;
            one_driver->cur_op = OW_RESET_OP_WAIT_480;
            break;
         }

         /* If we get here we timed out */
         if  ( !( one_driver->delay_count) )
         {
            gpio_disable(ow_pin); // release
            one_driver->error = OW_ERROR_RESET_RESP_TIMEOUT;
            one_driver->seq_state = OW_SEQ_STATE_DONE;
         }
      } break;

      case OW_RESET_OP_WAIT_480:
      {
         if  ( !( --one_driver->delay_count ) )
         {
            one_driver->cur_op = OW_RESET_OP_DONE;
         }
      } break;

      case OW_RESET_OP_DONE:
      {
         one_driver->seq_pos++;
         one_driver->seq_state = OW_SEQ_STATE_TRANSITION;
      } break;

      default: // invalid?
         one_driver->error = OW_ERROR_INVALID_OP;
         break;
   }
}

/* Onewire read state machine */
static void OW_doit_read(void)
{
   static uint8_t bitMask = 0x01;
   static int sample = 0;
   static uint8_t readsLeft = 0;
   static uint8_t readCount = 0;
   static uint8_t data = 0;

again:
{
   uint8_t cur_op = one_driver->cur_op;
   switch (cur_op)
   {
      case OW_READ_OP_INIT:
      {
         bitMask = 0x01;
         sample = 0;
         readCount = 0;
         data = 0;
         readsLeft = one_driver->arg_arr[one_driver->seq_pos];
         one_driver->cur_op = OW_READ_OP_LINE_LOW;
         goto again;
      } break;

      case OW_READ_OP_LINE_LOW:
      {
         /* Line low */
         gpio_enable(ow_pin, GPIO_OUT_OPEN_DRAIN);
         gpio_write(ow_pin, 0);

         /* Wait */
         sdk_os_delay_us(2);

         /* Release */
         gpio_disable(ow_pin);

         one_driver->cur_op = OW_READ_OP_SAMPLE;
      #ifdef OW_ADDTL_DEBUG
         one_driver->readbit_count++;
      #endif
      } break;

      case OW_READ_OP_SAMPLE:
      {
         /* Sample */
         sample = gpio_read(ow_pin);
         if (sample)
            data |= bitMask;

         bitMask <<= 1;

         one_driver->delay_count = 4;
         one_driver->cur_op = OW_READ_OP_WAIT_50;
      } break;

      case OW_READ_OP_WAIT_50:
      {
         /* Done waiting. Perform checks. */
         if  ( !(--one_driver->delay_count))
         {
            /* We're done with *a* read sequence. */
            if (!bitMask)
            {
               /* Put data into buffer */
               one_driver->spad_buf[ readCount++ ] = data;

               /* Reset data and bitMask. Important! */
               data = 0;
               bitMask = 0x01;

            #ifdef OW_DEBUG_VALS
               one_driver->readbyte_count++;
            #endif

               /* No more read sequences left. We're completely done. */
               if ( !(--readsLeft) )
               {
                  one_driver->cur_op = OW_READ_OP_DONE;
                  break;
               }
            }

            /* If we get here we have more reads to do. Restart at line low. */
            one_driver->cur_op = OW_READ_OP_LINE_LOW;
            goto again;
         }
      } break;
   }
} /* End again block */


  /* Last OP. This means we're done with this set of ops.              *
   * Increment seq_pos. Main switch will determine if this means we're *
   * also done with the last op-set in the sequence. to finished so we */
   if (one_driver->cur_op == OW_READ_OP_DONE)
   {
      one_driver->seq_state = OW_SEQ_STATE_TRANSITION;
      one_driver->seq_pos++;
   }
}

/* Onewire Write state machine */
static void OW_doit_write(void)
{
   static uint8_t bitMask = 0x01;
   static uint8_t data = 0;

again:
{
   uint8_t cur_op = one_driver->cur_op;
   switch (cur_op)
   {
      case OW_WRITE_OP_INIT:
      {
         data = one_driver->arg_arr[one_driver->seq_pos];
         bitMask = 0x01;
         one_driver->cur_op = OW_WRITE_OP_LINE_LOW;
      } break;

      case OW_WRITE_OP_LINE_LOW:
      {
         if (bitMask & data)
         {
            one_driver->cur_op = OW_WRITE_OP_WRITE_FIRST; // "1" type
            one_driver->delay_count = 6;
         }
         else
         {
            one_driver->cur_op = OW_WRITE_OP_WAIT_FIRST; // "0" type
            one_driver->delay_count = 6;
         }
         /* Line Low */
         gpio_write(ow_pin, 0);
         gpio_enable(ow_pin, GPIO_OUT_OPEN_DRAIN);
      } break;

      /* "1" type, write now. */
      case OW_WRITE_OP_WRITE_FIRST:
      {
         if  ( one_driver->delay_count == 6 )
         {
            /* Drive high */
            gpio_write(ow_pin, 1);
         }

         if  ( !(--one_driver->delay_count) )
         {
            one_driver->cur_op = OW_WRITE_OP_FINALLY;
         }
      } break;

      /* "0" type, wait to write. */
      case OW_WRITE_OP_WAIT_FIRST:
      {
         /* Done waiting (or no wait). Write bit. */
         if  ( !(--one_driver->delay_count) )
         {
            /* Drive high */
            gpio_write(ow_pin, 1);
            one_driver->cur_op = OW_WRITE_OP_FINALLY;
         }
      } break;

      case OW_WRITE_OP_FINALLY:
      {
         bitMask = bitMask << 1;

         /* We're done. Move on. */
         if (! (bitMask) )
         {
            if (USE_P_PWR)
            {
               gpio_disable(ow_pin);
               gpio_write(ow_pin, 0);
            }
            one_driver->cur_op = OW_WRITE_OP_DONE;
         }
         /* More bits to read. Go back to line low. */
         else
         {
            one_driver->cur_op = OW_WRITE_OP_LINE_LOW;
            goto again;
         }
      } break;
   }
} /* End again block */

   /*  Last OP. This means we're done with this set of ops.                   */
   if (one_driver->cur_op == OW_WRITE_OP_DONE)
   {
      one_driver->seq_state = OW_SEQ_STATE_TRANSITION;
      one_driver->seq_pos++;
   }
}


/* -------------------------------------------------------------------------- *
 * NOTE: Putting debugs in this method, or methods called herein, WILL result *
 *       in improper readings, or no readings at all. Use a results callback  *
 *       if you want to see what happened during the process.                 *
 * Sequence:                                                                  *
 *    -> Encompasses the entire set of reset,write,read (ops). 1 at a time.   *
 * Operation:                                                                 *
 *    -> An Op is defined for any part of a sequence that requires a delay    *
 *       that is >= 10us between. Eg.:                                        *
 *       -> A delay of 10us is an op. Another delay of 10us after a GPIO op   *
 *          is another *separate* op.                                         *
 *       -> A one-time delay of 500us is a single op that gets repeated.      *
 *       -> A GPIO read followed by a 5us delay and another write is 1 op.    *
 * -------------------------------------------------------------------------- */
void OW_doit(void)
{
   int type = one_driver->op_type;
   int state = one_driver->seq_state;

   switch (type)
   {
      case OW_OP_READ:
      {
         if (state == OW_SEQ_STATE_TRANSITION)
         {
            one_driver->seq_state = OW_SEQ_STATE_STARTED;
            one_driver->cur_op = OW_READ_OP_INIT;
         }
         /* Run the read state machine */
         OW_doit_read();
      } break;

      case OW_OP_WRITE:
      {
         if (state == OW_SEQ_STATE_TRANSITION)
         {
            one_driver->seq_state = OW_SEQ_STATE_STARTED;
            one_driver->cur_op = OW_WRITE_OP_INIT;
         }
         /* Run the write state machine */
         OW_doit_write();
      } break;

      case OW_OP_RESET:
      {
         if (state == OW_SEQ_STATE_TRANSITION)
         {
            one_driver->seq_state = OW_SEQ_STATE_STARTED;
            one_driver->cur_op = OW_RESET_OP_INIT;
         }
         /* Run the reset state machine */
         OW_doit_reset();
      } break;

      /* Last OP. This means we're done. Set seq state to finished so we   *
       * know to disable the hw_timer and call the callback.               */
      case OW_OP_END:
      #ifdef OW_DEBUG_SEQS
         printf("op end\n");
      #endif
         one_driver->seq_state = OW_SEQ_STATE_DONE;
         break;

      /* Shouldn't occur. Treat as done anyways */
      case OW_OP_MAX:
         one_driver->seq_state = OW_SEQ_STATE_DONE;
         break;

      /* Invalid. Terminate */
      default:
         printf("seq_state inv\n");
         one_driver->seq_state = OW_SEQ_STATE_DONE;
         break;
   }

   state = one_driver->seq_state;
   switch (state)
   {
      case OW_SEQ_STATE_INIT:
      {
         /* Update seq_state and arm HW timer */
         one_driver->seq_state = OW_SEQ_STATE_STARTED;
         // hw_timer_arm(10);
      } break;

      case OW_SEQ_STATE_STARTED:
      {
         /* Just arm HW timer */
         // hw_timer_arm(10);
      } break;

      case OW_SEQ_STATE_TRANSITION:
      {
         /* If seq_pos is equal to, or has advanced beyond the total # of     *
          * sequences (seq_len) we're done. End it.                           */
         if (one_driver->seq_pos >= one_driver->seq_len)
         {
            one_driver->op_type = OW_OP_END;
            one_driver->seq_state = OW_SEQ_STATE_DONE;
            // hw_timer_stop();
         }
         /* Reset cur_op to 0 to trigger the seq init on the next doit call.  */
         else
         {
         #ifdef OW_DEBUG_SEQS
            printf("trans:%hd\n", one_driver->seq_pos);
         #endif
            one_driver->op_type = one_driver->seq_arr[one_driver->seq_pos];
            one_driver->cur_op = 0;
            // hw_timer_arm(10);
         }
      } break;

      case OW_SEQ_STATE_DONE:
      {
         if (one_driver->callback)
         {
            one_driver->callback();
         }
         /* Disarm HW timer gets taken care of in callbacks. Leaving here as  *
         * a reference in case it's desired to disarm the hw_timer inside the *
         * state machine. Could be desired if using the hw timer configured   *
         * with autoload for reset or some other longer delay.                */
         // hw_timer_disarm();
      } break;
   }
}

/* This is called immediately at the end of a read uuid sequence */
static void read_uuid_done_cb(void)
{
   uint8_t i, j;
#ifdef OW_DEBUG_VALS
   printf("Read done: %d bits, %d bytes\n", one_driver->readbit_count, one_driver->readbyte_count);
#endif
   if (one_driver->error)
   {
      OW_handle_error(DS_SEQ_UUID_T);
      return;
   }
   hw_timer_stop();

   ow_task_arg = DS_SEQ_CONV_T;

   one_driver->UUID[0] = DS_FAMILY_CODE;
   memcpy(one_driver->UUID+1, one_driver->spad_buf, 7);

   for (i=1,j=0; i<8; i++)
   {
      if ((one_driver->UUID[i] == 0) || (one_driver->UUID[i] == 0xff))
      {
         j++;
      }
   }

   if (j == (OW_UUID_LEN-1))
   {
      one_driver->have_uuid = 0;
      printf("bad UUID. Retrying\n");
   } else {
      one_driver->have_uuid = 1;
   }

#ifdef OW_DEBUG_UUID
   printf("UUID: %02x", one_driver->UUID[0]);
   for (i=1; i<8; i++)
      printf(":%02x", one_driver->UUID[i]);
   printf("\n");
#endif

   /* Reset state to trigger OW_init_seq_task() */
   one_driver->seq_state = OW_SEQ_STATE_INIT;
}

/* This is called immediately at the end of a conv_t sequence */
static void conv_t_done_cb(void)
{
   if (one_driver->error)
   {
      OW_handle_error(DS_SEQ_CONV_T);
      return;
   }
   hw_timer_stop();

   /* Set next sequence */
   ow_task_arg = DS_SEQ_READ_T;

   /* Reset state to trigger OW_init_seq_task() */
   one_driver->seq_state = OW_SEQ_STATE_INIT;
}

/* This is called immediately at the end of a read_t sequence */
static void read_temp_done_cb(void)
{
   uint16_t reading;
   if (one_driver->error)
   {
      OW_handle_error(DS_SEQ_READ_T);
      return;
   }
   hw_timer_stop();

#ifdef OW_DEBUG_VALS
   uint8_t i;
   for (i=0; i<9; i++)
      printf("r:%02x\n", one_driver->spad_buf[i]);
#endif

   const uint8_t * checkCrc = one_driver->spad_buf;

   /* Verify CRC */
   uint8_t crc = onewire_crc8(checkCrc, 8);
   if (crc == one_driver->spad_buf[8])
   {
      /* CRC OK, populate latest_temp */
      latest_temp.sign = '+';
      reading = (one_driver->spad_buf[1] << 8) | one_driver->spad_buf[0];
      if (reading & 0x8000)
      {
         reading = (reading ^ 0xffff) + 1;				// 2's complement
         latest_temp.sign = '-';
      }

      latest_temp.val = reading >> 4;  // separate off the whole and fractional portions
      latest_temp.fract = (reading & 0xf) * 100 / 16;
      latest_temp.available = 1; // mark as available

      OW_queue_temperature();
   }
   else
   {
      printf("OW CRC err: %02x(exp) != 0x%02x!\n", one_driver->spad_buf[8], crc);
   }

   memset(one_driver->spad_buf, 0, OW_SPAD_SIZE);

   ow_task_arg = DS_SEQ_NEXT_READ_T;
}

static void OW_init_seq_task(void *pxParameter)
{
   while(1)
   {
      if (one_driver->seq_state == OW_SEQ_STATE_INIT)
      {
         /* Assign first op (always 0) */
         one_driver->op_type = one_driver->seq_arr[0];
         /* Start with first valid operation (always 1) */
         one_driver->cur_op = 0;
         one_driver->seq_pos = 0;

      #ifdef OW_DEBUG_SEQS
         printf("1w seq init: type=%hd, op=%hd\n", one_driver->op_type, one_driver->cur_op);
      #endif

         switch (ow_task_arg)
         {
            /* Start read UUID in right away */
            case DS_SEQ_UUID_T:
               OW_init_seq(DS_SEQ_UUID_T);
               break;

            /* UUID is done. Wait a bit then start conversion. */
            case DS_SEQ_CONV_T:
               vTaskDelayMs(15U);
               OW_init_seq(DS_SEQ_CONV_T);
               break;

            /* Conversion is done. Wait 750ms then start read. */
            case DS_SEQ_READ_T:
               vTaskDelayMs(750U);
               OW_init_seq(DS_SEQ_READ_T);
               break;

               /* Read is done. Print out and wait some time to reset the process */
            case DS_SEQ_NEXT_READ_T:
            #ifdef OW_DEBUG_VALS
               one_driver->readbit_count = 0;
               one_driver->readbyte_count = 0;
            #endif
               OW_init_seq(DS_SEQ_CONV_T);
               // hw_timer_arm(10);
               vTaskDelayMs(2431U);
               break;

            default:
               one_driver->seq_state = 0;
               printf("invalid ow_task_arg\n");
               vTaskDelayMs(2512U);
               break;
         }
         vTaskDelayMs(1);
      }
      else
      {
         vTaskDelayMs(25);
      }

   }
}

void OW_handle_error(uint8_t cb_type)
{
   hw_timer_stop();
   printf("OW err #%hd in seq #%hd\n", one_driver->error, cb_type);
   one_driver->error = OW_ERROR_NONE;
   one_driver->ow_state = OW_STATE_READY;
}

void OW_queue_temperature(void)
{
   static Temperature prev_temp;
   static char buf[32];
   if (latest_temp.available)
   {
      if (memcmp(&prev_temp, &latest_temp, sizeof(Temperature)) != 0)
      {
         memcpy(&prev_temp, &latest_temp, sizeof(Temperature));
      #ifdef OW_DEBUG_TEMP
         printf("new temp = %c%d.%02d deg.C\n", latest_temp.sign, latest_temp.val, latest_temp.fract);
      #endif
         int ret = sprintf(buf, "%c%d.%02d", latest_temp.sign, latest_temp.val, latest_temp.fract);

         char * pubBuf = (char*) malloc(sizeof(char)*7);
         memcpy(pubBuf, &buf, 6);
         pubBuf[6] = '\0';

         /* QueueHandle_t	xQueue, const void* pvItemToQueue, TickType_t xTicksToWait */
         xQueueSendToBack(*pubTempQueueHandle, &pubBuf, (TickType_t)0);
      }
   }
   else
   {
      printf("Temp. data N/A\n");
   }
}

void OW_init_seq(uint8_t seq)
{
#ifdef OW_DEBUG_SEQS
   printf("OW_init_seq\n");
#endif
   /* Assign seq array and args */
   one_driver->seq_arr = ds_seqs[seq];
   one_driver->arg_arr = ds_seq_args[seq];
   one_driver->seq_len = ds_seq_lens[seq];

   /* Set to init state */
   one_driver->seq_state = OW_SEQ_STATE_TRANSITION;
   one_driver->error = OW_ERROR_NONE;
   ow_task_arg = (int)seq;

   switch (seq)
   {
      case DS_SEQ_UUID_T: one_driver->callback = read_uuid_done_cb; break;
      case DS_SEQ_CONV_T: one_driver->callback = conv_t_done_cb; break;
      case DS_SEQ_READ_T: one_driver->callback = read_temp_done_cb; break;
      default:
         printf("inv init seq\n");
         ow_task_arg = 0;
         one_driver->callback = NULL;
         return;
   }

   hw_timer_init();
   hw_timer_start();
}

bool OW_request_new_temp(void)
{
   /* Clear spad buffer */
   memset(one_driver->spad_buf, 0, sizeof(OW_SPAD_SIZE));

   if (one_driver->ow_state != OW_STATE_READY)
   {
      printf("ow not ready! Reason: %s\n",
         (one_driver->ow_state == OW_STATE_IN_PROGRESS)?"Reading in progress":"not initialized" );
      return false;
   }

   /* Set onewire to pullup */
   gpio_enable(ow_pin, GPIO_OUT_OPEN_DRAIN);
   gpio_set_pullup(ow_pin, true, false); //pin, enabled, enabled during sleep

   /* Start from Convert T if we have UUID */
   if (one_driver->have_uuid)
   {
      /* Assign UUID seq array and args */
      one_driver->seq_arr = ds_seqs[DS_SEQ_CONV_T];
      one_driver->arg_arr = ds_seq_args[DS_SEQ_CONV_T];
      one_driver->seq_len = ds_seq_lens[DS_SEQ_CONV_T];

      ow_task_arg = (int)DS_SEQ_CONV_T;
      one_driver->callback = conv_t_done_cb;
   }
   /* Start from UUID if we don't have one yet */
   else
   {
      /* Assign UUID seq array and args */
      one_driver->seq_arr = ds_seqs[DS_SEQ_UUID_T];
      one_driver->arg_arr = ds_seq_args[DS_SEQ_UUID_T];
      one_driver->seq_len = ds_seq_lens[DS_SEQ_UUID_T];

      ow_task_arg = (int)DS_SEQ_UUID_T;
      one_driver->callback = read_uuid_done_cb;
   }

   /* Set to init state */
   one_driver->error = OW_ERROR_NONE;
   one_driver->seq_state = OW_SEQ_STATE_INIT;
   return true;
}

void OW_init(QueueHandle_t * pubTempQueue)
{
   if (pubTempQueue)
      pubTempQueueHandle = pubTempQueue;

   /* Init all to 0 */
   memset(one_driver, 0, sizeof(onewire_driver));
   memset(&latest_temp, 0, sizeof(latest_temp));

   /* Set onewire to pullup */
   gpio_enable(ow_pin, GPIO_OUT_OPEN_DRAIN);
   gpio_set_pullup(ow_pin, true, false); //pin, enabled, enabled during sleep

   /* Init hw timer */
   hw_timer_init();

   xTaskCreate(OW_init_seq_task, "OWInitSeqTask", 1024, NULL, OW_TASK_PRIO, &ow_seq_int_task_handle);

   one_driver->ow_state = OW_STATE_READY;
}
