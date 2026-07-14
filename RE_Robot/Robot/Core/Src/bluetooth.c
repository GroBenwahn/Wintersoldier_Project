#include "bluetooth.h"
#include <string.h>

/* 단일 프레임 수신: [0xAA][8 bytes][0x55] */
static BtStatus_t RecvFrame(uint8_t *buf, uint32_t hunt_timeout_ms)
{
    uint8_t byte;
    uint32_t start = HAL_GetTick();

    /* Start byte(0xAA) 탐색 */
    do {
        if (HAL_UART_Receive(&huart1, &byte, 1, 2) == HAL_OK && byte == PKT_START)
            break;
        if ((HAL_GetTick() - start) >= hunt_timeout_ms)
            return BT_ERR;
    } while (1);


    /* 8 bytes 데이터 + 1 byte end byte 수신 */
    uint8_t frame[9];
    if (HAL_UART_Receive(&huart1, frame, 9, 20) != HAL_OK)
        return BT_ERR;


    /* End byte 확인 */
    if (frame[8] != PKT_END)
        return BT_ERR;


    /* 체크섬 검증 (byte 0~6 XOR = byte 7) */
    uint8_t cs = 0;
    for (int i = 0; i < 7; i++) cs ^= frame[i];
    if (cs != frame[7])
        return BT_ERR;


    memcpy(buf, frame, 8);
    return BT_OK;
}



/* Hand BT 프레임 수신: GSensorPacket + FlexPacket */
BtStatus_t BT_Recv(GSensorPacket_t *gPkt, FlexPacket_t *fPkt)
{
    uint8_t buf[8];

    /* GSensorPacket 수신 */
    if (RecvFrame(buf, 50) != BT_OK) return BT_ERR;
    memcpy(gPkt, buf, 8);

    /* FlexPacket 수신 */
    if (RecvFrame(buf, 30) != BT_OK) return BT_ERR;
    memcpy(fPkt, buf, 8);

    return BT_OK;
}
