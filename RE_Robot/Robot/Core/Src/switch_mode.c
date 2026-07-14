#include "switch_mode.h"

volatile CommMode_t comm_mode = COMM_NONE;

/* GPIO 폴링으로 스위치 위치 감지 → 통신 모드 갱신 */
void SwitchMode_Update(void)
{
    uint8_t can_sw = (HAL_GPIO_ReadPin(CH_Switch_CAN_GPIO_Port, CH_Switch_CAN_Pin) == GPIO_PIN_RESET) ? 1 : 0;
    uint8_t bt_sw  = (HAL_GPIO_ReadPin(CH_Switch_BT_GPIO_Port,  CH_Switch_BT_Pin)  == GPIO_PIN_RESET) ? 1 : 0;

    if      (can_sw && !bt_sw)  comm_mode = COMM_CAN;   /* 오른쪽: PB4=LOW */
    else if (bt_sw  && !can_sw) comm_mode = COMM_BT;    /* 왼쪽:  PB5=LOW */
    else                        comm_mode = COMM_NONE;  /* 가운데: 둘 다 HIGH */
}

CommMode_t SwitchMode_Get(void)
{
    return comm_mode;
}
