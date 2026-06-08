/*
  Comm_Power_Select.c

  동작 요약
    버튼(PB4, PULLUP) 누름
      → EXTI ISR → CommPowerSelect_ButtonPressed() → TimerOnce 50ms 원샷 시작
      → TimerOnce_Callback → CommPowerSelect_DebounceExpired() → 레벨 재확인 후 토글

  토글 상태 (g_comm_toggle)
    홀수(1) : 릴레이 ON  (GPIO HIGH) → BT 모듈 전원 공급  → COMM_MODE_BT
    짝수(0) : 릴레이 OFF (GPIO LOW)  → CAN 트랜시버 전원  → COMM_MODE_CAN

  SPDT 릴레이 배선
    GPIO_Relay_Output = HIGH → COM-NO 연결 → BT 모듈 전원
    GPIO_Relay_Output = LOW  → COM-NC 연결 → CAN 트랜시버 전원
    (반대 배선 시 RELAY_BT_STATE / RELAY_CAN_STATE 값만 교체)
 */

#include "Comm_Power_Select.h"
#include "bt_comm.h"
#include "can_comm.h"

/* ── 릴레이 출력 레벨 정의 (배선에 따라 조정) ── */
#define RELAY_BT_STATE   GPIO_PIN_SET    /* 릴레이 ON  → BT 전원  */
#define RELAY_CAN_STATE  GPIO_PIN_RESET  /* 릴레이 OFF → CAN 전원 */

/* ── 타이밍 정의 ─────────────────────────────── */
#define DEBOUNCE_MS   50    /* 디바운스 대기 시간 */
#define BT_BOOT_MS   100   /* HC-06 전원 인가 후 안정화 대기 */
#define CAN_BOOT_MS   50   /* CAN 트랜시버 안정화 대기 */


/* ── 내부 변수 ─────────────────────────────────── */
static volatile uint8_t g_comm_toggle   = 1;   /* 0=CAN, 1=BT  (기본: BT) */
static volatile uint8_t g_debounce_busy = 0;   /* 디바운스 진행 중 플래그 */

/* TimerOnce 핸들 — main.c 정의 */
extern osTimerId_t TimerOnceHandle;

CommMode currentCommMode  = COMM_MODE_IDLE;
uint8_t  localSwitchStatus = 1;
/* ── 내부 헬퍼 ─────────────────────────────────── */

static void apply_bt_mode(void)
{
    /* 1. 릴레이 ON → BT 모듈 전원 인가 */
    HAL_GPIO_WritePin(GPIO_Relay_Output_GPIO_Port,
                      GPIO_Relay_Output_Pin,
                      RELAY_BT_STATE);

    /* 2. HC-06 부팅 안정화 대기 */
    osDelay(BT_BOOT_MS);

    /* 3. UART 수신 인터럽트 시작 */
    BT_Init();

    /* 4. 모드 확정 */
    localSwitchStatus = 1;   /* 1 = BT 모드 */
    currentCommMode = COMM_MODE_BT;
}

static void apply_can_mode(void)
{
    /* 1. 릴레이 OFF → CAN 트랜시버 전원 인가 */
    HAL_GPIO_WritePin(GPIO_Relay_Output_GPIO_Port,
                      GPIO_Relay_Output_Pin,
                      RELAY_CAN_STATE);

    /* 2. 트랜시버 안정화 대기 */
    osDelay(CAN_BOOT_MS);

    /* 3. FDCAN 시작 (MX_FDCAN1_Init은 main에서 완료됨) */
    CAN_Start();

    /* 4. 모드 확정 */
    localSwitchStatus = 0;   /* 0 = CAN 모드 */
    currentCommMode = COMM_MODE_CAN;
}

/* ── 공개 API ──────────────────────────────────── */

/*
  @brief 시스템 시작 시 초기 상태 설정
        기본값: BT 모드 (릴레이 ON)
        main.c의 USER CODE BEGIN 2 에서 호출
 */
void CommPowerSelect_Init(void)
{
    g_comm_toggle   = 1;
    g_debounce_busy = 0;
    apply_bt_mode();
}

/*
 * @brief EXTI ISR에서 버튼 엣지 감지 시 호출
 *        디바운스 타이머(TimerOnce) 50ms 원샷 시작
 *
 * 호출 위치 (stm32g4xx_it.c 또는 main.c USER CODE BEGIN 4):
 *   void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
 *       if (GPIO_Pin == GPIO_Input_Switch_Pin)
 *           CommPowerSelect_ButtonPressed();
 *   }
 */
void CommPowerSelect_ButtonPressed(void)
{
    if (g_debounce_busy) return;   /* 디바운스 진행 중이면 무시 */

    g_debounce_busy = 1;
    osTimerStart(TimerOnceHandle, DEBOUNCE_MS);
}

/**
 * @brief TimerOnce_Callback에서 50ms 만료 시 호출
 *        실제 핀 레벨 재확인 후 유효한 누름이면 토글 수행
 *
 * 호출 위치 (main.c):
 *   void TimerOnce_Callback(void *argument) {
 *       CommPowerSelect_DebounceExpired();
 *   }
 */
void CommPowerSelect_DebounceExpired(void)
{
    /* 실제 핀 레벨 재확인 (PULLUP → 눌리면 LOW) */
    if (HAL_GPIO_ReadPin(GPIO_Input_Switch_GPIO_Port,
                         GPIO_Input_Switch_Pin) == GPIO_PIN_RESET)
    {
        g_comm_toggle ^= 1;          /* 토글 */
        currentCommMode = COMM_MODE_IDLE;    /* 전환 중 표시 */

        if (g_comm_toggle == 1) {
            apply_bt_mode();         /* 홀수 → BT */
        } else {
            apply_can_mode();        /* 짝수 → CAN */
        }
    }

    g_debounce_busy = 0;
}

