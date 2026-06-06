/*
 * 연결 흐름
 *   BT_Init() 호출
 *     → UART RX 인터럽트 시작
 *     → g_bt_conn_state = BT_STATE_CONNECTING
 *     → Master HC-05가 자동으로 Slave(HC-06)에 연결 시도
 *     → 첫 UART 수신 발생 시 → BT_STATE_CONNECTED
 *
 * 연결 감지
 *   g_last_rx_tick 기준으로 BT_CONN_TIMEOUT_MS 이내 수신 있으면 CONNECTED
 *   타임아웃 시 DISCONNECTED (BT 모드는 유지, 버튼으로만 전환)

 */

#include "bt_comm.h"
#include <string.h>

/* ── 타이밍 ─────────────────────────────────────────── */
#define BT_CONN_TIMEOUT_MS   3000   /* 3초 수신 없으면 DISCONNECTED */
#define BT_TX_TIMEOUT_MS     5   /* 115200bps 기준 13byte ≈ 1.1ms, 여유 5ms */

/* ── 연결 상태 전역 ─────────────────────────────────── */
volatile BT_ConnState_t g_bt_conn_state = BT_STATE_DISCONNECTED;
static   uint32_t       g_last_rx_tick  = 0;

/* ── RX 단일 바이트 버퍼 (HAL IT용) ────────────────── */
static uint8_t g_rx_byte;

/* ── RX 상태머신 ────────────────────────────────────── */
typedef enum {
    PARSE_SOF = 0,
    PARSE_ID,
    PARSE_LEN,
    PARSE_PAYLOAD,
    PARSE_CHECKSUM,
    PARSE_EOF
} RxParseState_t;

static RxParseState_t g_parse_state = PARSE_SOF;
static BT_Packet_t    g_rx_pkt;
static uint8_t        g_payload_idx = 0;

/* ── 완성 패킷 슬롯 ─────────────────────────────────── */
static volatile uint8_t  g_pkt_ready = 0;
static          BT_Packet_t g_ready_pkt;

/* ── 내부 헬퍼 ──────────────────────────────────────── */

static uint8_t calc_checksum(const BT_Packet_t *p)
{
    uint8_t cs = p->id ^ p->len;
    for (uint8_t i = 0; i < BT_PAYLOAD_SIZE; i++)
        cs ^= p->payload[i];
    return cs;
}

static void parser_reset(void)
{
    g_parse_state = PARSE_SOF;
    g_payload_idx = 0;
    memset(&g_rx_pkt, 0, sizeof(BT_Packet_t));
}

static void parse_byte(uint8_t byte)
{
    switch (g_parse_state) {

        case PARSE_SOF:
            if (byte == BT_SOF) {
                g_rx_pkt.sof  = byte;
                g_parse_state = PARSE_ID;
            }
            break;

        case PARSE_ID:
            g_rx_pkt.id   = byte;
            g_parse_state = PARSE_LEN;
            break;

        case PARSE_LEN:
            g_rx_pkt.len  = byte;
            g_payload_idx = 0;
            memset(g_rx_pkt.payload, 0, BT_PAYLOAD_SIZE);
            g_parse_state = PARSE_PAYLOAD;
            break;

        case PARSE_PAYLOAD:
            if (g_payload_idx < BT_PAYLOAD_SIZE)
                g_rx_pkt.payload[g_payload_idx++] = byte;
            if (g_payload_idx >= BT_PAYLOAD_SIZE)
                g_parse_state = PARSE_CHECKSUM;
            break;

        case PARSE_CHECKSUM:
            g_rx_pkt.checksum = byte;
            g_parse_state     = PARSE_EOF;
            break;

        case PARSE_EOF:
            if (byte == BT_EOF) {
                /* 체크섬 검증 */
                if (calc_checksum(&g_rx_pkt) == g_rx_pkt.checksum) {
                    g_rx_pkt.eof = byte;
                    /* 완성 패킷 복사 */
                    memcpy((void *)&g_ready_pkt, &g_rx_pkt,
                           sizeof(BT_Packet_t));
                    g_pkt_ready = 1;

                    /* 연결 감지 — 첫 수신 시 CONNECTED */
                    g_last_rx_tick  = HAL_GetTick();
                    g_bt_conn_state = BT_STATE_CONNECTED;
                }
            }
            parser_reset();   /* 성공/실패 무관하게 리셋 */
            break;
    }
}

