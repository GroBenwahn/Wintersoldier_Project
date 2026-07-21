#include "servo.h"

/* Waist 이동 속도: ServoTask 1회(20ms)당 1도 이동 = 50도/초 */
#define WAIST_STEP  1

/* 현재 서보 출력 각도 (-90~+90도), LcdTask에서 읽음 */
int8_t servo_angles[6] = {0};

/* Waist 현재 누적 위치 */
static int8_t waist_pos = 0;


/* 각도(-90~+90) → PWM CCR 변환 (PSC=169, ARR=19999 기준, 1μs/tick) */
static uint32_t angle_to_ccr(int8_t angle)
{
    int32_t ccr = 1500 + (int32_t)angle * 1000 / 90;
    if (ccr < 500)  ccr = 500;
    if (ccr > 2500) ccr = 2500;
    return (uint32_t)ccr;
}


/* int32_t → int8_t 클램프 */
static int8_t clamp8(int32_t val, int8_t min_val, int8_t max_val)
{
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return (int8_t)val;
}


/* TIM 채널에 각도 출력 */
static void SetPWM(TIM_HandleTypeDef *htim, uint32_t channel, int8_t angle)
{
    __HAL_TIM_SET_COMPARE(htim, channel, angle_to_ccr(angle));
}


/* PWM 6채널 시작 + 전체 서보 중립(0도) 초기화 */
void Servo_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);  /* Waist       */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);  /* Shoulder    */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);  /* Elbow       */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);  /* Wrist Roll  */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);  /* Wrist Pitch */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);  /* Grip        */

    SetPWM(&htim3, TIM_CHANNEL_2, 0);
    SetPWM(&htim3, TIM_CHANNEL_1, 0);
    SetPWM(&htim3, TIM_CHANNEL_3, 0);
    SetPWM(&htim1, TIM_CHANNEL_1, 0);
    SetPWM(&htim1, TIM_CHANNEL_2, 0);
    SetPWM(&htim2, TIM_CHANNEL_1, 0);
}


/* 패킷 → 서보 각도 계산 → PWM 출력 (ServoTask에서 20ms마다 호출) */
void Servo_Update(const volatile GSensorPacket_t *gPkt, const volatile FlexPacket_t *fPkt)
{
    /* ── Waist: Switch Up/Down으로 누적 이동 ── */
    switch ((HeightSwitch_t)gPkt->height_sw)
    {
        case HEIGHT_UP:
            if (waist_pos < 90) waist_pos += WAIST_STEP;
            break;
        case HEIGHT_DOWN:
            if (waist_pos > -90) waist_pos -= WAIST_STEP;
            break;
        default:
            break;
    }
    servo_angles[SERVO_WAIST] = waist_pos;
    SetPWM(&htim3, TIM_CHANNEL_2, waist_pos);

    /* ── Shoulder: Flex 3 (0~4095 → 0~+90도) ── */
    int8_t shoulder = clamp8((int32_t)fPkt->flex3 * 90 / 4095, 0, 90);
    servo_angles[SERVO_SHOULDER] = shoulder;
    SetPWM(&htim3, TIM_CHANNEL_1, shoulder);

    /* ── Elbow: Flex 2 (0~4095 → 0~+90도) ── */
    int8_t elbow = clamp8((int32_t)fPkt->flex2 * 90 / 4095, 0, 90);
    servo_angles[SERVO_ELBOW] = elbow;
    SetPWM(&htim3, TIM_CHANNEL_3, elbow);

    /* ── Wrist Roll: G Roll (-1800~+1800 → -90~+90도) ── */
    int8_t wrist_roll = clamp8(gPkt->roll / 20, -90, 90);
    servo_angles[SERVO_WRIST_ROLL] = wrist_roll;
    SetPWM(&htim1, TIM_CHANNEL_1, wrist_roll);

    /* ── Wrist Pitch: G Pitch (-1800~+1800 → -90~+90도) ── */
    int8_t wrist_pitch = clamp8(gPkt->pitch / 20, -90, 90);
    servo_angles[SERVO_WRIST_PITCH] = wrist_pitch;
    SetPWM(&htim1, TIM_CHANNEL_2, wrist_pitch);

    /* ── Grip: Flex 1 (0~4095 → 0~+90도) ── */
    int8_t grip = clamp8((int32_t)fPkt->flex1 * 90 / 4095, 0, 90);
    servo_angles[SERVO_GRIP] = grip;
    SetPWM(&htim2, TIM_CHANNEL_1, grip);
}
