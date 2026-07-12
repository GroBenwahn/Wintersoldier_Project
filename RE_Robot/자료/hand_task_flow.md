# Hand 보드 부팅/실행 흐름 (수정본)

사용자가 작성한 초기 흐름도를 `RE_Robot/Hand/Core/Src/main.c`, `can_comm.c`, `sensor.c` 실제 코드와 대조해서 수정함 (2026-07-12 확인). CAN 재설계 작업 배경은 [wintersoldier_can_session_summary.md](wintersoldier_can_session_summary.md) 참고.

---

## 수정된 흐름도

```
전원 ON
  │
  ▼
HAL_Init()                 ← 하드웨어 기초 초기화 (TIM6 타임베이스 시작)
SystemClock_Config()       ← CPU 170MHz 설정 (HSI 16MHz → PLL)
  │
MX_GPIO_Init()        ← 스위치, G센서 INT 핀 설정
MX_DMA_Init()         ← Flex ADC DMA 채널 설정
MX_ADC1_Init()        ← ADC1 설정 (12bit, 연속변환, DMA)
MX_FDCAN1_Init()      ← FDCAN1 설정 (500kbps Classic CAN, 필터 0개)
MX_I2C1_Init()        ← I2C1 설정 (ADXL345용)
MX_USART1_UART_Init() ← UART1 설정 (115200, HC-05용)
  │
osKernelInitialize()  ← FreeRTOS 커널 준비
  │
TimerOnce   = osTimerNew(Callback_Once,  osTimerOnce)      ← 생성만, 아직 미시작
Timer10ms   = osTimerNew(Callback_10ms,  osTimerPeriodic)  ← 생성 직후 osTimerStart(10)  [NEW]
Timer100ms  = osTimerNew(Callback_100ms, osTimerPeriodic)  ← 생성만, 아직 미시작
  │
packetQueue = osMessageQueueNew(8, 8)  ← 큐 생성 (8슬롯 × 8바이트)
osThreadNew × 3  ← CommTask(Normal) / SensorTask(Normal) / SwitchTask(AboveNormal) 등록  [우선순위 정정]
  │
osKernelStart()      ← 스케줄러 시작, 이후 FreeRTOS가 제어
  │
  ├─ CommTask (Normal)
  │    CAN_Comm_Init()  ← CAN 활성화 (Bus-off 알림 등록 + HAL_FDCAN_Start)
  │    osMessageQueueGet(100ms 타임아웃) ← 큐 비어있음 → 블로킹
  │
  ├─ SensorTask (Normal)
  │    Sensor_Init()
  │      ├─ ADXL345 측정 시작
  │      ├─ Flex DMA 시작 (HAL_ADC_Start_DMA)
  │      ├─ osDelay(500) → 블로킹 (0.5초 안정화 대기)
  │      ├─ prev_flex[] 동기화 (Flex 변화감지 기준값)
  │      └─ Sensor_CalibrateOffset() ← 지금 자세를 0으로 저장 (roll/pitch offset)
  │    (0.5초 후) for(;;) 루프 진입 → gsensor_changed/flex_changed 확인 시작
  │
  ├─ SwitchTask (AboveNormal — 3개 태스크 중 최우선)                          [NEW: 이벤트 흐름에 추가]
  │    for(;;) switch_changed 확인 → 없음 → osDelay(10) → 블로킹
  │
  └─ (FreeRTOS Timer 데몬 태스크)                                             [NEW: 4번째 실행 흐름]
       Timer10ms 10ms마다 만료 — 위 3개 태스크와 완전히 독립적으로 실행
         Callback_10ms:
           Sensor_ReadAll(&g_sensorData)  ← I2C(G센서 블로킹) + ADC버퍼 + GPIO 읽기
           Sensor_PackGSensor() / Sensor_PackFlex()
           CAN_Send(CAN_MSG_SENSOR, ...) + CAN_Send(CAN_MSG_FLEX, ...)
           ★ 값 변화·이벤트와 무관하게 10ms마다 무조건 송신

─── 센서 변화 발생 (G센서 / Flex / 높낮이 스위치) ────────────────────────────
  │
  인터럽트 (EXTI 또는 ADC DMA 콜백)
    G센서 INT1(PA7)           → gsensor_changed = 1
    Flex 변화 임계값(20) 초과  → flex_changed = 1        (HAL_ADC_ConvCpltCallback)
    높낮이 스위치(PB4/PB5) EXTI → switch_changed = 1      [NEW: 세 번째 플래그]
  │
  SensorTask (10ms 폴링, gsensor/flex 담당)  ─┐
  SwitchTask (10ms 폴링, switch 담당)   ───────┤  ★ 두 태스크가 같은 packetQueue에 병렬로 신호 투입
  플래그 감지 → osMessageQueuePut(sig) ────────┘   (sig 내용은 안 씀, 깨우기 용도의 더미 8바이트)
  │
  CommTask 깨어남  ← 큐에 메시지 도착 → osMessageQueueGet 블로킹 해제
  ★ "AboveNormal이라 즉시 선점"이 아님 — CommTask는 Normal 우선순위로,
    신호를 보내는 SwitchTask(AboveNormal)보다 오히려 낮음. 단순히 큐 데이터 도착으로 깨어나는 것.
  │
    Sensor_PackGSensor(&g_sensorData, &gPkt)   ★ Sensor_ReadAll()이 아님!
    Sensor_PackFlex(&g_sensorData, &fPkt)         Callback_10ms가 이미 갱신해둔 공유값을 패킹만 함
    BT_Send(&gPkt, &fPkt)                       ← UART 송신 (신호 올 때마다, 이벤트 기반 유지)
    gsensor_changed = 0
    flex_changed    = 0
    switch_changed  = 0
    Sensor_ResetFlexBaseline()                  [NEW] Flex 변화감지 기준값을 "지금" 갱신
                                                 (10ms마다 갱신하면 느린 지속 변화가 영영
                                                  감지 안 되는 문제가 있어서 이 시점으로 분리함)
  │
  CAN_Recovery()  ← 루프마다 항상 호출되지만, 내부에서 tick 비교해
                     can_offline=1 이고 2초 이상 지났을 때만 실제 재연결 시도
                     ★ CAN_Send()는 CommTask 안에 없음 — Callback_10ms로 완전히 이동함
  │
  다시 osMessageQueueGet(100ms) 대기 → 반복 (신호 없어도 100ms마다 깨서 CAN_Recovery 재확인)
```

