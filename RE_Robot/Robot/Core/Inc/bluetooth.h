#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include "Packet.h"
#include "main.h"

extern UART_HandleTypeDef huart1;

typedef enum {
    BT_OK  = 0,
    BT_ERR = 1
} BtStatus_t;

void       BT_Init(void);
void       BT_FlushQueue(void);
BtStatus_t BT_Recv(GSensorPacket_t *gPkt, FlexPacket_t *fPkt);

#endif /* __BLUETOOTH_H */
