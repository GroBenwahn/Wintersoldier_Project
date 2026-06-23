/*
  Comm_Power_Select.c  (릴레이 없음 — 소프트웨어 전환)

  동작 요약
    1. 전원 인가 → BT 모드 (기본값) : HC-05/06 자동 연결
    2. 버튼 1회  → BT 차단, CAN 시작
    3. 버튼 2회  → CAN 차단, BT 재시작 : HC-05/06 자동 재연결

  토글 상태 (g_comm_toggle)
    1 : BT 모드  → UART 수신 활성,  CAN 중지
    0 : CAN 모드 → UART 수신 중지,  CAN 활성
 */

#include "Comm_Power_Select.h"
#include "bt_comm.h"
#include "comm_can.h"
#include "timers.h"

extern UART_HandleTypeDef  huart1;
extern FDCAN_HandleTypeDef hfdcan1;

/* ── 내부 변수 ─────────────────────────────────── */
static volatile uint8_t g_comm_toggle         = 1;
static volatile uint8_t g_debounce_busy       = 0;
static volatile uint8_t g_mode_change_pending = 0;

extern osTimerId_t TimerOnceHandle;

CommMode currentCommMode   = COMM_MODE_IDLE;
uint8_t  localSwitchStatus = 1;

/* ── 내부 헬퍼 ─────────────────────────────────── */

static void apply_bt_mode(void)
{
    /* 1. CAN 중지 */
    HAL_FDCAN_Stop(&hfdcan1);

    /* 2. BT UART 수신 시작 → HC-05 자동 연결 */
    BT_Init();

    /* 3. 모드 확정 */
    localSwitchStatus = 1;
    currentCommMode   = COMM_MODE_BT;
}

static void apply_can_mode(void)
{
    /* 1. BT UART 수신 중지 + 연결 상태 초기화 */
    HAL_UART_Abort(&huart1);
    g_bt_conn_state = BT_STATE_DISCONNECTED;

    /* 2. CAN 시작 */
    CAN_Start();

    /* 3. 모드 확정 */
    localSwitchStatus = 0;
    currentCommMode   = COMM_MODE_CAN;
}

/* ── 공개 API ──────────────────────────────────── */

void CommPowerSelect_Init(void)
{
    g_comm_toggle   = 1;
    g_debounce_busy = 0;
    apply_bt_mode();
}

void CommPowerSelect_ButtonPressed(void)
{
    if (g_debounce_busy) return;

    g_debounce_busy = 1;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTimerStartFromISR(TimerOnceHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void CommPowerSelect_DebounceExpired(void)
{
    if (HAL_GPIO_ReadPin(GPIO_Input_Switch_GPIO_Port,
                         GPIO_Input_Switch_Pin) == GPIO_PIN_RESET)
    {
        g_comm_toggle ^= 1;
        currentCommMode       = COMM_MODE_IDLE;
        g_mode_change_pending = 1;
    }
    g_debounce_busy = 0;
}

void CommPowerSelect_Apply(void)
{
    if (!g_mode_change_pending) return;
    g_mode_change_pending = 0;

    if (g_comm_toggle == 1)
        apply_bt_mode();
    else
        apply_can_mode();
}

