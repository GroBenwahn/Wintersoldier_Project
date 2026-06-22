/*
 * debug_comm.c
 *
 * 릴레이 하드웨어 없이 CAN/BT 통신 모드를 강제 설정하는 디버그 모듈
 *
 * ※ 하드웨어 전제조건 (코드로 해결 불가):
 *    CAN 테스트: SN65HVD230 VCC → 릴레이 우회, 3.3V 직결
 *    BT  테스트: HC-06 VCC      → 릴레이 우회, 3.3V 직결
 */

#include "debug_comm.h"
#include "comm_can.h"
#include "bt_comm.h"

#if (DEBUG_COMM_ENABLE)

extern FDCAN_HandleTypeDef hfdcan1;
extern osSemaphoreId_t CAN_SemHandle;

/* ── Live Expression 대상 변수 ─────────────────────────
 * 디버거에서 이 변수 값을 직접 변경:
 *   1 = COMM_MODE_CAN, 2 = COMM_MODE_BT, 0 = 변경 없음
 * ──────────────────────────────────────────────────── */
volatile CommMode g_debug_comm_mode = DEBUG_COMM_MODE_DEFAULT;

/* ── 내부 구현 ─────────────────────────────────────── */

static void _force_can_mode(void)
{
    /* 이미 시작된 경우 정지 후 재시작 (이중 시작 방지) */
    HAL_FDCAN_Stop(&hfdcan1);
    CAN_Start();
    localSwitchStatus = 0;
    currentCommMode   = COMM_MODE_CAN;
}

static void _force_bt_mode(void)
{
    /* CAN 수신 인터럽트 잔존 방지 */
    HAL_FDCAN_Stop(&hfdcan1);
    HAL_Delay(100);
    BT_Init();
    localSwitchStatus = 1;
    currentCommMode   = COMM_MODE_BT;
}

/* ── 공개 API ──────────────────────────────────────── */

void Debug_Comm_Init(void)
{
    /* RTOS 시작 전 호출 — 세마포어 없이 직접 전환 */
    currentCommMode = COMM_MODE_IDLE;

    if (g_debug_comm_mode == COMM_MODE_CAN) {
        _force_can_mode();
    } else if (g_debug_comm_mode == COMM_MODE_BT) {
        _force_bt_mode();
    }

    g_debug_comm_mode = currentCommMode;
}

void Debug_Comm_ForceMode(CommMode mode)
{
    /* Comm_Task가 FDCAN을 동시에 사용하지 못하도록 세마포어 선점
     * (HAL_FDCAN_Stop과 AddMessageToTxFifoQ 동시 접근 → HardFault 방지) */
    osSemaphoreAcquire(CAN_SemHandle, osWaitForever);

    currentCommMode = COMM_MODE_IDLE;

    if (mode == COMM_MODE_CAN) {
        _force_can_mode();
    } else if (mode == COMM_MODE_BT) {
        _force_bt_mode();
    }

    /* 전환 완료 후 g_debug_comm_mode를 currentCommMode와 동기화
     * (IDLE 등 잘못된 값 입력 시 Poll이 무한 재전환하는 것 방지) */
    g_debug_comm_mode = currentCommMode;

    osSemaphoreRelease(CAN_SemHandle);
}

void Debug_Comm_Poll(void)
{
    /* CAN(1), BT(2) 만 유효한 전환 대상 */
    if (g_debug_comm_mode != COMM_MODE_CAN &&
        g_debug_comm_mode != COMM_MODE_BT) {
        return;
    }

    if (g_debug_comm_mode != currentCommMode) {
        Debug_Comm_ForceMode(g_debug_comm_mode);
    }
}

#endif /* DEBUG_COMM_ENABLE */
