#ifndef __CAN_COMM_H
#define __CAN_COMM_H

#include "Packet.h"
#include "main.h"

extern FDCAN_HandleTypeDef hfdcan1;

/* 최근 수신 패킷 (CAN RX 인터럽트에서 갱신) */
extern volatile GSensorPacket_t latest_gPkt;
extern volatile FlexPacket_t    latest_fPkt;

void CAN_Comm_Init(void);

#endif /* __CAN_COMM_H */
