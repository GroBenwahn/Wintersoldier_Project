#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include "Packet.h"
#include "main.h"

/* HAL 핸들 (main.c에서 선언) */
extern UART_HandleTypeDef huart1;

/* 함수 선언 */
void BT_Send(const GSensorPacket_t *gPkt, const FlexPacket_t *fPkt);

#endif /* __BLUETOOTH_H */
