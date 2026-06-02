#ifndef COMM_POWER_SELECT_H
#define COMM_POWER_SELECT_H

#include "main.h"
#include "cmsis_os.h"

/* 통신 모드 */
typedef enum {
    COMM_MODE_NONE = 0,
    COMM_MODE_BT,
    COMM_MODE_CAN
} CommMode_t;

/* 전역 상태 — Comm_Task / main.c 에서 참조 */
extern volatile CommMode_t currentCommMode;
extern volatile uint8_t    localSwitchStatus;   /* 0=BT(릴레이ON), 1=CAN(릴레이OFF) */

/* API */
void CommPowerSelect_Init(void);
void CommPowerSelect_ButtonPressed(void);    /* EXTI 콜백에서 호출 */
void CommPowerSelect_DebounceExpired(void);  /* TimerOnce_Callback에서 호출 */

#endif /* COMM_POWER_SELECT_H */
