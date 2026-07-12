#include "sensor.h"
#include "cmsis_os.h"
#include <math.h>

/* G센서 I2C 주소 (SDO=GND 기준, 7bit=0x53, HAL용 8bit=0xA6) */
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

/* G센서 착용 자세 오프셋 (전원 인가 시 자세 = 0 기준) */
static int16_t roll_offset  = 0;
static int16_t pitch_offset = 0;



/* G센서 설정값 레지스터 1바이트 쓰기 */
static void GSensor_WriteReg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    HAL_I2C_Master_Transmit(&hi2c1, ADXL345_ADDR, buf, 2, 10);
}


/* 전원 인가 시 G센서 현재 자세를 오프셋으로 저장 처음 손위치 값 보정 */
static void Sensor_CalibrateOffset(void)
{
    uint8_t raw[6] = {0};
    uint8_t reg = ADXL345_DATAX0;
    HAL_I2C_Master_Transmit(&hi2c1, ADXL345_ADDR, &reg, 1, 10);
    HAL_I2C_Master_Receive (&hi2c1, ADXL345_ADDR, raw,  6, 10);

    int16_t ax = (int16_t)((raw[1] << 8) | raw[0]);
    int16_t ay = (int16_t)((raw[3] << 8) | raw[2]);
    int16_t az = (int16_t)((raw[5] << 8) | raw[4]);

    float fax = ax * 0.004f;
    float fay = ay * 0.004f;
    float faz = az * 0.004f;

    roll_offset  = (int16_t)(atan2f(fay, faz)                       * (1800.0f / 3.14159f));
    pitch_offset = (int16_t)(atan2f(-fax, sqrtf(fay*fay + faz*faz)) * (1800.0f / 3.14159f));
}


/* G센서 초기화 */
static void GSensor_Init(void)
{
    GSensor_WriteReg(ADXL345_DATA_FORMAT, 0x08); /* 풀리졸루션, ±2g */
    GSensor_WriteReg(ADXL345_BW_RATE,     0x0A); /* 출력속도 100Hz */
    GSensor_WriteReg(ADXL345_INT_MAP,     0x00); /* 모든 인터럽트 → INT1 */
    GSensor_WriteReg(ADXL345_INT_ENABLE,  0x80); /* DATA_READY 인터럽트 활성화 */
    GSensor_WriteReg(ADXL345_POWER_CTL,   0x08); /* 측정 시작 */
}


/* 센서 초기화 */
void Sensor_Init(void)
{
    GSensor_Init();
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, 2);
    osDelay(500); /* G센서 측정 안정화 대기 */
    prev_flex[0] = adc_buf[0]; /* Flex 초기 기준값 동기화 */
    prev_flex[1] = adc_buf[1];
    Sensor_CalibrateOffset();
}


/* 전체 센서 원시값 읽기 (I2C 블로킹 읽기 + ADC 버퍼 복사 + GPIO 읽기) */
void Sensor_ReadAll(SensorData_t *data)
{
    /* --- G센서 읽기 --- */
    uint8_t raw[6] = {0};
    uint8_t reg = ADXL345_DATAX0;
    HAL_I2C_Master_Transmit(&hi2c1, ADXL345_ADDR, &reg, 1, 10);
    HAL_I2C_Master_Receive (&hi2c1, ADXL345_ADDR, raw,  6, 10);

    int16_t ax = (int16_t)((raw[1] << 8) | raw[0]);
    int16_t ay = (int16_t)((raw[3] << 8) | raw[2]);
    int16_t az = (int16_t)((raw[5] << 8) | raw[4]);

    float fax = ax * 0.004f;
    float fay = ay * 0.004f;
    float faz = az * 0.004f;

    data->roll  = (int16_t)(atan2f(fay, faz)                       * (1800.0f / 3.14159f)) - roll_offset;
    data->pitch = (int16_t)(atan2f(-fax, sqrtf(fay*fay + faz*faz)) * (1800.0f / 3.14159f)) - pitch_offset;

    /* --- 스위치 읽기 --- */
    uint8_t up   = (HAL_GPIO_ReadPin(Up_Switch_GPIO_Port,   Up_Switch_Pin)   == GPIO_PIN_RESET) ? 1 : 0;
    uint8_t down = (HAL_GPIO_ReadPin(Down_Switch_GPIO_Port, Down_Switch_Pin) == GPIO_PIN_RESET) ? 1 : 0;

    if      (up)   data->height_sw = HEIGHT_UP;
    else if (down) data->height_sw = HEIGHT_DOWN;
    else           data->height_sw = HEIGHT_STOP;

    /* --- Flex 읽기 (ADC는 DMA로 항상 최신값이 버퍼에 있어 그대로 복사) --- */
    data->flex1 = adc_buf[0];
    data->flex2 = adc_buf[1];
}


/* GSensorPacket 조립 (체크섬 포함) */
void Sensor_PackGSensor(const SensorData_t *data, GSensorPacket_t *pkt)
{
    pkt->roll        = data->roll;
    pkt->pitch       = data->pitch;
    pkt->height_sw   = (uint8_t)data->height_sw;
    pkt->reserved[0] = 0;
    pkt->reserved[1] = 0;
    pkt->checksum    = CALC_CHECKSUM(pkt);
}


/* FlexPacket 조립 (체크섬 포함) */
void Sensor_PackFlex(const SensorData_t *data, FlexPacket_t *pkt)
{
    pkt->flex1       = data->flex1;
    pkt->flex2       = data->flex2;
    pkt->reserved[0] = 0;
    pkt->reserved[1] = 0;
    pkt->reserved[2] = 0;
    pkt->checksum    = CALC_CHECKSUM(pkt);
}


/* Flex 변경 감지 기준값 갱신 (BT로 변경 이벤트를 처리한 직후 호출) */
void Sensor_ResetFlexBaseline(void)
{
    prev_flex[0] = adc_buf[0];
    prev_flex[1] = adc_buf[1];
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
