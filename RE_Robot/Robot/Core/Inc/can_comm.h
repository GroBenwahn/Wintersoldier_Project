#ifndef __CAN_COMM_H
#define __CAN_COMM_H

#include "Packet.h"
#include "main.h"

extern FDCAN_HandleTypeDef hfdcan1;

/* 최근 수신 패킷 (CAN RX 인터럽트에서 갱신) */
extern volatile GSensorPacket_t latest_gPkt;
extern volatile FlexPacket_t    latest_fPkt;

/* 통신 연결 상태 값 */
#define COMM_CONNECTED_NONE  0   /* 연결 없음 */
#define COMM_CONNECTED_CAN   1   /* CAN 연결 성공 */
#define COMM_CONNECTED_BT    2   /* BT 연결 성공 */

/* 통신 연결 플래그 (CAN ISR → COMM_CONNECTED_CAN, BT CommTask → COMM_CONNECTED_BT) */
extern volatile uint8_t comm_connected;

void CAN_Comm_Init(void);

#endif /* __CAN_COMM_H */
