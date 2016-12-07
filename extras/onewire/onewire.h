#ifndef __ONEWIRE_H__
#define __ONEWIRE_H__

#include <espressif/esp_misc.h> // sdk_os_delay_us
#include "FreeRTOS.h"
#include "queue.h"

// #define OW_DEBUG_SEQS
// #define OW_DEBUG_VALS
// #define OW_DEBUG_TEMP
// #define OW_DEBUG_UUID

/* Scratchpad size (uint8_t) */
#define OW_SPAD_SIZE 9
/* Onewire device UUID length (uint8_t) */
#define OW_UUID_LEN  8

typedef enum {
   OW_SEQ_STATE_INVALID = 0,
   OW_SEQ_STATE_LOAD,
   OW_SEQ_STATE_INIT,
   OW_SEQ_STATE_STARTED,
   OW_SEQ_STATE_TRANSITION,
   OW_SEQ_STATE_DONE,
   OW_SEQ_STATE_MAX
} OW_SEQ_STATE_T;

typedef enum {
   OW_OP_INVALID = 0,
   OW_OP_RESET,
   OW_OP_READ,
   OW_OP_WRITE,
   OW_OP_END,
   OW_OP_MAX
} OW_OP_T;

typedef enum {
   OW_READ_OP_INVALID = 0,
   OW_READ_OP_INIT,
   OW_READ_OP_LINE_LOW,
   OW_READ_OP_SAMPLE,
   OW_READ_OP_WAIT_50,
   OW_READ_OP_DONE
} OW_READ_OP_T;

typedef enum {
   OW_WRITE_OP_INVALID = 0,
   OW_WRITE_OP_INIT,
   OW_WRITE_OP_LINE_LOW,
   OW_WRITE_OP_WRITE_FIRST,
   OW_WRITE_OP_WAIT_FIRST,
   OW_WRITE_OP_FINALLY,
   OW_WRITE_OP_DONE
} OW_WRITE_OP_T;

typedef enum {
   OW_RESET_OP_INVALID = 0,
   OW_RESET_OP_INIT,
   OW_RESET_OP_WAIT_480_LOW,
   OW_RESET_OP_WAIT_60_RESP,
   OW_RESET_OP_WAIT_480,
   OW_RESET_OP_DONE
} OW_RESET_OP_T;

typedef enum {
   OW_ERROR_NONE = 0,
   OW_ERROR_INVALID_SEQ,        /* Errno. 1 */
   OW_ERROR_INVALID_OP,         /* Errno. 2 */
   OW_ERROR_RESET_RESP_TIMEOUT  /* Errno. 3 */
} OW_ERROR_T;

typedef enum {
   OW_STATE_INVALID = 0,
   OW_STATE_DISABLED,
   OW_STATE_IN_PROGRESS,
   OW_STATE_READY
} OW_STATE_T;

typedef void (*OW_cb_t)(void);

typedef struct
{
   uint8_t * seq_arr;   // read temp, read rom, etc
   uint8_t * arg_arr;   // read temp, read rom, etc

   uint8_t seq_len;
   uint8_t seq_state;   // init, started, stopped
   uint8_t seq_pos;     // reset, skip rom, conv, etc
   uint8_t error;

   uint8_t op_type;     // read, write, reset, etc (OW_OP_T)
   uint8_t cur_op;      // where are we in the operation
   uint8_t delay_count; // 1 count per 10us

   uint8_t spad_buf[OW_SPAD_SIZE]; // scratchpad buffer for temperature data
   uint8_t UUID[8];     // unique ID for this device

   uint8_t have_uuid;
   uint8_t ow_state;
   uint8_t pad[2];

   OW_cb_t callback;

   /* Useful for debugging */
#ifdef OW_DEBUG_VALS
   uint16_t readbyte_count;
   uint16_t readbit_count;
#endif
} onewire_driver_t;


void OW_doit(void);
void OW_init_seq(uint8_t seq);

void OW_queue_temperature(void);
void OW_handle_error(uint8_t cb_type);

bool OW_request_new_temp(void);
void OW_init(QueueHandle_t * pubTempQueue);





#endif  /* __ONEWIRE_H__ */