---

## 변경 사항 요약 (이전 다이어그램 → 실제 코드)

| 항목 | 이전 다이어그램 (사용자 작성) | 실제 코드 (2026-07-12 확인) |
|---|---|---|
| **CAN 송신 트리거** | 센서 변화 감지 이벤트 → `CommTask`가 `CAN_Send()` 호출 | `Timer10ms`(`Callback_10ms`)가 **10ms마다 무조건** 송신. 값 변화·이벤트와 무관 |
| **CAN 송신이 도는 위치** | `CommTask` 내부 | `CommTask` 아님 — **FreeRTOS Timer 데몬 태스크**(`Callback_10ms`) |
| **`Sensor_ReadAll()` 호출 주체** | `CommTask`가 이벤트 발생 시 직접 호출 | `Callback_10ms`만 호출 (10ms마다). `CommTask`는 더 이상 센서를 직접 안 읽음 |
| **센서 원시값 공유** | 언급 없음 (`CommTask`가 그때그때 직접 읽는 구조로 서술됨) | 전역 `g_sensorData`를 `Callback_10ms`가 갱신하고, `CommTask`는 패킹만 함 |
| **태스크 우선순위** | `CommTask`=AboveNormal(최고) / `SwitchTask`=Normal / `SensorTask`=BelowNormal(최저) | `CommTask`=Normal / `SensorTask`=Normal / **`SwitchTask`=AboveNormal(최고)** — 순서 자체가 다름 |
| **`CommTask`가 깨어나는 이유** | "AboveNormal → 즉시 선점" | 큐에 메시지 도착 → 블로킹 해제. 우선순위 기반 선점이 아니며, `CommTask`는 오히려 `SwitchTask`보다 낮음 |
| **`SwitchTask`의 역할** | 태스크 생성 목록엔 있으나, 이벤트 흐름도에서 완전히 누락 | `SensorTask`와 동일한 패턴(10ms 폴링)으로 `switch_changed` 감지 → 같은 큐에 신호 투입 |
| **`prev_flex` 갱신 시점** | 언급 없음 | `CommTask`가 BT 이벤트를 실제로 처리한 직후에만 `Sensor_ResetFlexBaseline()`으로 갱신 (10ms 폴링 시점이 아님) |
| **부팅 시 타이머 생성** | 다이어그램에 없음 (태스크 3개만 생성) | `osKernelInitialize()` 직후 `TimerOnce`/`Timer10ms`/`Timer100ms` 3개 생성 + `Timer10ms`만 즉시 `osTimerStart(10)` |
| **`CAN_Recovery()` 위치/주기** | `CommTask` 안, "2초마다" | 여전히 `CommTask` 루프 안(매 반복 호출)이지만, 내부적으로 tick 비교해 2초 지나야 실제 재시도 — **결과적으로는 사용자 설명과 동일** |
| **`Callback_100ms` / `Callback_Once`** | 언급 없음 | 타이머는 생성됐지만 콜백 본문이 아직 빈 스텁 (용도 미정) |

### 핵심 요약
가장 큰 구조 변화는 **CAN 송신이 이벤트 기반 → 완전 주기 기반(10ms)으로 바뀌면서, 그 실행 주체가 `CommTask`에서 별도의 타이머 콜백(`Callback_10ms`)으로 옮겨간 것**입니다. 이 때문에 실행 흐름이 기존 3개 태스크 구조가 아니라 **"3개 태스크 + FreeRTOS 타이머 데몬"의 4갈래 구조**가 됐고, `CommTask`는 이제 BT 이벤트 송신과 CAN 복구 재시도만 담당하는 쪽으로 역할이 좁아졌습니다. 태스크 우선순위도 사용자가 기억하던 것과 실제 코드가 반대로 되어 있어 함께 정정했습니다.
