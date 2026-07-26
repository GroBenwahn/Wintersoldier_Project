#ifndef __PACKET_H
#define __PACKET_H

#include <stdint.h>

/* UART 프레임 마커 */
#define PKT_START              0xAA
#define PKT_END                0x55

/* CAN 메시지 ID */
#define CAN_MSG_SENSOR      0x100  /* Hand → Robot: G센서 + 높낮이 스위치 */
#define CAN_MSG_FLEX        0x101  /* Hand → Robot: Flex 센서 */
#define CAN_MSG_ERROR_HAND  0x102  /* Hand → Robot: Hand 자체 상태(Flex/G센서) */
#define CAN_MSG_SERVO_DBG   0x200  /* Robot → Hand: 서보 6개 현재 각도 */
#define CAN_MSG_ERROR_ROBOT 0x201  /* Robot → Hand: Robot 자체 상태(LCD/서보 회로) */



/* 높낮이 스위치 상태 (Up_Switch=PB4, Down_Switch=PB5, Active LOW) */
typedef enum 
{
    HEIGHT_STOP = 0,   /* 둘다 HIGH → 정지 */
    HEIGHT_UP   = 1,   /* PB4=LOW  → 위로 */
    HEIGHT_DOWN = 2    /* PB5=LOW  → 아래로 */
} HeightSwitch_t;

/*
 * 패킷 1 - G센서 + 높낮이 스위치 (CAN ID: 0x001)
 * 8바이트 = CAN 클래식 1프레임
 *
 * UART 송신: [PKT_START(1)] [GSensorPacket_t(8)] [PKT_END(1)] = 10바이트
 */
#pragma pack(1)
typedef struct 
{
    int16_t  roll;        /* ADXL345 Roll  각도, 단위 0.1도, -1800 ~ +1800 */
    int16_t  pitch;       /* ADXL345 Pitch 각도, 단위 0.1도, -1800 ~ +1800 */
    uint8_t  height_sw;   /* HeightSwitch_t (HEIGHT_STOP / HEIGHT_UP / HEIGHT_DOWN) */
    uint8_t  reserved[2]; /* 예약 (0으로 채움) */
    uint8_t  checksum;    /* XOR(byte 0 ~ 6) */
} GSensorPacket_t;        /* 총 8바이트 */
#pragma pack()


/*
 * 패킷 2 - Flex 센서
 * 8바이트 = CAN 클래식 1프레임
 *
 * UART 송신: [PKT_START(1)] [FlexPacket_t(8)] [PKT_END(1)] = 10바이트
 */
#pragma pack(1)
typedef struct
{
    uint16_t flex1;       /* Flex 센서 1, ADC 12bit값 (0 ~ 4095) */
    uint16_t flex2;       /* Flex 센서 2, ADC 12bit값 (0 ~ 4095) */
    uint16_t flex3;       /* Flex 센서 3, ADC 12bit값 (0 ~ 4095) */
    uint8_t  reserved[1]; /* 예약 (0으로 채움) */
    uint8_t  checksum;    /* XOR(byte 0 ~ 6) */
} FlexPacket_t;           /* 총 8바이트 */
#pragma pack()


/*
 * 패킷 3 - 서보 디버그 피드백 (CAN ID: 0x200, Robot→Hand)
 * 8바이트 = CAN 클래식 1프레임
 */
#pragma pack(1)
typedef struct 
{
    int8_t   servo1;     /* 서보 1 현재 각도 (-90 ~ +90도, 중립=0) */
    int8_t   servo2;     /* 서보 2 현재 각도 (-90 ~ +90도, 중립=0) */
    int8_t   servo3;     /* 서보 3 현재 각도 (-90 ~ +90도, 중립=0) */
    int8_t   servo4;     /* 서보 4 현재 각도 (-90 ~ +90도, 중립=0) */
    int8_t   servo5;     /* 서보 5 현재 각도 (-90 ~ +90도, 중립=0) */
    int8_t   servo6;     /* 서보 6 현재 각도 (-90 ~ +90도, 중립=0) */
    uint8_t  reserved;   /* 예약 (0으로 채움) */
    uint8_t  checksum;   /* XOR(byte 0 ~ 6) */
} ServoDbgPacket_t;      /* 총 8바이트 */
#pragma pack()



/* 하드웨어 상태 값 (연결/센서 오류 구분) */
typedef enum
{
    HW_STATUS_DISCONNECTED = 0,
    HW_STATUS_CONNECTED    = 1,
    HW_STATUS_SENSOR_ERROR = 3,
} HwStatus_t;

/*
 * 패킷 4 - Hand 자체 상태 (CAN ID: 0x102, Hand→Robot)
 * 8바이트 = CAN 클래식 1프레임
 */
#pragma pack(1)
typedef struct
{
    uint8_t error_Flex;     /* HwStatus_t: Flex 센서(ADC/DMA) 상태 */
    uint8_t error_Gsensor;  /* HwStatus_t: G센서(ADXL345, I2C) 상태 */
    uint8_t reserved[5];    /* 예약 (0으로 채움) */
    uint8_t checksum;       /* XOR(byte 0 ~ 6) */
} HandStatusPacket_t;       /* 총 8바이트 */
#pragma pack()


/*
 * 패킷 5 - Robot 자체 상태 (CAN ID: 0x201, Robot→Hand)
 * 8바이트 = CAN 클래식 1프레임. 서보 이상은 한 번에 1개씩만 보고.
 */
#pragma pack(1)
typedef struct
{
    uint8_t error_LCD;           /* HwStatus_t: LCD(PCF8574, I2C) 상태 */
    uint8_t status_servo;        /* 이상 있는 서보 인덱스 0~5 (servo.h의 SERVO_WAIST~SERVO_GRIP과 동일 값) */
    uint8_t circuit_open_servo;  /* status_servo가 가리키는 서보의 개방(단선) 여부: 0=정상, 1=개방 */
    uint8_t circuit_close_servo; /* status_servo가 가리키는 서보의 단락 여부: 0=정상, 1=단락 */
    uint8_t reserved[3];         /* 예약 (0으로 채움) */
    uint8_t checksum;            /* XOR(byte 0 ~ 6) */
} RobotStatusPacket_t;           /* 총 8바이트 */
#pragma pack()


/* 패킷 크기 */
#define GSENSOR_PKT_SIZE      ((uint8_t)sizeof(GSensorPacket_t))      // 8
#define FLEX_PKT_SIZE         ((uint8_t)sizeof(FlexPacket_t))         // 8
#define SERVO_DBG_PKT_SIZE    ((uint8_t)sizeof(ServoDbgPacket_t))     // 8
#define HAND_STATUS_PKT_SIZE  ((uint8_t)sizeof(HandStatusPacket_t))   // 8
#define ROBOT_STATUS_PKT_SIZE ((uint8_t)sizeof(RobotStatusPacket_t))  // 8


/* UART 전체 프레임 크기 = START(1) + DATA(8) + END(1) = 10 */
#define UART_FRAME_SIZE    (1 + 8 + 1)


/* 체크섬 계산 (byte 0~6 XOR) */
#define CALC_CHECKSUM(p)   ((uint8_t)( \
    ((uint8_t*)(p))[0] ^ ((uint8_t*)(p))[1] ^ \
    ((uint8_t*)(p))[2] ^ ((uint8_t*)(p))[3] ^ \
    ((uint8_t*)(p))[4] ^ ((uint8_t*)(p))[5] ^ \
    ((uint8_t*)(p))[6] ))

#endif /* __PACKET_H */
