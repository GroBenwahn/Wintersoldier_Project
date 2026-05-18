# Wintersoldier Project — System Architecture

## 하드웨어 구성

```
┌─────────────────────────────────────┐        ┌─────────────────────────────────────┐
│         REMOTE CONTROLLER           │        │           ROBOT ARM                 │
│       (STM32G431KB / NUCLEO)        │        │       (STM32G431KB / NUCLEO)        │
│         ProjModeState = 0           │        │         ProjModeState = 1           │
│                                     │        │                                     │
│  PA0 ── ADC1_CH1 ── Bending[0]      │        │  PA8  ── TIM1_CH1 ── Servo 0 (PWM)  │
│  PA1 ── ADC1_CH2 ── Bending[1]      │        │  PA9  ── TIM1_CH2 ── Servo 1 (PWM)  │
│  PA7 ── EXTI7    ── Gyro INT        │        │  PA5  ── TIM2_CH1 ── Servo 2 (PWM)  │
│  PA15── I2C1_SCL ── MPU6050 SCL     │        │  PA6  ── TIM3_CH1 ── Servo 3 (PWM)  │
│  PB7 ── I2C1_SDA ── MPU6050 SDA     │        │  PA4  ── TIM3_CH2 ── Servo 4 (PWM)  │
│  PB4 ── GPIO_IN  ── Switch (PULLUP) │        │  PB0  ── TIM3_CH3 ── Servo 5 (PWM)  │
│  PB5 ── GPIO_OUT ── Relay           │        │  PA15 ── I2C1_SCL ── LCD SCL        │
│  PA11── FDCAN1_RX                   │        │  PB7  ── I2C1_SDA ── LCD SDA        │
│  PA12── FDCAN1_TX                   │        │  PA11 ── FDCAN1_RX                  │
│  PA10── USART1_RX ── HC-06 TX       │        │  PA12 ── FDCAN1_TX                  │
│  PB6 ── USART1_TX ── HC-06 RX       │        │  PA10 ── USART1_RX ── HC-06 TX      │
└─────────────────────────────────────┘        │  PB6  ── USART1_TX ── HC-06 RX      │
                                               └─────────────────────────────────────┘
```

## 통신 구조

```
  REMOTE CONTROLLER                                      ROBOT ARM
  ─────────────────                                      ─────────

  [CAN 0x100] ──── 밴딩센서 + 자이로 (10ms) ──────────►
  [CAN 0x101] ──── 리모콘 상태 (100ms) ────────────────►

                   ◄──── 모터 각도 피드백 (10ms) ──── [CAN 0x200]
                   ◄──── 모터 각도 피드백 (10ms) ──── [CAN 0x201]
                   ◄──── 로봇팔 상태 (100ms) ──────── [CAN 0x202]

  [BT UART] ◄─────── Bluetooth HC-06 ──────────────────► [BT UART]
             (CommMode = BT일 때 사용, 추후 구현)

  CAN 버스: FDCAN1, Classic CAN, 500 kbps
  BT:       USART1, 9600 bps, HC-06
```

## FreeRTOS 태스크 구성

```
우선순위 높음
     │
     ▼
 ┌──────────────────────────────────────────────────────────────────┐
 │  Mode_Task  (AboveNormal / 128 words)                            │
 │                                                                  │
 │  loop (100ms osDelay):                                           │
 │    ReadSensor_Update_100ms()  ─── 스위치/릴레이/LCD 상태 읽기       │
 │    CommSelect_100ms()         ─── localSwitchStatus → CommMode   │
 └──────────────────────────────────────────────────────────────────┘

 ┌──────────────────────────────────────────────────────────────────┐
 │  Comm_Task  (AboveNormal / 256 words)                            │
 │                                                                  │
 │  osSemaphoreAcquire(CAN_Sem)  ◄── Timer10ms_Callback (10ms)      │
 │                                                                  │
 │  [매 10ms]                                                       │
 │    ReadSensor_Update_10ms()   ─── 밴딩센서/자이로/모터 CCR 읽기      │
 │    Pack + Tx  ────────────────── CAN or BT 송신                   │
 │                                                                  │
 │  [매 100ms  (10틱마다 1회)]                                        │
 │    Pack + Tx  ────────────────── 상태 메시지 송신                   │
 └──────────────────────────────────────────────────────────────────┘

 ┌──────────────────────────────────────────────────────────────────┐
 │  Servo_Task  (Normal / 256 words)          [추후 구현]            │
 │                                                                  │
 │  remoteSensorRx → 각도 매핑 → robotMotorTx → PWM 출력              │
 └──────────────────────────────────────────────────────────────────┘

 ┌──────────────────────────────────────────────────────────────────┐
 │  LCD_Task  (BelowNormal / 256 words)       [추후 구현]            │
 │                                                                  │
 │  osSemaphoreAcquire(LCD_Sem)  ◄── Timer100ms_Callback (100ms)    │
 │    LCD 표시 갱신                                                  │
 └──────────────────────────────────────────────────────────────────┘
     │
     ▼
 우선순위 낮음
```

