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
│  PA15── I2C1_SCL ── MPU6050 SCL     │        │  PA5  ── TIM2_CH1 ── Servo 2 (PWM)  │
│  PB7 ── I2C1_SDA ── MPU6050 SDA     │        │  PA6  ── TIM3_CH1 ── Servo 3 (PWM)  │
│  PB4 ── GPIO_IN  ── Button (PULLUP) │        │  PA4  ── TIM3_CH2 ── Servo 4 (PWM)  │
│  PB5 ── GPIO_OUT ── Relay (SPDT)    │        │  PB0  ── TIM3_CH3 ── Servo 5 (PWM)  │
│  PA11── FDCAN1_RX                   │        │  PA15 ── I2C1_SCL ── LCD SCL        │
│  PA12── FDCAN1_TX                   │        │  PB7  ── I2C1_SDA ── LCD SDA        │
│  PA10── USART1_RX ── HC-06 TX       │        │  PA11 ── FDCAN1_RX                  │
│  PB6 ── USART1_TX ── HC-06 RX       │        │  PA12 ── FDCAN1_TX                  │
└─────────────────────────────────────┘        │  PA10 ── USART1_RX ── HC-06 TX      │
                                               │  PB6  ── USART1_TX ── HC-06 RX      │
                                               └─────────────────────────────────────┘
```

## 통신 구조

```
  REMOTE CONTROLLER                                      ROBOT ARM
  ─────────────────                                      ─────────

  [CAN 0x100] ──── 밴딩센서 + 자이로 (10ms) ──────────►
  [CAN 0x101] ──── 리모콘 상태 (100ms) ────────────────►

                   ◄──── 로봇팔 상태 (100ms) ──────── [CAN 0x202]

  [BT UART] ◄─────── Bluetooth HC-06 ──────────────────► [BT UART]
             BT_Pack_And_Send(BT_ID_REMOTE_SENSOR, ...)

  CAN 버스: FDCAN1, Classic CAN, 500 kbps, Intel Byte Order (LSB First)
  BT:       USART1, 115200 bps, HC-06
  BT 패킷:  [SOF(0xAA)][ID][LEN][PAYLOAD×8][CHECKSUM][EOF(0x55)] = 13 bytes 고정
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
 │    ReadSensor_Update_100ms()      ─── 릴레이 상태 읽기             │
 │    BT_ConnectionMonitor_100ms()   ─── 3초 무수신 → DISCONNECTED   │
 └──────────────────────────────────────────────────────────────────┘

 ┌──────────────────────────────────────────────────────────────────┐
 │  Comm_Task  (AboveNormal / 256 words)                            │
 │                                                                  │
 │  osSemaphoreAcquire(CAN_Sem)  ◄── Timer10ms_Callback (10ms)      │
 │                                                                  │
 │  [매 10ms]                                                       │
 │    ReadSensor_Update_10ms()   ─── 밴딩센서/자이로/모터 CCR 읽기      │
 │    currentCommMode == CAN  → Pack+Tx CAN 0x100                  │
 │    currentCommMode == BT   → BT_Pack_And_Send(REMOTE_SENSOR)    │
 │                                                                  │
 │  [매 100ms  (10틱마다 1회)]                                        │
 │    currentCommMode == CAN  → Pack+Tx CAN 0x101 or 0x202         │
 └──────────────────────────────────────────────────────────────────┘

 ┌──────────────────────────────────────────────────────────────────┐
 │  Servo_Task  (Normal / 256 words)          [미구현]               │
 │                                                                  │
 │  remoteSensorRx → 각도 매핑 → PWM 출력                             │
 └──────────────────────────────────────────────────────────────────┘

 ┌──────────────────────────────────────────────────────────────────┐
 │  LCD_Task  (BelowNormal / 256 words)       [미구현]               │
 │                                                                  │
 │  osSemaphoreAcquire(LCD_Sem)  ◄── Timer100ms_Callback (100ms)    │
 └──────────────────────────────────────────────────────────────────┘
     │
     ▼
 우선순위 낮음
```

## SPDT 릴레이 배선 (리모콘 보드)

```
  STM32 PB5 (GPIO_Relay_Output)
         │
         ▼
    [SPDT 릴레이]
         │
    ┌────┴────┐
    │   COM   │
    └────┬────┘
         │
    ┌────┴────────────────┐
    │                     │
   NO (HIGH)            NC (LOW)
    │                     │
  BT 모듈 전원          CAN 트랜시버 전원
  (HC-06 VCC)          (SN65HVD230 VCC)

  PB5 HIGH → 릴레이 ON  → NO 연결 → BT 모듈에 전원 → HC-06 동작
  PB5 LOW  → 릴레이 OFF → NC 연결 → CAN 트랜시버에 전원 → FDCAN1 동작
```

## 부팅 시퀀스

```
  main()
    │
    ├── HAL_Init() / SystemClock_Config() / MX_xxx_Init()
    │
    ├── ReadSensor_Init()           ADC DMA 시작, MPU6050 초기화 (I2C)
    │
    ├── CommPowerSelect_Init()      g_comm_toggle=1, apply_bt_mode()
    │       │                         PB5 HIGH → BT 전원 인가
    │       │                         HAL_Delay(100ms) → HC-06 안정화
    │       └────────────────────►    BT_Init() → UART RX IT 시작
    │                                 localSwitchStatus=1, currentCommMode=BT
    │
    ├── osKernelInitialize()
    ├── [세마포어 / 타이머 / 큐 / 태스크 생성]
    ├── osTimerStart(Timer10ms, 10)
    ├── osTimerStart(Timer100ms, 100)
    │
    └── osKernelStart()
            │
            ├── Mode_Task:   ReadSensor_Update_100ms + BT_ConnectionMonitor (100ms)
            ├── Comm_Task:   CAN_Sem 대기 → 10ms 송신 루프
            ├── Servo_Task:  [미구현]
            └── LCD_Task:    LCD_Sem 대기 → [미구현]
