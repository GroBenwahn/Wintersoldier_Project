#ifndef __CAN_COMM_H
#define __CAN_COMM_H

#include "Packet.h"
#include "main.h"

/* HAL 핸들 (main.c에서 선언) */
extern FDCAN_HandleTypeDef hfdcan1;

/* CAN 연결 상태 */
extern uint8_t can_offline;

/* 함수 선언 */
void CAN_Comm_Init(void);
void CAN_Send(const GSensorPacket_t *gPkt, const FlexPacket_t *fPkt);
void CAN_Recovery(void);

#endif /* __CAN_COMM_H */
