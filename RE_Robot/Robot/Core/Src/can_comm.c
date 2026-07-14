#include "can_comm.h"
#include <string.h>

/* 최근 수신 패킷 저장소 (ISR에서 갱신되므로 volatile) */
volatile GSensorPacket_t latest_gPkt = {0};
volatile FlexPacket_t    latest_fPkt = {0};


/* CAN 초기화: 수신 필터 + 알림 등록 + 시작 */
void CAN_Comm_Init(void)
{
    /* 표준 ID 수신 필터: 0x001(GSensor) ~ 0x002(Flex) → Rx FIFO0 */
    FDCAN_FilterTypeDef filter;
    filter.IdType       = FDCAN_STANDARD_ID;
    filter.FilterIndex  = 0;
    filter.FilterType   = FDCAN_FILTER_RANGE;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1    = CAN_MSG_SENSOR;       /* 0x100 */
    filter.FilterID2    = CAN_MSG_FLEX;        /* 0x101 */
    HAL_FDCAN_ConfigFilter(&hfdcan1, &filter);

    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_Start(&hfdcan1);
}



/* CAN Rx FIFO0 수신 콜백 (인터럽트) */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if (!(RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE)) return;

    FDCAN_RxHeaderTypeDef rxHeader;
    uint8_t data[8];

    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, data) != HAL_OK) return;

    /* 체크섬 검증 (byte 0~6 XOR = byte 7) */
    uint8_t cs = 0;
    for (int i = 0; i < 7; i++) cs ^= data[i];
    
    if (cs != data[7]) return;

    if (rxHeader.Identifier == CAN_MSG_SENSOR)
        memcpy((void *)&latest_gPkt, data, 8);

    else if (rxHeader.Identifier == CAN_MSG_FLEX)
        memcpy((void *)&latest_fPkt, data, 8);
}


