# Wintersoldier Project — CAN Protocol Sheet

## 개요

| 항목 | 값 |
|---|---|
| 버스 | FDCAN1, Classic CAN |
| 속도 | 500 kbps |
| ID 타입 | Standard ID (11-bit) |
| DLC | 8 bytes (고정) |
| **바이트 순서** | **Intel Byte Order (Little-Endian, LSB First)** |
| 필터 | Range 0x100~0x1FF / 0x200~0x2FF |
| Timeout | 300 ms (`CAN_TIMEOUT_MS`) |

> **바이트 순서 상세:**  
> STM32 Cortex-M4는 기본적으로 Little-Endian(Intel 방식)으로 동작한다.  
> 다중 바이트 필드(uint16, int16)는 **낮은 주소(낮은 Byte 번호)에 LSB**, 높은 주소에 MSB가 배치된다.  
> can_comm.h의 union(`BYTE_FIELD` ↔ `BIT_FIELD`)이 변환 없이 직접 매핑되므로 송수신 양쪽 모두 STM32이면 별도 변환 불필요.  
>
> 예) `BendingSensor_0 = 0x0C80` (3200) 전송 시:  
> | Byte 0 | Byte 1 |  
> |--------|--------|  
> | `0x80` (LSB) | `0x0C` (MSB) |

---

## 메시지 목록

| CAN ID | 방향 | 주기 | 내용 |
|---|---|---|---|
| `0x100` | Remote → Robot Arm | 10 ms | 밴딩센서 + 자이로 |
| `0x101` | Remote → Robot Arm | 100 ms | 리모콘 시스템 상태 |
| `0x200` | Robot Arm → Remote | 10 ms | 모터 각도 0~3 |
| `0x201` | Robot Arm → Remote | 10 ms | 모터 각도 4~5 |
| `0x202` | Robot Arm → Remote | 100 ms | 로봇팔 시스템 상태 |

---

## 0x100 — Remote Sensor Data

**방향:** Remote Controller → Robot Arm  
**주기:** 10 ms  
**구조체:** `_REMOTE_SENSOR`

```
Byte │  7  │  6  │  5  │  4  │  3  │  2  │  1  │  0  │
─────┼──────────────────────────────────────────────────
 0   │                BendingSensor_0 [7:0]             │ uint16 LSB
 1   │                BendingSensor_0 [15:8]            │ uint16 MSB
─────┼──────────────────────────────────────────────────
 2   │                BendingSensor_1 [7:0]             │ uint16 LSB
 3   │                BendingSensor_1 [15:8]            │ uint16 MSB
─────┼──────────────────────────────────────────────────
 4   │                GyroPitch [7:0]                   │ int16 LSB
 5   │                GyroPitch [15:8]                  │ int16 MSB
─────┼──────────────────────────────────────────────────
 6   │                GyroRoll [7:0]                    │ int16 LSB
 7   │                GyroRoll [15:8]                   │ int16 MSB
```

| 필드 | 타입 | 범위 | 설명 |
|---|---|---|---|
| BendingSensor_0 | uint16 | 0 ~ 4095 | ADC1 CH1 (PA0) 원시값, 12-bit |
| BendingSensor_1 | uint16 | 0 ~ 4095 | ADC1 CH2 (PA1) 원시값, 12-bit |
| GyroPitch | int16 | TBD | MPU6050 Pitch 각도 (추후 구현) |
| GyroRoll | int16 | TBD | MPU6050 Roll 각도 (추후 구현) |

> **이상 감지:** BendingSensor 값이 0 또는 4095이면 단선/단락 의심 → `localSensorStatus` bit 세팅

---

## 0x101 — Remote Status

**방향:** Remote Controller → Robot Arm  
**주기:** 100 ms  
**구조체:** `_REMOTE_STATUS`

