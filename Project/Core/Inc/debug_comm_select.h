/*
 * debug_comm_select.h
 *
 * debug_comm 에 PB4 폴링 기반 버튼 감지를 추가한 테스트 모듈.
 *
 * [ 사용법 ]
 *   debug_comm.h 의 DEBUG_COMM_ENABLE / DEBUG_COMM_MODE_DEFAULT 그대로 사용.
 *   main.c 에서:
 *     Debug_Comm_Init()  → Debug_CommSelect_Init()  으로 교체
 *     Debug_Comm_Poll()  → Debug_CommSelect_Apply() 으로 교체
 *
 * [ 모드 전환 경로 — 둘 다 동작 ]
 *   1. Live Expression: g_debug_comm_mode 값 직접 변경 (1=CAN, 2=BT)
 *   2. 물리 버튼(PB4): 누르면 CAN↔BT 토글 (300ms 폴링 디바운스)
 *
 * [ Live Expression 추가 목록 ]
 *   g_debug_comm_mode  — 현재 모드 요청값 (변경으로 전환)
 *   g_debug_poll_state — Poll 경로 진단
 *   g_debug_bt_step    — 전환 단계 진단
 *   g_dcs_btn_raw      — PB4 GPIO 현재값 (0=눌림, 1=해제)
 *   g_dcs_stab_cnt     — 디바운스 연속 카운터 (3 도달 시 전환)
 */

#ifndef INC_DEBUG_COMM_SELECT_H_
#define INC_DEBUG_COMM_SELECT_H_

#include "debug_comm.h"   /* DEBUG_COMM_ENABLE, g_debug_comm_mode 재사용 */

#if (DEBUG_COMM_ENABLE)

extern volatile uint8_t g_dcs_btn_raw;   /* PB4 현재 GPIO 값 */
extern volatile uint8_t g_dcs_stab_cnt;  /* 디바운스 연속 카운터 */

void Debug_CommSelect_Init(void);    /* Debug_Comm_Init 래핑 */
void Debug_CommSelect_Apply(void);   /* 버튼 폴링 + Debug_Comm_Poll 통합 호출 */

#endif /* DEBUG_COMM_ENABLE */
#endif /* INC_DEBUG_COMM_SELECT_H_ */
