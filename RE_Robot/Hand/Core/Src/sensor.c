#include "sensor.h"
#include <math.h>

/* ADXL345 I2C 주소 (SDO=GND 기준, 7bit=0x53, HAL용 8bit=0xA6) */
#define ADXL345_ADDR        (0x53 << 1)

/* ADXL345 레지스터 */
#define ADXL345_DATA_FORMAT 0x31
#define ADXL345_BW_RATE     0x2C
#define ADXL345_POWER_CTL   0x2D
#define ADXL345_INT_ENABLE  0x2E
#define ADXL345_INT_MAP     0x2F
#define ADXL345_DATAX0      0x32

/* Flex 변경 감지 임계값 (ADC 노이즈 필터) */
#define FLEX_THRESHOLD      20

/* 센서 변경 플래그 */
volatile uint8_t gsensor_changed = 0;
volatile uint8_t flex_changed    = 0;
volatile uint8_t switch_changed  = 0;

/* ADC DMA 버퍼 */
static uint16_t adc_buf[2]   = {0};
static uint16_t prev_flex[2] = {0};


/* ADXL345 레지스터 1바이트 쓰기 */
static void ADXL345_WriteReg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    HAL_I2C_Master_Transmit(&hi2c1, ADXL345_ADDR, buf, 2, 10);
}

/* ADXL345 초기화 */
static void ADXL345_Init(void)
{
    ADXL345_WriteReg(ADXL345_DATA_FORMAT, 0x08); /* 풀리졸루션, ±2g */
    ADXL345_WriteReg(ADXL345_BW_RATE,     0x0A); /* 출력속도 100Hz */
    ADXL345_WriteReg(ADXL345_INT_MAP,     0x00); /* 모든 인터럽트 → INT1 */
    ADXL345_WriteReg(ADXL345_INT_ENABLE,  0x80); /* DATA_READY 인터럽트 활성화 */
    ADXL345_WriteReg(ADXL345_POWER_CTL,   0x08); /* 측정 시작 */
}

/* 센서 초기화 */
void Sensor_Init(void)
{
    ADXL345_Init();
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, 2);
}

/* 전체 센서 읽기 + 패킷 조립 */
void Sensor_ReadAll(GSensorPacket_t *gPkt, FlexPacket_t *fPkt)
{
    /* --- G센서 읽기 --- */
    uint8_t raw[6] = {0};
    uint8_t reg = ADXL345_DATAX0;
    HAL_I2C_Master_Transmit(&hi2c1, ADXL345_ADDR, &reg, 1, 10);
    HAL_I2C_Master_Receive (&hi2c1, ADXL345_ADDR, raw,  6, 10);

    int16_t ax = (int16_t)((raw[1] << 8) | raw[0]);
    int16_t ay = (int16_t)((raw[3] << 8) | raw[2]);
    int16_t az = (int16_t)((raw[5] << 8) | raw[4]);

    float fax = ax * 0.004f; /* 풀리졸루션 4mg/LSB → g 단위 */
    float fay = ay * 0.004f;
    float faz = az * 0.004f;

    int16_t roll  = (int16_t)(atan2f(fay, faz)                      * (1800.0f / 3.14159f));
    int16_t pitch = (int16_t)(atan2f(-fax, sqrtf(fay*fay + faz*faz)) * (1800.0f / 3.14159f));

    /* --- 스위치 읽기 --- */
    uint8_t up   = (HAL_GPIO_ReadPin(Up_Switch_GPIO_Port,   Up_Switch_Pin)   == GPIO_PIN_RESET) ? 1 : 0;
    uint8_t down = (HAL_GPIO_ReadPin(Down_Switch_GPIO_Port, Down_Switch_Pin) == GPIO_PIN_RESET) ? 1 : 0;

    HeightSwitch_t sw;
    if      (up)   sw = HEIGHT_UP;
    else if (down) sw = HEIGHT_DOWN;
    else           sw = HEIGHT_STOP;

    /* --- GSensorPacket 조립 --- */
    gPkt->roll      = roll;
    gPkt->pitch     = pitch;
    gPkt->height_sw = (uint8_t)sw;
    gPkt->reserved[0] = 0;
    gPkt->reserved[1] = 0;
    gPkt->checksum  = CALC_CHECKSUM(gPkt);

    /* --- FlexPacket 조립 --- */
    fPkt->flex1 = adc_buf[0];
    fPkt->flex2 = adc_buf[1];
    fPkt->reserved[0] = 0;
    fPkt->reserved[1] = 0;
    fPkt->reserved[2] = 0;
    fPkt->checksum = CALC_CHECKSUM(fPkt);

    /* 이전값 갱신 */
    prev_flex[0] = adc_buf[0];
    prev_flex[1] = adc_buf[1];

    /* 플래그 클리어 */
    gsensor_changed = 0;
    flex_changed    = 0;
    switch_changed  = 0;
}

/* G센서 INT1 (PA7) EXTI 콜백 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == G_Sensor_INT_Pin)
    {
        gsensor_changed = 1;
    }
    else if (GPIO_Pin == Up_Switch_Pin || GPIO_Pin == Down_Switch_Pin)
    {
        switch_changed = 1;
    }
}

/* Flex ADC DMA 완료 콜백 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        int32_t d0 = (int32_t)adc_buf[0] - (int32_t)prev_flex[0];
        int32_t d1 = (int32_t)adc_buf[1] - (int32_t)prev_flex[1];

        if (d0 < 0) d0 = -d0;
        if (d1 < 0) d1 = -d1;

        if (d0 > FLEX_THRESHOLD || d1 > FLEX_THRESHOLD)
        {
            flex_changed = 1;
        }
    }
}
