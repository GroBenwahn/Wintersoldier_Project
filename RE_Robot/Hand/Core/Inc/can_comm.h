#ifndef __CAN_COMM_H
#define __CAN_COMM_H

#include "Packet.h"
#include "main.h"

/* HAL 핸들 (main.c에서 선언) */
extern FDCAN_HandleTypeDef hfdcan1;

/* CAN 연결 상태 */
extern uint8_t can_offline;

/* CAN 메시지 ID 단축 이름 */
#define CAN_MSG_SENSOR  CAN_MSG_ID_SENSOR
#define CAN_MSG_FLEX    CAN_MSG_ID_FLEX

/* 함수 선언 */
void CAN_Comm_Init(void);
void CAN_Send(uint32_t id, const uint8_t *data);
void CAN_Recovery(void);

#endif /* __CAN_COMM_H */