```
Byte │  7  │  6  │  5  │  4  │  3  │  2  │  1  │  0  │
─────┼──────────────────────────────────────────────────
 0   │              RemoteCommStatus                    │ CommMode enum
─────┼──────────────────────────────────────────────────
 1   │              _reserved0  (0x00)                  │
─────┼──────────────────────────────────────────────────
 2   │  -  │  -  │  -  │relay│ sw  │gyro │ben1 │ben0 │ RemoteSensorStatus
─────┼──────────────────────────────────────────────────
 3   │              RemoteChecksum                      │ 0x100 데이터 합산
─────┼──────────────────────────────────────────────────
4~7  │              미사용 (0x00)                        │
```

| 필드 | 비트 | 설명 |
|---|---|---|
| RemoteCommStatus | Byte 0 | 0=IDLE, 1=CAN, 2=BT, 3=ERROR |
| _reserved0 | Byte 1 | 0 고정 (전압 측정 핀 없음) |
| RemoteSensorStatus[0] | Byte 2 bit0 | Bending 0 이상 (0=정상, 1=이상) |
| RemoteSensorStatus[1] | Byte 2 bit1 | Bending 1 이상 (0=정상, 1=이상) |
| RemoteSensorStatus[2] | Byte 2 bit2 | Gyro 이상 (0=정상, 1=이상) |
| RemoteSensorStatus[3] | Byte 2 bit3 | Switch 상태 (0=CAN쪽, 1=BT쪽) |
| RemoteSensorStatus[4] | Byte 2 bit4 | Relay 출력 상태 (0=OFF, 1=ON) |
| RemoteChecksum | Byte 3 | 0x100 메시지 BYTE_FIELD[0~7] 합산 (uint8 overflow) |

---

## 0x200 — Robot Motor Data 1

**방향:** Robot Arm → Remote Controller  
**주기:** 10 ms  
**구조체:** `_ROBOT_MOTOR_1`

```
Byte │  7  │  6  │  5  │  4  │  3  │  2  │  1  │  0  │
─────┼──────────────────────────────────────────────────
 0   │                MotorAngle_0 [7:0]                │ uint16 LSB
 1   │                MotorAngle_0 [15:8]               │ uint16 MSB
─────┼──────────────────────────────────────────────────
 2   │                MotorAngle_1 [7:0]                │ uint16 LSB
 3   │                MotorAngle_1 [15:8]               │ uint16 MSB
─────┼──────────────────────────────────────────────────
 4   │                MotorAngle_2 [7:0]                │ uint16 LSB
 5   │                MotorAngle_2 [15:8]               │ uint16 MSB
─────┼──────────────────────────────────────────────────
 6   │                MotorAngle_3 [7:0]                │ uint16 LSB
 7   │                MotorAngle_3 [15:8]               │ uint16 MSB
```

| 필드 | 타입 | 범위 | 핀 |
|---|---|---|---|
| MotorAngle_0 | uint16 | 0 ~ 180° | TIM1_CH1 (PA8) |
| MotorAngle_1 | uint16 | 0 ~ 180° | TIM1_CH2 (PA9) |
| MotorAngle_2 | uint16 | 0 ~ 180° | TIM2_CH1 (PA5) |
| MotorAngle_3 | uint16 | 0 ~ 180° | TIM3_CH1 (PA6) |

---

## 0x201 — Robot Motor Data 2

**방향:** Robot Arm → Remote Controller  
**주기:** 10 ms  
**구조체:** `_ROBOT_MOTOR_2`

```
Byte │  7  │  6  │  5  │  4  │  3  │  2  │  1  │  0  │
─────┼──────────────────────────────────────────────────
 0   │                MotorAngle_4 [7:0]                │ uint16 LSB
 1   │                MotorAngle_4 [15:8]               │ uint16 MSB
─────┼──────────────────────────────────────────────────
 2   │                MotorAngle_5 [7:0]                │ uint16 LSB
 3   │                MotorAngle_5 [15:8]               │ uint16 MSB
─────┼──────────────────────────────────────────────────
4~7  │              미사용 (0x00)                        │
```

