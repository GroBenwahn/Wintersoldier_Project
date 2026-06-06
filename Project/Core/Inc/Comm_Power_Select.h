#ifndef COMM_POWER_SELECT_H
#define COMM_POWER_SELECT_H

#include "config.h"   /* CommMode, BT_ConnState_t, extern 선언 모두 여기서 */

/* API */
void CommPowerSelect_Init(void);
void CommPowerSelect_ButtonPressed(void);    /* EXTI 콜백에서 호출 */
void CommPowerSelect_DebounceExpired(void);  /* TimerOnce_Callback에서 호출 */

#endif /* COMM_POWER_SELECT_H */
