#ifndef BT_COMM_H
#define BT_COMM_H

#include "main.h"
#include "cmsis_os.h"
#include <stdint.h>

/* ── 패킷 프레임 정의 ─────────────────────────────────
 * [SOF(1)] [ID(1)] [LEN(1)] [PAYLOAD(8)] [CHECKSUM(1)] [EOF(1)]
 * 총 13 bytes 고정
 * ──────────────────────────────────────────────────── */
#define BT_SOF            0xAA
#define BT_EOF            0x55
#define BT_PAYLOAD_SIZE   8
#define BT_FRAME_SIZE     13    /* 1+1+1+8+1+1 */

typedef struct {
    uint8_t sof;
    uint8_t id;
    uint8_t len;
    uint8_t payload[BT_PAYLOAD_SIZE];
    uint8_t checksum;
    uint8_t eof;
} BT_Packet_t;

/* ── 패킷 ID 정의 (CAN ID 체계와 동일) ──────────────── */
/* 리모콘 보드 송신 */
#define BT_ID_REMOTE_SENSOR   0x10    /* 10ms: 굽힘센서 + 자이로 */

/* 로봇팔 보드 송신 */
#define BT_ID_ROBOT_MOTOR_1   0x30    /* 10ms: 모터1~3 PWM */
#define BT_ID_ROBOT_MOTOR_2   0x40    /* 10ms: 모터4~6 PWM */
#define BT_ID_ROBOT_STATUS    0x50    /* 100ms: 로봇팔 상태 */

/* ── 연결 상태 ──────────────────────────────────────── */
typedef enum {
    BT_STATE_DISCONNECTED = 0,
    BT_STATE_CONNECTING,
    BT_STATE_CONNECTED
} BT_ConnState_t;

extern volatile BT_ConnState_t g_bt_conn_state;

/* ── API ─────────────────────────────────────────────── */
void    BT_Init(void);
void    BT_Pack_And_Send(uint8_t id, const uint8_t *data, uint8_t len);
uint8_t BT_Receive_Packet(BT_Packet_t *out);
uint8_t BT_IsPaired(void);
void    BT_ConnectionMonitor_100ms(void);   /* Mode_Task에서 호출 */

/* HAL 콜백 — bt_comm.c 내부 구현 */
/* void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart); */

extern UART_HandleTypeDef huart1;
extern osMessageQueueId_t BT_QueueHandle;

#endif /* BT_COMM_H */
