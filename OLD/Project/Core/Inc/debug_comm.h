/*
 * debug_comm.h
 *
 * [ 사용법 ]
 *   1. DEBUG_COMM_ENABLE 1
 *   2. DEBUG_COMM_MODE_DEFAULT 로 초기 모드 설정
 *   3. Live Expressions 추가:
 *        g_debug_comm_mode   — 값 변경으로 런타임 모드 전환 (1=CAN, 2=BT)
 *        g_debug_poll_state  — Poll 경로 확인
 *        g_debug_bt_step     — 전환 진행 단계 확인
 *   4. 실사용 전환: DEBUG_COMM_ENABLE 0
 *
 * [ 하드웨어 전제조건 ]
 *   CAN 테스트: SN65HVD230 VCC → 릴레이 우회, 3.3V 직결
 *   BT  테스트: HC-06    VCC → 릴레이 우회, 3.3V 직결
 */

#ifndef INC_DEBUG_COMM_H_
#define INC_DEBUG_COMM_H_

#include "config.h"

#define DEBUG_COMM_ENABLE        1
#define DEBUG_COMM_MODE_DEFAULT  COMM_MODE_CAN

#if (DEBUG_COMM_ENABLE)

extern volatile CommMode g_debug_comm_mode;   /* Live Expression 전환 대상 */
extern volatile uint8_t  g_debug_poll_state;  /* Poll 경로 진단 */
extern volatile uint8_t  g_debug_bt_step;     /* Init/전환 단계 진단 */

void Debug_Comm_Init(void);
void Debug_Comm_Poll(void);

#endif /* DEBUG_COMM_ENABLE */
#endif /* INC_DEBUG_COMM_H_ */
