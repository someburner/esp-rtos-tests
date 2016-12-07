/* Driver definitions for Maxim xx28 Family Temp Sensors */
#ifndef __MAXIM_28_h
#define __MAXIM_28_h

/*******************************************************************************
 * Timing Overview: (Reset)
 * Step 1: Master Pull LOW for >= 480us then releases
 * Step 2: Device waits at least 15us after the 480us mark and up to 60us for
 *         the line to be pulled back HIGH.
 * Step 3: After the device detects the line has gone HIGH, it pulls low between
 *         60us and 240us
 * Step 4: At the end of step 3, the device releases the line. The master should
 *         be watching for line to come back high 60-240us after it was pulled
 *         low in step 3.
*******************************************************************************/

#include "onewire.h"

#define IS_MAX31820

/*******************************************************************************
 * Address, hardware definitions
*******************************************************************************/
/* --------------------------- Command Addresses ---------------------------- */
#define DS_WRITE_SPAD        0x4E
#define DS_READ_SPAD         0xBE
#define DS_COPY_SPAD         0x48
#define DS_READ_EEPROM       0xB8
#define DS_READ_PWRSUPPLY    0xB4
#define DS_SEARCHROM         0xF0
#define DS_SKIPROM           0xCC
#define DS_READROM           0x33
#define DS_MATCHROM          0x55
#define DS_ALARMSEARCH       0xEC
#define DS_CONVERT_T         0x44

/* -------------------------- Onewire family codes -------------------------- */
#define MAXIM_20              0x10
#define MAXIM_28              0x28

/* NOTE: Comment/Uncomment in hardware_defs.h */
#if defined(IS_DS18B20) || defined(IS_MAX31820)
   #define DS_FAMILY_CODE MAXIM_28
#elif defined(IS_DS18S20)
   #define DS_FAMILY_CODE MAXIM_20
#else
   #define DS_FAMILY_CODE 0
#endif

/* -------------------------- Parasite Power Mode --------------------------- */
/* NOTE: Comment/Uncomment in hardware_defs.h */
#ifndef USE_PARASITE_POWER
   #define USE_P_PWR 0
#else
   #define USE_P_PWR 1
#endif


/*******************************************************************************
 * Sequence Definitions
*******************************************************************************/
/* -------------------------------------------------------------------------- *
 * NOTE:                                                                      *
 * Each "sequence" represents a linear combination of one or more commands or *
 * operations. These should be of type uint8_t[] where each array member is a *
 * onewire command (i.e. OW_OP_xxxx).                                         *
 * For each sequence array, you must also create an array of arguments, which *
 * contains the corresponding arg for each operation in the sequence.         *
 *    --> Read arg = # of times to read.                                      *
 *    --> Write arg = Address to write to                                     *
 *    --> Reset arg = 0 (none required)                                       *
 * -------------------------------------------------------------------------- */

/* -------------------------------- READ_UUID ------------------------------- */
#define DS_READ_UUID_SEQ_LEN 5
static  uint8_t ds_read_uuid_seq[DS_READ_UUID_SEQ_LEN] = {
   OW_OP_RESET, OW_OP_WRITE, OW_OP_WRITE, OW_OP_READ, OW_OP_END
};
static  uint8_t ds_read_uuid_seq_args[DS_READ_UUID_SEQ_LEN] = {
   0,           DS_READROM,  DS_READ_SPAD, 9,         0
};
/* ------------------------------- CONVERT_T -------------------------------- */
#define DS_CONV_T_SEQ_LEN 4
static  uint8_t ds_conv_t_seq[DS_CONV_T_SEQ_LEN] = {
   OW_OP_RESET, OW_OP_WRITE, OW_OP_WRITE,  OW_OP_END
};
static  uint8_t ds_conv_t_seq_args[DS_CONV_T_SEQ_LEN] = {
   0,           DS_SKIPROM,  DS_CONVERT_T, 0
};
/* ------------------------------- READ_TEMP -------------------------------- */
#define DS_READ_T_SEQ_LEN 5
static  uint8_t ds_read_t_seq[DS_READ_T_SEQ_LEN] = {
   OW_OP_RESET, OW_OP_WRITE, OW_OP_WRITE, OW_OP_READ, OW_OP_END
};
static  uint8_t ds_read_t_seq_args[DS_READ_T_SEQ_LEN] = {
   0,           DS_SKIPROM,  DS_READ_SPAD, 9,         0
};
/* -------------------------------- Collect --------------------------------- */
/* All sequences (+ null case) */
static uint8_t * ds_seqs[4] = {
   0, ds_read_uuid_seq, ds_conv_t_seq, ds_read_t_seq
};
/* All sequence args (+ null case) */
static uint8_t * ds_seq_args[4] = {
   0, ds_read_uuid_seq_args, ds_conv_t_seq_args, ds_read_t_seq_args
};
/* All sequence lengths */
static uint8_t ds_seq_lens[4] = {
   0, DS_READ_UUID_SEQ_LEN, DS_CONV_T_SEQ_LEN, DS_READ_T_SEQ_LEN
};

/* Enumerate sequences */
typedef enum {
   DS_SEQ_INVALID = 0,
   DS_SEQ_UUID_T,
   DS_SEQ_CONV_T,
   DS_SEQ_READ_T,
   DS_SEQ_NEXT_READ_T,
   DS_SEQ_MAX
} DS_SEQ_T;

/* Temperature struct definition */
typedef struct {
   char sign;
   uint16_t val;
   uint16_t fract;

   uint8_t available;
   uint8_t padding[2];
} Temperature;

#else /* ONEWIRE_BLOCKING */

#define MAX_DS18B20_SENSOR 1
#define MAX_TEMPERATURE_SENSOR 15
// 
// struct Temperature {
//    bool set;
//    bool override;
//    char sign;
//    uint16_t val;
//    uint16_t fract;
//    uint8_t missed;
//    char address[20];
// };


#endif
