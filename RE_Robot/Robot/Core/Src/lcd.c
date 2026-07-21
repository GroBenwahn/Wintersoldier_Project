#include "lcd.h"
#include "can_comm.h"
#include <stdio.h>
#include <string.h>

/* PCF8574 I2C 주소 (7bit=0x27, HAL용 8bit=0x4E) */
#define LCD_ADDR    (0x27 << 1)

/* PCF8574 핀 매핑 */
#define LCD_RS      0x01    /* P0: Register Select (0=명령, 1=데이터) */
#define LCD_RW      0x02    /* P1: Read/Write (항상 0=Write) */
#define LCD_EN      0x04    /* P2: Enable */
#define LCD_BL      0x08    /* P3: 백라이트 ON */
/* P4=D4, P5=D5, P6=D6, P7=D7 */


/* PCF8574에 1바이트 I2C 전송 */
static void PCF8574_Write(uint8_t data)
{
    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, &data, 1, 5);
}


/* 상위 4비트 니블 + EN 펄스 전송 */
static void LCD_SendNibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (nibble & 0xF0) | LCD_BL | (rs ? LCD_RS : 0);
    PCF8574_Write(data | LCD_EN);   /* EN=1 (래치 시작) */
    PCF8574_Write(data);            /* EN=0 (래치 완료) */
}


/* 1바이트를 상위/하위 니블 2회로 전송 */
static void LCD_SendByte(uint8_t byte, uint8_t rs)
{
    LCD_SendNibble(byte & 0xF0, rs);          /* 상위 4비트 */
    LCD_SendNibble((byte << 4) & 0xF0, rs);   /* 하위 4비트 */
}


/* 명령 전송 (RS=0) */
static void LCD_Command(uint8_t cmd)
{
    LCD_SendByte(cmd, 0);
    if (cmd == 0x01 || cmd == 0x02)
        HAL_Delay(2);   /* Clear/Home 명령만 추가 대기 필요 */
}


/* 문자 데이터 전송 (RS=1) */
static void LCD_Char(uint8_t ch)
{
    LCD_SendByte(ch, 1);
}


/* 커서 위치 설정 (row: 0~1, col: 0~15) */
static void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_Command(addr);
}


/* 문자열 출력 */
static void LCD_Print(const char *str)
{
    while (*str)
        LCD_Char((uint8_t)*str++);
}


/* 문자열 출력 후 width까지 공백으로 채움 (잔상 제거) */
static void LCD_PrintPadded(const char *str, uint8_t width)
{
    uint8_t len = (uint8_t)strlen(str);
    if (len > width) len = width;
    for (uint8_t i = 0; i < len; i++)
        LCD_Char((uint8_t)str[i]);
    for (uint8_t i = len; i < width; i++)
        LCD_Char(' ');
}


/* LCD 초기화 (LcdTask 시작 시 1회 호출) */
void LCD_Init(void)
{
    HAL_Delay(50);   /* 전원 안정화 대기 */

    /* 4-bit 초기화 */
    LCD_SendNibble(0x30, 0); HAL_Delay(5);
    LCD_SendNibble(0x30, 0); HAL_Delay(1);
    LCD_SendNibble(0x30, 0); HAL_Delay(1);
    LCD_SendNibble(0x20, 0); HAL_Delay(1);   /* 4-bit 모드 전환 */

    LCD_Command(0x28);   /* 4-bit, 2줄, 5x8 폰트 */
    LCD_Command(0x0C);   /* 디스플레이 ON, 커서 OFF, 깜빡임 OFF */
    LCD_Command(0x06);   /* 커서 오른쪽 이동, 화면 이동 없음 */
    LCD_Command(0x01);   /* 화면 지우기 */
    HAL_Delay(2);
}


/*
 * 줄 1: "Normal Mode     "
 * 줄 2: "Select Comm Mode"
 */
void LCD_ShowNormal(void)
{
    LCD_SetCursor(0, 0);
    LCD_PrintPadded("Normal Mode", 16);
    LCD_SetCursor(1, 0);
    LCD_PrintPadded("Select Comm Mode", 16);
}


/*
 * 줄 1: "  Comm Mode:    "
 * 줄 2: "     CAN        "  OR  "      BT        "
 */
void LCD_ShowMode(CommMode_t mode)
{
    LCD_SetCursor(0, 0);
    LCD_PrintPadded("  Comm Mode:", 16);
    LCD_SetCursor(1, 0);
    if (mode == COMM_CAN)
        LCD_PrintPadded("     CAN", 16);
    else
        LCD_PrintPadded("      BT", 16);
}


/*
 * 줄 1: "  Comm Mode:    "
 * 줄 2: "CAN Connected!  "  또는  " BT Connected!  "
 * comm_connected 값으로 실제 연결된 통신 종류 판단
 */
void LCD_ShowConnected(void)
{
    LCD_SetCursor(0, 0);
    LCD_PrintPadded("  Comm Mode:", 16);
    LCD_SetCursor(1, 0);
    if (comm_connected == COMM_CONNECTED_CAN)
        LCD_PrintPadded("CAN Connected!", 16);
    else if (comm_connected == COMM_CONNECTED_BT)
        LCD_PrintPadded(" BT Connected!", 16);
}


/*
 * 줄 1: Grip / Wrist Pitch / Wrist Roll  (servo_angles 5, 4, 3)
 * 줄 2: Elbow / Shoulder / Waist         (servo_angles 2, 1, 0)
 * 예시: "   0 -30  45    "
 *       " -12  50  12    "
 */
void LCD_ShowAngles(const int8_t *angles)
{
    char buf[17];

    snprintf(buf, sizeof(buf), "%4d%4d%4d", angles[5], angles[4], angles[3]);
    LCD_SetCursor(0, 0);
    LCD_PrintPadded(buf, 16);

    snprintf(buf, sizeof(buf), "%4d%4d%4d", angles[2], angles[1], angles[0]);
    LCD_SetCursor(1, 0);
    LCD_PrintPadded(buf, 16);
}
