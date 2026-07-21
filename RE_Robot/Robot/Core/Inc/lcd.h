#ifndef __LCD_H
#define __LCD_H

#include "main.h"
#include "switch_mode.h"
#include <stdint.h>

extern I2C_HandleTypeDef hi2c1;

void LCD_Init(void);
void LCD_ShowNormal(void);                     /* Normal Mode + Select Comm Mode */
void LCD_ShowMode(CommMode_t mode);            /* Comm Mode: CAN/BT (2초) */
void LCD_ShowConnected(void);                  /* CAN/BT Connected! (comm_connected 값으로 판단) */
void LCD_ShowAngles(const int8_t *angles);    /* 서보 6개 각도 표시 */

#endif /* __LCD_H */
