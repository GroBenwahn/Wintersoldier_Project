#ifndef __SERVO_H
#define __SERVO_H

#include "main.h"
#include "Packet.h"
#include <stdint.h>

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;

/* 서보 인덱스 */
#define SERVO_WAIST        0   /* TIM3_CH2 (PA4)  MG996R - Switch */
#define SERVO_SHOULDER     1   /* TIM3_CH1 (PA6)  MG996R - Flex 3 */
#define SERVO_ELBOW        2   /* TIM3_CH3 (PB0)  MG996R - Flex 2 */
#define SERVO_WRIST_ROLL   3   /* TIM1_CH1 (PA8)  SG90   - G Roll */
#define SERVO_WRIST_PITCH  4   /* TIM1_CH2 (PA9)  SG90   - G Pitch */
#define SERVO_GRIP         5   /* TIM2_CH1 (PA5)  SG90   - Flex 1 */

/* 현재 서보 출력 각도 배열 (-90~+90도), LcdTask에서 읽음 */
extern int8_t servo_angles[6];

void Servo_Init(void);
void Servo_Update(const volatile GSensorPacket_t *gPkt, const volatile FlexPacket_t *fPkt);

#endif /* __SERVO_H */
