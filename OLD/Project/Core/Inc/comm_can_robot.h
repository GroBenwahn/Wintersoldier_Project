/*
 * comm_can_robot.h  —  로봇팔 보드 CAN 처리
 *
 * 역할: 로봇팔이 보내는 0x202 패킹 및 TX,
 *       로봇팔이 받는 0x100/0x101 파싱
 */

#ifndef INC_COMM_CAN_ROBOT_H_
#define INC_COMM_CAN_ROBOT_H_

#include "comm_can.h"

#if (ProjModeState)

void Robot_CAN_TX_Status(void);             /* 상태 정보 → TX 0x202 (100ms)      */
void Robot_CAN_RX_Handle(uint32_t id, uint8_t *data);  /* 0x100/0x101 수신 → remoteSensorRx, sysStatus */

#endif /* ProjModeState */

#endif /* INC_COMM_CAN_ROBOT_H_ */