```

## 통신 모드 전환 로직 (Comm_Power_Select.c)

```
  버튼(PB4) PULLUP, LOW=누름
       │
       ▼ EXTI
  CommPowerSelect_ButtonPressed()
       │
       ▼ TimerOnce 50ms 원샷 시작 (디바운스)
  CommPowerSelect_DebounceExpired()
       │
       ├── 핀 재확인 LOW? ──YES──► g_comm_toggle ^= 1
       │                                │
       │                    ┌───────────┴───────────┐
       │                    ▼                       ▼
       │             g_comm_toggle==1        g_comm_toggle==0
       │             apply_bt_mode()         apply_can_mode()
       │                    │                       │
       │             PB5 HIGH                PB5 LOW
       │             HAL_Delay(100ms)        HAL_Delay(50ms)
       │             BT_Init()               CAN_Start()
       │             localSwitchStatus=1     localSwitchStatus=0
       │             currentCommMode=BT      currentCommMode=CAN
       │
       └── 핀 LOW 아님 → 무시

  ※ 기본 시작: BT 모드 (CommPowerSelect_Init → apply_bt_mode)
  ※ localSwitchStatus: 0=CAN, 1=BT
```

## 데이터 흐름 (리모콘 보드)

```
  ADC DMA ────► adcBuffer[2] ────► Read_BendingSensor() ──┐
                                                           │
  I2C MPU6050 ──────────────────► Read_GyroSensor()  ─────┤
                                                           ▼
                                                    remoteSensorTx
                                                           │
                              ┌────────────────────────────┤
                              │                            │
                         CAN 모드                       BT 모드
                              │                            │
                   Pack_Remote_CAN_Message(0x100)   BT_Pack_And_Send
                   Tx_Remote_CAN_Message(0x100)     (BT_ID_REMOTE_SENSOR)
                              │                            │
                           [CAN Bus]                 [UART HC-06]
```

## 데이터 흐름 (로봇팔 보드)

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
      │                                            Servo_Task [미구현]
      │
      └── 0x101 ──► Update_SystemStatus_FromRemote() ──► sysStatus

  [UART HC-06]
      │
      ▼
  HAL_UART_RxCpltCallback() → parse_byte() → g_ready_pkt
      │
      ▼ (Comm_Task에서 폴링)
  BT_Receive_Packet() → osMessageQueuePut(BT_Queue)
      │
      ▼
  Servo_Task [미구현]
```

## CAN 메시지 ID 정의

| ID | 방향 | 주기 | 내용 |
|----|------|------|------|
| 0x100 | Remote → Robot | 10ms | 밴딩센서[2] + 자이로 Pitch/Roll |
| 0x101 | Remote → Robot | 100ms | CommMode + SensorStatus + Checksum |
| 0x202 | Robot → Remote | 100ms | CommMode + MotorStatus[6] + LcdStatus |

### CAN 프레임 바이트 레이아웃 (Intel Byte Order, LSB First)

**0x100 — 리모콘 센서 (8 bytes)**
```
Byte  0~1 : BendingSensor[0]  (uint16_t, ADC 원시값)
Byte  2~3 : BendingSensor[1]  (uint16_t, ADC 원시값)
Byte  4~5 : GyroPitch         (int16_t,  단위: 0.1°)
Byte  6~7 : GyroRoll          (int16_t,  단위: 0.1°)
```

**0x101 — 리모콘 상태 (8 bytes)**
```
Byte  0   : RemoteCommStatus  (CommMode enum: 0=IDLE, 1=CAN, 2=BT, 3=ERROR)
Byte  1   : RemoteSensorStatus
             bit[0]   = BendingSensor[0] 이상
             bit[1]   = BendingSensor[1] 이상
             bit[2]   = Gyro 이상
             bit[3]   = localSwitchStatus (0=CAN, 1=BT)
             bit[4]   = localRelayStatus  (PB5 실제 출력 레벨)
Byte  2   : RemoteChecksum    (0x100 프레임 8바이트 합산)
Byte  3~7 : 예약
```

**0x202 — 로봇팔 상태 (8 bytes)**
```
Byte  0   : RobotCommStatus   (CommMode enum)
Byte  1   : MotorStatus
             bit[0~5] = 모터 0~5 PWM 출력 여부 (CCR > 0)
Byte  2   : LcdStatus         (0=이상, 1=정상)
Byte  3~7 : 예약
```

## 주요 변수 소유권

```
  ┌──────────────────────────────────────────────────────────┐
  │  변수                  정의 위치              읽는 곳       │
  ├──────────────────────────────────────────────────────────┤
  │  localSwitchStatus     Comm_Power_Select.c   Comm_Task   │
  │  currentCommMode       Comm_Power_Select.c   Comm_Task   │
  │  localRelayStatus      readSensor.c          can_comm.c  │
  │  localSensorStatus     readSensor.c          can_comm.c  │
  │  localRelayStatus      readSensor.c          can_comm.c  │
  │  adcBuffer[2]          readSensor.c          내부         │
  │  remoteSensorTx        can_comm.c            readSensor  │
  │  remoteSensorRx        can_comm.c            Servo_Task  │
  │  sysStatus             can_comm.c            LCD_Task    │
  └──────────────────────────────────────────────────────────┘

  ※ localSwitchStatus는 Comm_Power_Select.c 전용
     Read_SwitchRelay()는 localRelayStatus(PB5 핀 레벨)만 읽음
     localSwitchStatus는 apply_bt/can_mode()에서만 변경됨
```
