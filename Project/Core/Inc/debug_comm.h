/*
 * debug_comm.h
 *
 * 릴레이 하드웨어 없이 CAN/BT 통신 모드를 강제 설정하는 디버그 모듈
 *
 * [ 사용법 ]
 *   1. DEBUG_COMM_ENABLE 1로 설정
 *   2. DEBUG_COMM_MODE_DEFAULT 로 초기 모드 설정
 *   3. main.c USER CODE BEGIN 2:
 *        CommPowerSelect_Init();  ← 주석 처리 (자동으로 #if 분기됨)
 *   4. STM32CubeIDE Live Expression 창에서:
 *        g_debug_comm_mode 를 추가
 *        값을 0(IDLE) / 1(CAN) / 2(BT) 로 변경 → 100ms 내 자동 전환
 *   5. 실사용 전환: DEBUG_COMM_ENABLE 0으로 변경
 */

#ifndef INC_DEBUG_COMM_H_
#define INC_DEBUG_COMM_H_

#include "config.h"

/* ── 설정 ──────────────────────────────────────────── */
#define DEBUG_COMM_ENABLE        1              /* 1=활성화, 0=비활성화    */
#define DEBUG_COMM_MODE_DEFAULT  COMM_MODE_CAN  /* 초기값 (런타임 변경 가능) */

/* ── Live Expression 대상 변수 ──────────────────────
 *  디버거에서 이 변수에 아래 값을 직접 써서 모드 전환:
 *    0 = COMM_MODE_IDLE  (전환 없음)
 *    1 = COMM_MODE_CAN
 *    2 = COMM_MODE_BT
 * ──────────────────────────────────────────────────── */
#if (DEBUG_COMM_ENABLE)
extern volatile CommMode g_debug_comm_mode;
#endif

/* ── API ────────────────────────────────────────────── */
#if (DEBUG_COMM_ENABLE)

void Debug_Comm_Init(void);
void Debug_Comm_ForceMode(CommMode mode);

/* Mode_Task 루프에서 100ms마다 호출
 * g_debug_comm_mode 변경 감지 → 자동 전환 */
void Debug_Comm_Poll(void);

#endif /* DEBUG_COMM_ENABLE */

#endif /* INC_DEBUG_COMM_H_ */
