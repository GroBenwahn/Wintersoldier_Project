## 생각해야하는 로직

# 대전제 : 코드는 하나로 통일하며 State에 따라 로봇팔 모드와 리모콘 모드로 따로 디버그 한다.
# 로봇팔 모드 = 1, 리모콘 모드 = 0
#  #if (ProjModeState)
#  #endif

FreeRTOS Task 목록
{
	1. defalut Task : 일반적인 테스크 (**ms)
	2. Comm_Task : 통신용 테스크 (10ms)
	3. Servo_Task : 모터용 테스크 (10 or 100ms)
	4. LDC_Task : LDC 출력 테스크 (100ms)
	5. Mode_Task : 스위치 확인 및 모드 결정 테스크 (100ms)
}



1. 통신 유무선 선택 로직

2. 수신 송신 데이터 처리 로직

	- 데이터 송신

	1. 자이로스코프 SCL, SDA 레벨 확인, 밴딩 센서 저항값 10ms 마다 확인
	2. CAN 통신, Bluetooth 통신인지 확인
	3-1. CAN Frame 에 맞게 데이터 전송
	3-2. Bluetooth 에 맞게 데이터 전송
    //ms 마다 전송 해야할까..? 100? 10?


	- 데이터 수신

	1. CAN 통신, Bluetooth 통신인지 확인
	2. 수신 받고 있는 데이터가 있는지 확인 
    // 만약 확인된 통신과 다른 곳에서 데이터를 받을시 Error
	
	3-1. CAN 정보 수집
	3-2. Bluetooth 정보 수집
	
	4. 받은 데이터 수치 변환
	5. 모터에 변환된 수치값 출력 (데이터 수치 변환과는 다른 함수로 실행)

3. 데이터 수치 변환 로직

4. 모터 출력 로직

5. 시스템 Voltage 및 센서 상태 측정 처리

	1. CAN, Bluetooth, Sys Volt 정상 체크
	
	2. if (로봇팔 모드)
			LDC, motor 정상 체크
		else :
			bending Sensor, Gyro Sensor, Switch 정상 체크

6. 에러 처리
	1. 시스템 상태 측정값 확인
	2. Error & Warnning 값 확인
	3. if (로봇팔)
			LDC에 Error & Warnning 값 출력
		else :
			LED로 출력
			1번 점등, 2번 점등, 3번 점등 == 각 점등마다 에러 확인