## 데이터 흐름 (리모콘)

```
  GPIO PB4 ──► Read_SwitchRelay() ──► localSwitchStatus ──► CommSelect_100ms()
  GPIO PB5 ──► Read_SwitchRelay() ──► localRelayStatus  ──► Pack(0x101)

  ADC DMA ───► adcBuffer[2] ─────────► Read_BendingSensor()
                                             │
  I2C MPU6050 ──────────────────────────► Read_GyroSensor()
                                             │
                                             ▼
                                       remoteSensorTx
                                             │
                                             ▼
                                    Pack_Remote_CAN_Message(0x100)
                                             │
                                             ▼
                                    Tx_Remote_CAN_Message(0x100)
                                             │
                                           [CAN Bus]
```

## 데이터 흐름 (로봇팔)

```
  [CAN Bus]
      │
      ▼
  HAL_FDCAN_RxFifo0Callback()
      │
      ├── 0x100 ──► Fill_Remote_CAN_Message() ──► Update_RemoteSensorRx()
      │                                                    │
      │                                                    ▼
      │                                            remoteSensorRx
      │                                                    │
      │                                                    ▼
      │                                            Servo_Task  [추후 구현]
      │                                                    │
      │                                                    ▼
      │                                            robotMotorTx
      │                                                    │
      │                                                    ▼
      │                                    Pack_Robot_CAN_Message(0x200/0x201)
      │                                                    │
      │                                                    ▼
      │                                    Tx_Robot_CAN_Message() ──► [CAN Bus]
      │
      └── 0x101 ──► Fill_Remote_CAN_Message() ──► Update_SystemStatus_FromRemote()
                                                          │
                                                          ▼
                                                      sysStatus
```

## 통신 모드 선택 로직

```
  ┌─────────────────── CommSelect_100ms() ────────────────────────┐
  │                                                               │
  │  ProjModeState = 0 (리모콘)    ProjModeState = 1 (로봇팔)       │
  │       Remote_CommCheck()            Robot_CommCheck()         │
  │                                                               │
  │  localSwitchStatus == 1 (BT)    CAN_IsConnected() ?           │
  │  ├─ BT_IsPaired()                  ├─ YES → COMM_MODE_CAN     │
  │  │   ├─ CAN_IsConnected()          ├─ BT_IsPaired()           │
  │  │   │   ├─ YES → ERROR            │   ├─ YES → COMM_MODE_BT  │
  │  │   │   └─ NO  → COMM_MODE_BT     │   └─ NO  → COMM_MODE_IDLE│
  │  │   └─ NOT paired                 └────────────────────────  │
  │  │       ├─ CAN_IsConnected()                                 │
  │  │       │   ├─ YES → ERROR                                   │
  │  │       │   └─ NO  → COMM_MODE_IDLE                          │
  │  │                                                            │
  │  localSwitchStatus == 0 (CAN)                                 │
  │  ├─ CAN_IsConnected()                                         │
  │  │   ├─ YES → COMM_MODE_CAN                                   │
  │  │   └─ NO                                                    │
  │  │       ├─ BT_IsPaired() → YES → ERROR                       │
  │  │       └─ BT_IsPaired() → NO  → COMM_MODE_IDLE              │
  └───────────────────────────────────────────────────────────────┘
```
