#ifndef BT_COMM_H
#define BT_COMM_H

#include "config.h"   /* BT_ConnState_t, extern 선언 모두 여기서 */

/* ── 패킷 프레임 정의 ────────────────────────────────
 * [SOF(1)] [ID(1)] [LEN(1)] [PAYLOAD(8)] [CHECKSUM(1)] [EOF(1)]
 * 총 13 bytes 고정
 * ──────────────────────────────────────────────────── */
#define BT_SOF            0xAA
#define BT_EOF            0x55
#define BT_PAYLOAD_SIZE   8
#define BT_FRAME_SIZE     13

typedef struct {
    uint8_t sof;
    uint8_t id;
    uint8_t len;
    uint8_t payload[BT_PAYLOAD_SIZE];
    uint8_t checksum;
    uint8_t eof;
} BT_Packet_t;

/* ── 패킷 ID 정의 ────────────────────────────────────
 * 리모콘 → 로봇팔 (단방향)
 * ──────────────────────────────────────────────────── */
#define BT_ID_REMOTE_SENSOR   0x10    /* 10ms: 굽힘센서 + 자이로 */

/* ── 진단 변수 (Live Expression) ──────────────────────
 *   DIAG_bt_state: 0=DISCONNECTED  1=CONNECTING  2=CONNECTED */
extern volatile uint8_t DIAG_bt_state;

/* ── API ─────────────────────────────────────────────*/
void    BT_Init(void);
void    BT_Pack_And_Send(uint8_t id, const uint8_t *data, uint8_t len);
uint8_t BT_Receive_Packet(BT_Packet_t *out);
uint8_t BT_IsPaired(void);
void    BT_ConnectionMonitor_100ms(void);

#endif /* BT_COMM_H */