/* ── 공개 API ───────────────────────────────────────── */

/**
 * @brief BT 초기화 — Comm_Power_Select.c의 apply_bt_mode()에서 호출
 *        UART RX IT 시작, 상태머신 초기화
 */
void BT_Init(void)
{
    parser_reset();
    g_pkt_ready     = 0;
    g_last_rx_tick  = 0;
    g_bt_conn_state = BT_STATE_CONNECTING;

    /* 1바이트 단위 수신 인터럽트 시작 */
    HAL_UART_Receive_IT(&huart1, &g_rx_byte, 1);
}

/**
 * @brief 패킷 빌드 후 UART 즉시 전송
 *        Comm_Task의 10ms / 100ms 블록에서 호출
 *
 * @param id    BT_ID_xxx 정의 참고
 * @param data  페이로드 포인터 (NULL 가능, len=0)
 * @param len   페이로드 길이 (최대 BT_PAYLOAD_SIZE)
 */
void BT_Pack_And_Send(uint8_t id, const uint8_t *data, uint8_t len)
{
    BT_Packet_t pkt;

    pkt.sof = BT_SOF;
    pkt.id  = id;
    pkt.len = (len > BT_PAYLOAD_SIZE) ? BT_PAYLOAD_SIZE : len;
    memset(pkt.payload, 0, BT_PAYLOAD_SIZE);
    if (data && pkt.len > 0)
        memcpy(pkt.payload, data, pkt.len);
    pkt.checksum = calc_checksum(&pkt);
    pkt.eof      = BT_EOF;

    HAL_UART_Transmit(&huart1,
                      (uint8_t *)&pkt,
                      BT_FRAME_SIZE,
                      BT_TX_TIMEOUT_MS);
}

/**
 * @brief 수신된 완성 패킷 꺼내기
 *        Comm_Task / Servo_Task에서 폴링으로 호출
 *
 * @param out  수신 패킷 저장 포인터
 * @return     1: 새 패킷 있음, 0: 없음
 */
uint8_t BT_Receive_Packet(BT_Packet_t *out)
{
    if (!g_pkt_ready) return 0;
    memcpy(out, (const void *)&g_ready_pkt, sizeof(BT_Packet_t));
    g_pkt_ready = 0;
    return 1;
}

/**
 * @brief 연결 상태 반환
 * @return 1: CONNECTED, 0: DISCONNECTED / CONNECTING
 */
uint8_t BT_IsPaired(void)
{
    return (g_bt_conn_state == BT_STATE_CONNECTED) ? 1 : 0;
}

/**
 * @brief Mode_Task 100ms 루프에서 호출 — 연결 타임아웃 감지
 *        3초 이상 수신 없으면 DISCONNECTED (BT 모드는 유지)
 */
void BT_ConnectionMonitor_100ms(void)
{
    if (g_bt_conn_state == BT_STATE_DISCONNECTED) return;

    if (g_last_rx_tick == 0) return;   /* 아직 한 번도 수신 안 됨 */

    if ((HAL_GetTick() - g_last_rx_tick) >= BT_CONN_TIMEOUT_MS) {
        g_bt_conn_state = BT_STATE_DISCONNECTED;
        /* BT 모드 유지 — 버튼으로만 전환 가능 */
    }
}

/* ── HAL UART 콜백 ──────────────────────────────────── */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        parse_byte(g_rx_byte);
        /* 다음 1바이트 수신 재등록 */
        HAL_UART_Receive_IT(&huart1, &g_rx_byte, 1);
    }
}
