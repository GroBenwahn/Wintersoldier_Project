/*
 * debug_comm_select.c
 *
 * debug_comm 에 PB4 폴링 기반 버튼 감지를 추가한 테스트 모듈.
 * 릴레이 없음.
 *
 * 동작 흐름:
 *   버튼(PB4) 또는 Live Expression 으로 g_debug_comm_mode 변경
 *   → Debug_Comm_Poll() 이 g_debug_comm_mode vs currentCommMode 비교 후 HW 전환
 *
 * PB4 (GPIO_Input_Switch) 는 EXTI 불가 핀이므로
 * 100ms 폴링 + 3회 연속(=300ms) 디바운스로 버튼 감지.
 */

#include "debug_comm_select.h"

#if (DEBUG_COMM_ENABLE)

/* ── 진단 변수 (Live Expression) ── */
volatile uint8_t g_dcs_btn_raw  = 1;   /* PULLUP 기본값: HIGH */
volatile uint8_t g_dcs_stab_cnt = 0;

/* ── 공개 API ── */

void Debug_CommSelect_Init(void)
{
    /* 초기화 경로는 기존 debug_comm 과 동일 */
    Debug_Comm_Init();
}

/*
 * Mode_Task 100ms 루프에서 호출 (Debug_Comm_Poll 대신 사용).
 *
 * 1. PB4 폴링 → 누름 확정 시 g_debug_comm_mode 토글 (CAN↔BT)
 *    (Live Expression 으로 직접 값을 바꿔도 같은 경로로 전환됨)
 * 2. Debug_Comm_Poll() 이 g_debug_comm_mode 를 보고 실제 HW 전환
 * 3. 전환 완료 후 2초 쿨다운 — 모드 전환 직후 버튼 재전환 방지
 *    (Live Expression 직접 변경은 쿨다운 무관하게 동작)
 */
void Debug_CommSelect_Apply(void)
{
    static uint8_t prev_btn = 1;
    static uint8_t btn_lock = 0;   /* 홀드 중 중복 토글 방지 */
    static uint8_t cooldown = 0;   /* 전환 후 재전환 방지 (100ms 단위) */

    uint8_t cur_btn = (HAL_GPIO_ReadPin(GPIO_Input_Switch_GPIO_Port,
                                        GPIO_Input_Switch_Pin) == GPIO_PIN_RESET) ? 0 : 1;
    g_dcs_btn_raw = cur_btn;

    /* 쿨다운 중: 버튼 토글만 비활성화
     * Live Expression(g_debug_comm_mode 직접 변경)은 항상 유효           */
    if (cooldown > 0) {
        cooldown--;
    } else {
        /* ── 버튼 폴링 (PB4, PULLUP) ──
         *   누름 = GPIO_PIN_RESET(LOW),  해제 = HIGH
         *   3회 연속(=300ms) 동일 값 확인 후 에지 처리                   */
        if (cur_btn != prev_btn) {
            if (++g_dcs_stab_cnt >= 3) {
                g_dcs_stab_cnt = 0;
                prev_btn       = cur_btn;

                if (cur_btn == 0 && !btn_lock) {    /* 눌림 확정 */
                    btn_lock = 1;
                    g_debug_comm_mode = (g_debug_comm_mode == COMM_MODE_CAN)
                                        ? COMM_MODE_BT : COMM_MODE_CAN;
                }
                if (cur_btn == 1) btn_lock = 0;     /* 릴리즈 확정 */
            }
        } else {
            g_dcs_stab_cnt = 0;
        }
    }

    /* g_debug_comm_mode vs currentCommMode 비교 후 HW 전환
     * 전환 발생 시 쿨다운 2초 + 버튼 상태 초기화                         */
    CommMode mode_before = currentCommMode;
    Debug_Comm_Poll();
    if (currentCommMode != mode_before) {
        cooldown = 20;   /* 2초 = 100ms × 20 */
        btn_lock = 0;
        prev_btn = 1;
    }
}

#endif /* DEBUG_COMM_ENABLE */
