#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include "Packet.h"
#include "main.h"

extern UART_HandleTypeDef huart1;

typedef enum {
    BT_OK  = 0,
    BT_ERR = 1
} BtStatus_t;

/* Hand BT 프레임 수신 (GSensorPacket + FlexPacket 두 프레임) */
BtStatus_t BT_Recv(GSensorPacket_t *gPkt, FlexPacket_t *fPkt);

#endif /* __BLUETOOTH_H */
