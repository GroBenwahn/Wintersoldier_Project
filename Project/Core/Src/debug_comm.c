/*
 * debug_comm.c
 *
 * 릴레이 없이 CAN/BT 모드를 강제 설정하는 디버그 모듈.
 * Live Expression: g_debug_comm_mode 값을 1(CAN) 또는 2(BT)로 변경해 런타임 전환.
 *
 * ※ HAL_UART_Abort 사용 금지
 *    huart1.hdmarx 가 DMA 채널과 연결되어 있어 Abort 호출 시
 *    DMA1_Channel2_IRQHandler → HAL_DMA_IRQHandler 에서 HardFault 발생.
 *    UART RX 중단은 _uart_rx_stop() 으로 IT 비트 직접 클리어.
 */

#include "debug_comm.h"
#include "comm_can.h"
#include "bt_comm.h"

#if (DEBUG_COMM_ENABLE)

extern FDCAN_HandleTypeDef hfdcan1;
extern UART_HandleTypeDef  huart1;

volatile CommMode g_debug_comm_mode  = DEBUG_COMM_MODE_DEFAULT;
volatile uint8_t  g_debug_poll_state = 0;
volatile uint8_t  g_debug_bt_step    = 0;

/* UART RX IT 비트 직접 클리어 — DMA 채널을 건드리지 않음 */
static inline void _uart_rx_stop(void)
{
    __HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);
    __HAL_UART_DISABLE_IT(&huart1, UART_IT_PE);
    __HAL_UART_DISABLE_IT(&huart1, UART_IT_ERR);
    huart1.RxState = HAL_UART_STATE_READY;
}

/* ── 공개 API ─────────────────────────────────────── */

/*
 * RTOS 시작 전(main USER CODE BEGIN 2)에 호출.
 * HAL_FDCAN_Stop / HAL_UART_Abort 호출 없이
 * 선택 모드의 HW 초기화만 수행한다.
 *
 * g_debug_bt_step 진행 표시:
 *   100 = 진입
 *   110 = CAN 분기: CAN_Start 전
 *   111 = CAN_Start 완료
 *   112 = currentCommMode=CAN 설정 완료
 *   120 = BT 분기: BT_Init 전
 *   121 = BT_Init 완료
 *   122 = currentCommMode=BT 설정 완료
 *   200 = 함수 정상 종료
 */
/* BT_Init() 안에서 HAL_UART_Receive_IT → ATOMIC_SET_BIT(LDREX/STREX) 실행 중
 * DMA1_Channel2(ADC DMA) 인터럽트가 발생하면 exclusive monitor가 깨져
 * STREX가 계속 실패하는 문제가 있다.
 * BT_Init 구간만 해당 IRQ를 마스킹해 해결한다. */
static inline void _bt_init_safe(void)
{
    HAL_NVIC_DisableIRQ(DMA1_Channel2_IRQn);
    BT_Init();
    HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
}

void Debug_Comm_Init(void)
{
    g_debug_bt_step = 100;

    if (g_debug_comm_mode == COMM_MODE_CAN) {
        g_debug_bt_step = 110;
        CAN_Start();
        g_debug_bt_step = 111;
        localSwitchStatus = 0;
        currentCommMode   = COMM_MODE_CAN;
        g_debug_bt_step   = 112;

    } else if (g_debug_comm_mode == COMM_MODE_BT) {
        g_debug_bt_step = 120;
        _bt_init_safe();
        g_debug_bt_step = 121;
        localSwitchStatus = 1;
        currentCommMode   = COMM_MODE_BT;
        g_debug_bt_step   = 122;
    }

    g_debug_comm_mode = currentCommMode;
    g_debug_bt_step   = 200;
}

/*
 * Mode_Task 100ms 루프에서 호출.
 * g_debug_comm_mode != currentCommMode 일 때만 전환.
 *
 * g_debug_poll_state:
 *   10 = 진입
 *    1 = early return: mode 값이 CAN/BT 아님
 *    2 = early return: 이미 같은 모드
 *    3 = BT 전환 진행 중
 *    4 = CAN 전환 진행 중
 *    5 = 전환 완료
 *
 * g_debug_bt_step (전환 시):
 *   1 = HAL_FDCAN_Stop 전
 *   2 = _uart_rx_stop 전 (BT→CAN) 또는 HAL_FDCAN_Stop 완료 (CAN→BT)
 *   3 = CAN_Start 전 (BT→CAN) 또는 BT_Init 전 (CAN→BT)
 *   4 = 모드 변수 대입 전
 *   5 = 완료
 */
void Debug_Comm_Poll(void)
{
    g_debug_poll_state = 10;

    if (g_debug_comm_mode != COMM_MODE_CAN &&
        g_debug_comm_mode != COMM_MODE_BT) {
        currentCommMode    = COMM_MODE_IDLE;
        g_debug_poll_state = 1;
        return;
    }

    if (g_debug_comm_mode == currentCommMode) {
        g_debug_poll_state = 2;
        return;
    }

    /* 전환 시작: IDLE로 먼저 설정해 CommTask CAN TX 차단 */
    currentCommMode = COMM_MODE_IDLE;

    if (g_debug_comm_mode == COMM_MODE_CAN) {
        /* BT → CAN */
        g_debug_poll_state = 4;
        g_debug_bt_step = 1;
        _uart_rx_stop();
        g_debug_bt_step = 2;
        HAL_FDCAN_Stop(&hfdcan1);
        g_debug_bt_step = 3;
        CAN_Start();
        g_debug_bt_step = 4;
        localSwitchStatus = 0;
        currentCommMode   = COMM_MODE_CAN;
        g_debug_bt_step   = 5;

    } else {
        /* CAN → BT */
        g_debug_poll_state = 3;
        g_debug_bt_step = 1;
        HAL_FDCAN_Stop(&hfdcan1);
        g_debug_bt_step = 2;
        _uart_rx_stop();
        g_debug_bt_step = 3;
        _bt_init_safe();
        g_debug_bt_step = 4;
        localSwitchStatus = 1;
        currentCommMode   = COMM_MODE_BT;
        g_debug_bt_step   = 5;
    }

    g_debug_comm_mode  = currentCommMode;
    g_debug_poll_state = 5;
}

#endif /* DEBUG_COMM_ENABLE */
