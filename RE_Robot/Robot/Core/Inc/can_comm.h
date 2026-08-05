#ifndef __CAN_COMM_H
#define __CAN_COMM_H

#include "Packet.h"
#include "main.h"

extern FDCAN_HandleTypeDef hfdcan1;

/* CAN 연결 상태 (Hand와 동일한 Bus-Off 감지/복구용) */
extern uint8_t can_offline;

/* 최근 수신 패킷 (CAN RX 인터럽트에서 갱신) */
extern volatile GSensorPacket_t latest_gPkt;
extern volatile FlexPacket_t    latest_fPkt;

/* CAN에서 마지막으로 수신한 원본 (채널별 구분용, Live Expression 확인용) */
extern volatile GSensorPacket_t g_canRxGSensorPkt;
extern volatile FlexPacket_t    g_canRxFlexPkt;

/* Hand로부터 마지막으로 수신한 상태(0x102) — comm_mode와 무관하게 항상 갱신 */
extern volatile HandStatusPacket_t g_canRxHandStatusPkt;

/* Robot 자체 상태 (0x201로 1000ms마다 주기 송신) — lcd.c/servo.c가 필드를 나눠서 채움 */
extern RobotStatusPacket_t g_robotStatusPkt;

/* 통신 연결 상태 값 */
#define COMM_CONNECTED_NONE  0   /* 연결 없음 */
#define COMM_CONNECTED_CAN   1   /* CAN 연결 성공 */
#define COMM_CONNECTED_BT    2   /* BT 연결 성공 */

/* 통신 연결 플래그 (CAN ISR → COMM_CONNECTED_CAN, BT CommTask → COMM_CONNECTED_BT) */
extern volatile uint8_t comm_connected;

void CAN_Comm_Init(void);
void CAN_Send(uint32_t id, const uint8_t *data);
void CAN_Recovery(void);
void RobotStatus_Send(void);

#endif /* __CAN_COMM_H */
