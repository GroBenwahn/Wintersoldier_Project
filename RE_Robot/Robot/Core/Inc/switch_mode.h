#ifndef __SWITCH_MODE_H
#define __SWITCH_MODE_H

#include "main.h"

typedef enum {
    COMM_NONE = 0,  /* 가운데 (Normal) - 통신 없음 */
    COMM_BT   = 1,  /* 왼쪽  - BT 모드            */
    COMM_CAN  = 2   /* 오른쪽 - CAN 모드           */
} CommMode_t;

extern volatile CommMode_t comm_mode;

void       SwitchMode_Update(void);
CommMode_t SwitchMode_Get(void);

#endif /* __SWITCH_MODE_H */
