/*
 * comm_can_controller.h  —  리모콘 보드 CAN 처리
 *
 * 역할: 리모콘이 보내는 0x100/0x101 패킹 및 TX,
 *       리모콘이 받는 0x202 파싱
 */

#ifndef INC_COMM_CAN_CONTROLLER_H_
#define INC_COMM_CAN_CONTROLLER_H_

#include "comm_can.h"

#if (!ProjModeState)

void Controller_CAN_TX_Sensor(void);        /* remoteSensorTx → TX 0x100 (10ms)  */
void Controller_CAN_TX_Status(void);        /* 상태 정보      → TX 0x101 (100ms) */
void Controller_CAN_RX_Handle(uint32_t id, uint8_t *data);  /* 0x202 수신 → sysStatus */

#endif /* !ProjModeState */

#endif /* INC_COMM_CAN_CONTROLLER_H_ */
