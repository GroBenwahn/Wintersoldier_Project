# RE_Robot CAN 통신 개선 작업 요약

STM32G431 기반 Wintersoldier 프로젝트(Hand=글러브 컨트롤러, Robot=로봇팔)의 FDCAN1 CAN 통신 설정을 검토하고, 발견된 문제들을 해결했으며, CAN 송수신 구조를 이벤트 기반에서 주기 기반으로 재설계해 Hand 쪽 구현까지 완료한 세션 기록입니다.

---

## 1. 초기 CAN(FDCAN1) 설정 검토

### 발견 사항
- Hand(`H_C.ioc`)와 Robot(`R_A.ioc`)의 FDCAN1 비트타이밍이 서로 일치 (500kbps, 통신 가능한 상태였음)
- 다만 샘플포인트가 **52.9%**로 낮았음 (CAN 권장치는 75~87.5%)
  - 계산: Sync(1)+Seg1(8)+Seg2(8)=17TQ, 샘플포인트 = (1+8)/17 = 52.9%
- Nominal Sync Jump Width=1로 매우 작아 오실레이터 드리프트 보정 여력 부족
- Auto Retransmission=Disable, Std/Ext Filters=0/0 확인 (의도된 설계로 보이나 확인 필요 항목으로 안내함)

### 적용된 튜닝 (최종 반영됨)
클럭 소스(PCLK1, 170MHz)와 Prescaler(20)는 그대로 두고 Seg1/Seg2/SJW만 조정:

| 항목 | 이전 | 변경 |
|---|---|---|
| Nominal Time Seg1 | 8 | **12** |
| Nominal Time Seg2 | 8 | **4** |
| Nominal Sync Jump Width | 1 | **3** |

- 결과: Baud Rate 500kbps(499999bit/s)는 그대로, 샘플포인트 **76.5%**로 개선
- Hand/Robot 양쪽 `.ioc`와 생성된 `main.c`에 반영 완료 (사용자가 CubeMX에서 직접 작업 후 push)
- **주의**: 예전 설계 문서(`OLD/REDESIGN.md`)에 적힌 수치(Prescaler=4, Seg1=11, Seg2=4, SJW=3)는 FDCAN 클럭소스가 PLLQ 32MHz라는 다른 전제 하에 계산된 값이라, 지금 클럭(PCLK1 170MHz)에 그대로 넣으면 500kbps가 깨짐(수백만 bps로 어긋남). 그대로 가져다 쓰면 안 됨.

---

## 2. 발견된 저장소/코드 문제

### 2-1. `RE_Robot` / `RE_robot` 폴더 분리 (대소문자 문제)
- FreeRTOS 관련 파일(`Middlewares/`, `FreeRTOSConfig.h`, `stm32g4xx_hal_timebase_tim.c` 등)이 실제 프로젝트가 있는 `RE_Robot`(대문자)이 아니라 `RE_robot`(소문자)이라는 별도 git 경로에 들어가 있던 문제를 발견.
- 원인: Windows(대소문자 구분 안 함) 환경에서 CubeMX가 FreeRTOS 지원을 나중에 추가하며 코드를 재생성할 때 케이싱이 어긋났고, git에는 두 개의 완전히 별도인 디렉토리 트리로 커밋되어 있었음.
- 대소문자를 구분하는 환경(Linux 등)에서 클론하면 `Middlewares`를 못 찾아 빌드 자체가 실패하는 상태였음.
- git 커밋 히스토리 분석으로 정확한 발생 시점(FreeRTOS 최초 활성화 커밋)까지 특정함.
- 78개 파일을 `git mv`로 병합하는 작업을 진행했었으나, 이후 세션 중간에 다른 이유로 로컬 브랜치를 여러 번 리셋하면서 이 병합 커밋 자체는 원격에 반영되지 않았을 가능성이 있음. **→ 최신 상태에서 재확인 필요.**

### 2-2. `H_C.ioc` 병합 충돌 미해결
- `H_C.ioc`의 `FREERTOS.*` 설정 섹션에 git 병합 충돌 마커(`<<<<<<< HEAD` ~ `=======` ~ `>>>>>>> b6920da...`)가 해결되지 않은 채 그대로 커밋되어 있던 것을 발견.
- 파일이 문법적으로 깨져있어 CubeMX가 정상적으로 열지 못할 수 있는 상태였음.
- 사용자가 로컬에서 충돌을 해결(Task 우선순위는 기존 유지 + `Timers01` 추가)하고 CubeMX Generate Code를 재실행한 뒤 push하여 해결됨.
- 이 과정에서 `TimerOnce`(osTimerOnce), `Timer10ms`, `Timer100ms` 소프트웨어 타이머(FreeRTOS `osTimerNew`)가 정상적으로 코드에 생성됨.

