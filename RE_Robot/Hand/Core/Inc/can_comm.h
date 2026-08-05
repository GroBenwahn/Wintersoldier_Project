#ifndef __CAN_COMM_H
#define __CAN_COMM_H

#include "Packet.h"
#include "main.h"

/* HAL 핸들 (main.c에서 선언) */
extern FDCAN_HandleTypeDef hfdcan1;

/* CAN 연결 상태 */
extern uint8_t can_offline;

<<<<<<< HEAD
=======
/* 최근 수신 패킷 (CAN RX 인터럽트에서 갱신, Robot으로부터) */
extern volatile ServoDbgPacket_t    g_canRxServoDbgPkt;
extern volatile RobotStatusPacket_t g_canRxRobotStatusPkt;

>>>>>>> f898ed6684598243425a488b2c8abcd53dad3f7c
/* 함수 선언 */
void CAN_Comm_Init(void);
void CAN_Send(uint32_t id, const uint8_t *data);
void CAN_Recovery(void);

#endif /* __CAN_COMM_H */