| 필드 | 타입 | 범위 | 핀 |
|---|---|---|---|
| MotorAngle_4 | uint16 | 0 ~ 180° | TIM3_CH2 (PA4) |
| MotorAngle_5 | uint16 | 0 ~ 180° | TIM3_CH3 (PB0) |

---

## 0x202 — Robot Status

**방향:** Robot Arm → Remote Controller  
**주기:** 100 ms  
**구조체:** `_ROBOT_STATUS`

```
Byte │  7  │  6  │  5  │  4  │  3  │  2  │  1  │  0  │
─────┼──────────────────────────────────────────────────
 0   │  -  │  -  │ m5  │ m4  │ m3  │ m2  │ m1  │ m0  │ MotorStatus
─────┼──────────────────────────────────────────────────
 1   │              RobotCommStatus                     │ CommMode enum
─────┼──────────────────────────────────────────────────
 2   │              _reserved0  (0x00)                  │
─────┼──────────────────────────────────────────────────
 3   │              LcdStatus                           │ 0=이상, 1=정상
─────┼──────────────────────────────────────────────────
4~7  │              미사용 (0x00)                        │
```

| 필드 | 비트 | 설명 |
|---|---|---|
| MotorStatus[0] | Byte 0 bit0 | TIM1_CH1 PWM 출력 여부 (0=미출력, 1=출력 중) |
| MotorStatus[1] | Byte 0 bit1 | TIM1_CH2 PWM 출력 여부 |
| MotorStatus[2] | Byte 0 bit2 | TIM2_CH1 PWM 출력 여부 |
| MotorStatus[3] | Byte 0 bit3 | TIM3_CH1 PWM 출력 여부 |
| MotorStatus[4] | Byte 0 bit4 | TIM3_CH2 PWM 출력 여부 |
| MotorStatus[5] | Byte 0 bit5 | TIM3_CH3 PWM 출력 여부 |
| RobotCommStatus | Byte 1 | 0=IDLE, 1=CAN, 2=BT, 3=ERROR |
| _reserved0 | Byte 2 | 0 고정 (전압 측정 핀 없음) |
| LcdStatus | Byte 3 | I2C ACK 확인 결과 (0=응답없음, 1=정상) |

---

## CommMode 열거형

| 값 | 이름 | 설명 |
|---|---|---|
| 0 | `COMM_MODE_IDLE` | 통신 미확정 (부팅 직후 기본값) |
| 1 | `COMM_MODE_CAN` | CAN 통신 활성 |
| 2 | `COMM_MODE_BT` | Bluetooth 통신 활성 |
| 3 | `COMM_MODE_ERROR` | 스위치-실제통신 불일치 오류 |

---

## PWM 파라미터 (서보 모터)

| 항목 | 값 |
|---|---|
| 타이머 | TIM1, TIM2, TIM3 |
| Prescaler | 169 |
| Period (ARR) | 19999 |
| PWM 주기 | 20 ms (50 Hz) |
| 클럭 | 170 MHz |
| CCR 중립 | 1500 (1.5 ms) |
| CCR 범위 | 500 ~ 2500 (0.5 ms ~ 2.5 ms) |

> **각도 매핑 (추후 구현):** BendingSensor(0~4095) → CCR(500~2500)  
> 0° = CCR 500, 90° = CCR 1500, 180° = CCR 2500

---

## 버스 부하 계산

| 메시지 | 주기 | 페이로드 | 프레임 크기 (비트) | 초당 프레임 수 |
|---|---|---|---|---|
| 0x100 | 10 ms | 8 bytes | ~111 bit | 100 |
| 0x101 | 100 ms | 8 bytes | ~111 bit | 10 |
| 0x200 | 10 ms | 8 bytes | ~111 bit | 100 |
| 0x201 | 10 ms | 8 bytes | ~111 bit | 100 |
| 0x202 | 100 ms | 8 bytes | ~111 bit | 10 |
| **합계** | | | | **320 fps** |

> **버스 부하:** 320 × 111 / 500,000 ≈ **7.1%**  
> (Classic CAN 표준 프레임 기준, 비트 스터핑 미포함)