### 2-3. `can_comm.c` 매크로 이름 불일치 (빌드 깨짐)
- `Packet.h`의 CAN ID 매크로가 `CAN_MSG_ID_SENSOR`/`CAN_MSG_ID_FLEX` → `CAN_MSG_SENSOR`(0x100)/`CAN_MSG_FLEX`(0x101)로 이름이 바뀌었는데, `can_comm.c`는 옛 이름을 그대로 참조하고 있어 컴파일이 안 되는 상태였음.
- 이번 세션의 Hand 쪽 구현 작업에서 함께 수정함.

---

## 3. CAN 송수신 구조 재설계

### 문제 인식
- 기존 구조: `SensorTask`/`SwitchTask`가 10ms마다 변화 플래그만 확인 → 변화가 있을 때만 `CommTask`에 신호 → `CommTask`가 BT와 CAN을 동시에 전송.
- 변화가 없으면 CAN 프레임이 전혀 안 나가는 구조라, Robot 쪽에서 "연결 끊김"을 수신 타임아웃으로 판단하기 어려움 (오탐/누락 위험).
- Robot 쪽엔 CAN 수신 코드가 아예 없었음 — 필터 설정, RX 알림 활성화, `HAL_FDCAN_GetRxMessage`, ID별 dispatch 전부 없고 `CommTask`가 빈 루프.

### 확정된 설계
| 항목 | 결정 |
|---|---|
| CAN 송신 | **완전 주기 전송(10ms)**, 값 변화 여부와 무관 |
| 수신 타임아웃 기준 | **100ms** |
| 이번 라운드 구현 범위 | SENSOR(0x100)/FLEX(0x101) 송수신 + 타임아웃만. ERROR_HAND(0x102)/ERROR_ROBOT(0x201)는 다음 단계로 미룸 |
| Robot CAN 필터 | 필터 0개 + 소프트웨어 switch-case dispatch (하드웨어 Range 필터 안 씀) |
| BT(블루투스) 송신 | **기존처럼 변화 감지시에만 송신(이벤트 기반) 유지** — 변경 없음 |
| 센서 읽기 구조 | 중간 원시값 구조체 `SensorData_t` 도입 → BT용/CAN용 패킷은 이 원시값에서 각각 패킹 |
| ADC(Flex 센서) | DMA로 항상 최신값이 버퍼에 있어 별도 폴링/트리거 불필요, 버퍼 직접 참조 |
| G센서(ADXL345, I2C) | DMA가 아니라 블로킹 I2C 트랜잭션 — 여전히 누군가 주기적으로 능동 트리거해야 함 (Timer10ms 콜백이 담당) |
| `CAN_Send` 함수 형태 | ID별 wrapper 함수 여러 개 대신, `CAN_Send(uint32_t id, const uint8_t *data)` 하나 + 내부 switch-case로 ID 검증/분기 |
| 동시성 안전 | `tx_header`를 공유 `static` 변수 대신 호출마다 새로 만드는 지역변수(`CAN_MakeTxHeader` 헬퍼)로 변경 — 나중에 다른 주기의 CAN 메시지가 다른 타이머/태스크에서 호출되어도 안전 |

---

## 4. 구현 완료 (Hand 쪽)

### `RE_Robot/Hand/Core/Inc/can_comm.h`, `Core/Src/can_comm.c`
- `CAN_MSG_ID_SENSOR`/`CAN_MSG_ID_FLEX` → `CAN_MSG_SENSOR`/`CAN_MSG_FLEX`로 수정 (빌드 깨짐 수정)
- `void CAN_Send(uint32_t id, const uint8_t *data)` — switch-case로 알려진 ID인지 확인 후 내부 `CAN_SendFrame()` 호출
- `CAN_MakeTxHeader(uint32_t id)` 헬퍼 추가, `tx_header`를 함수별 지역변수로 전환
- `CAN_Comm_Init()`, `CAN_Recovery()`, `can_offline` 로직(Bus-off 감지/2초 주기 복구 시도)은 기존 그대로 유지

