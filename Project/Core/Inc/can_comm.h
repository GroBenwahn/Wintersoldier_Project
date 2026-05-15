/*
 * can_comm.h
 *
 *  Created on: 2026. 5. 14.
 *      Author: MinJae
 */

#ifndef INC_CAN_COMM_H_
#define INC_CAN_COMM_H_

#include "main.h"
#include "config.h"



#define CAN_TIMEOUT_MS  300
//CAN 메시지 ID 정의
#define CAN_ID_REMOTE_DATA	0x100	//리모콘 --> 로봇팔
#define CAN_ID_ROBOT_STATUS	0x200	//로봇팔 --> 리모콘



//CAN 수신 데이터 구조체
typedef struct {
    uint32_t id;
    uint8_t  data[8];
    uint8_t  dlc;
} CAN_RxRaw_t;

extern CAN_RxData_t canRxData;
extern uint8_t canConnected;


uint8_t CAN_IsConnected(void);
void CAN_Start(void);

#if(ProjModeState == PROJ_MODE_REMOTE)
HAL_StatusTypeDef CAN_Transmit(RemoteData *txData);
uint8_t           CAN_CalcChecksum(RemoteData *data);
#else
uint8_t CAN_ParseRxData(CAN_RxRaw_t *rxRaw, RemoteData *payload);
#endif

#endif /* INC_CAN_COMM_H_ */
