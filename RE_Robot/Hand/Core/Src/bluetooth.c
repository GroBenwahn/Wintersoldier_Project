#include "bluetooth.h"

/* BT UART 프레임 송신: [0xAA][8바이트 패킷][0x55] */
void BT_Send(const GSensorPacket_t *gPkt, const FlexPacket_t *fPkt)
{
    uint8_t frame[UART_FRAME_SIZE];

    /* GSensorPacket 송신 */
    frame[0] = PKT_START;
    for (int i = 0; i < 8; i++) {
        frame[1 + i] = ((const uint8_t *)gPkt)[i];
    }
    frame[9] = PKT_END;
    HAL_UART_Transmit(&huart1, frame, UART_FRAME_SIZE, 10);

    /* FlexPacket 송신 */
    frame[0] = PKT_START;
    for (int i = 0; i < 8; i++) {
        frame[1 + i] = ((const uint8_t *)fPkt)[i];
    }
    frame[9] = PKT_END;
    HAL_UART_Transmit(&huart1, frame, UART_FRAME_SIZE, 10);
}