### `RE_Robot/Hand/Core/Inc/sensor.h`, `Core/Src/sensor.c`
- `SensorData_t` 구조체 추가: `roll`, `pitch`, `height_sw`, `flex1`, `flex2`
- `Sensor_ReadAll(SensorData_t *data)` — I2C(G센서 블로킹 읽기) + ADC 버퍼 복사 + GPIO 스위치 읽기만 수행 (패킷 조립/체크섬 없음)
- `Sensor_PackGSensor()`, `Sensor_PackFlex()` 신설 — `SensorData_t` → 각 패킷(체크섬 포함) 조립
- `Sensor_ResetFlexBaseline()` 신설 — Flex 변화감지 기준값(`prev_flex`)을 **BT 이벤트를 실제로 처리한 시점**에만 갱신하도록 분리. (10ms마다 무조건 갱신하면 느리고 지속적인 변화가 매번 임계값 이하로만 움직여 영영 감지되지 않는 문제가 생길 뻔했음)
- `HAL_GPIO_EXTI_Callback`, `HAL_ADC_ConvCpltCallback`(변화 감지용 콜백)은 그대로 유지

### `RE_Robot/Hand/Core/Src/main.c`
- 전역 `static SensorData_t g_sensorData` 추가 — `Callback_10ms`가 갱신하고 BT/CAN 양쪽이 공유
- `osTimerStart(Timer10msHandle, 10)` 추가 (RTOS_TIMERS 섹션)
- `Callback_10ms`: `Sensor_ReadAll()` → `Sensor_PackGSensor()`/`Sensor_PackFlex()` → `CAN_Send(CAN_MSG_SENSOR,...)` + `CAN_Send(CAN_MSG_FLEX,...)` — **CAN 송신 전담**
- `StartCommTask`: 큐 신호 수신 시 공유된 `g_sensorData`를 패킹만 해서 `BT_Send()` 호출 (더 이상 자체적으로 센서를 읽지 않음), 이후 변화 플래그 클리어 + `Sensor_ResetFlexBaseline()` 호출. `CAN_Recovery()`는 매 루프 그대로 유지
- `StartSensorTask`/`StartSwitchTask`: **변경 없음** (10ms 폴링, 변화 감지 시 큐에 신호)
- `Callback_Once`/`Callback_100ms`: 아직 빈 상태로 유지 (용도 미정, 다음에 필요하면 채우기로 함)

---

## 5. 남은 작업 (다음 단계)

1. **Robot 쪽 CAN 수신 구현** (미착수)
   - `can_comm.c/h` 신규 생성
   - `HAL_FDCAN_ConfigGlobalFilter()`로 명시적 accept 설정 + `RX_FIFO0_NEW_MESSAGE`/`BUS_OFF` 알림 활성화
   - `HAL_FDCAN_RxFifo0Callback()`에서 `Identifier` 기준 switch-case dispatch (SENSOR/FLEX)
   - ID별 마지막 수신 시각 기록 + 100ms 타임아웃 감시 함수
   - `CommTask` 내용 채우기
2. **`Timer100ms` / `TimerOnce` 용도 결정** — Hand `.ioc`엔 선언되어 있으나 아직 아무 데도 안 쓰임
3. **`RE_Robot`/`RE_robot` 폴더 대소문자 분리 문제 최신 상태 재확인** — 세션 중간에 병합을 시도했었지만 그 이후 다른 이유로 로컬 브랜치를 여러 차례 리셋했어서, 현재 원격 저장소에 병합이 실제로 반영되어 있는지 다시 확인이 필요함

---

## 6. 작업 방식 관련 결정

- 이 세션(Claude Code 원격 컨테이너)은 GitHub push 권한 문제(GitHub App이 이 저장소에 대한 쓰기 권한을 못 받은 것으로 추정) + 커밋 서명 인프라 문제(서명 키 파일이 비어있음)로 인해 직접 git commit/push가 불가능한 것으로 확인됨.
- 이후로는 Claude가 git commit/push를 하지 않고, 파일만 수정한 뒤 **순수 diff 파일**로 만들어 전달 → 사용자가 직접 `git apply`로 적용하고 커밋/push를 전담하는 방식으로 전환함.
