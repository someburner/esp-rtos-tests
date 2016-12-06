#ifndef HW_TIMER_H_
#define HW_TIMER_H_

#include <stdint.h>

typedef enum {
   HW_TIMER_DISABLED = 0,
   HW_TIMER_INIT = 1,
   HW_TIMER_READY = 2,
   HW_TIMER_ACTIVE = 3,
   HW_TIMER_STOPPED = 4
} HW_TIMER_STATE_T;

HW_TIMER_STATE_T hw_timer_get_state(void);
uint32_t getTestCount(void);

void hw_timer_init(void);
void hw_timer_restart(void);
void hw_timer_start(void);
void hw_timer_stop(void);







#endif /* end HW_TIMER_H_ */
