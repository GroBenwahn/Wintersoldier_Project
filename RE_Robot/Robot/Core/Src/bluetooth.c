#include "bluetooth.h"
#include "can_comm.h"
#include "cmsis_os.h"
#include <string.h>

#define FRAME_SIZE  10   /* [0xAA][8 bytes data][0x55] */

/* 수신 상태머신 */
typedef enum { RX_HUNT = 0, RX_DATA } RxState_t;

static volatile RxState_t rx_state  = RX_HUNT;
static uint8_t            hunt_byte;             /* 1바이트 탐색 버퍼 */
static uint8_t            frame_buf[FRAME_SIZE]; /* 프레임 수신 버퍼  */

/* 검증 완료된 8바이트 패킷을 저장하는 큐 (ISR → CommTask) */
static osMessageQueueId_t bt_pkt_queue;


/* CommTask 시작 시 1회 호출 */
void BT_Init(void)
{
    bt_pkt_queue = osMessageQueueNew(4, 8, NULL);
    rx_state     = RX_HUNT;
    HAL_UART_Receive_IT(&huart1, &hunt_byte, 1);
}


/*
 * UART 수신 완료 인터럽트 콜백 (ISR 컨텍스트)
 *
 * RX_HUNT: 1바이트씩 읽어 0xAA(시작 바이트) 탐색
 * RX_DATA: 나머지 9바이트(데이터 8 + 종료 1) 수신 → 검증 → 큐에 Put
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1) return;

    if (rx_state == RX_HUNT)
    {
        if (hunt_byte == PKT_START)
        {
            frame_buf[0] = PKT_START;
            rx_state     = RX_DATA;
            HAL_UART_Receive_IT(&huart1, frame_buf + 1, FRAME_SIZE - 1);
        }
        else
        {
            HAL_UART_Receive_IT(&huart1, &hunt_byte, 1);
        }
    }
    else /* RX_DATA */
    {
        rx_state = RX_HUNT;

        /* 종료 바이트 + 체크섬 검증 (byte 1~7 XOR = byte 8) */
        if (frame_buf[FRAME_SIZE - 1] == PKT_END)
        {
            uint8_t cs = 0;
            for (int i = 1; i < 8; i++) cs ^= frame_buf[i];

            if (cs == frame_buf[8])
            {
                uint8_t pkt[8];
                memcpy(pkt, frame_buf + 1, 8);
                osMessageQueuePut(bt_pkt_queue, pkt, 0, 0); /* ISR-safe, 타임아웃=0 */
            }
        }

        HAL_UART_Receive_IT(&huart1, &hunt_byte, 1); /* 다음 프레임 탐색 시작 */
    }
}


/* 모드 전환 시 큐에 남은 오래된 데이터 제거 */
void BT_FlushQueue(void)
{
    osMessageQueueReset(bt_pkt_queue);
}


/*
 * CommTask에서 호출.
 * osMessageQueueGet은 데이터가 올 때까지 진짜 Block → CPU를 ServoTask에 양보.
 */
BtStatus_t BT_Recv(GSensorPacket_t *gPkt, FlexPacket_t *fPkt)
{
    uint8_t pkt[8];

    /* GSensorPacket 수신 대기 (100ms 타임아웃) */
    if (osMessageQueueGet(bt_pkt_queue, pkt, NULL, 100) != osOK) return BT_ERR;
    memcpy(gPkt, pkt, 8);

    /* FlexPacket 수신 대기 (100ms 타임아웃) */
    if (osMessageQueueGet(bt_pkt_queue, pkt, NULL, 100) != osOK) return BT_ERR;
    memcpy(fPkt, pkt, 8);

    comm_connected = COMM_CONNECTED_BT;
    return BT_OK;
}
