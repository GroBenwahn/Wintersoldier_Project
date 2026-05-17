/*
 * Comm_select.c
 *
 *  Created on: 2026. 5. 14.
 *      Author: MinJae
 */


/*
 * 현재 BT_IsPaired() 함수와 CAN_IsConnected()함수는 아직 없음
 * 따라서 이후 개발해야 할것
 *
 * 현재 클래스에는 통신 모드 설정이기 때문에 이 클래스에 구현은 하지 않음
 * 만약 만들게 된다면 void 함수를 만들 예정 이후 구조체로 전역 변수로 활용해야 하지 않을까 싶음
 *
 * 에러 구조체 생성후 에러값 플래그를 1로 생성하는 코드 삽입
 * ex) ErrorState.DisMatchPairing = 1
 * */

/****************************************************************
	Include Files
****************************************************************/
#include "config.h"

/****************************************************************
	Function Declaration
****************************************************************/
void CommSelect_100ms(void);
void Remote_CommCheck(void);
void Robot_CommCheck(void);

/****************************************************************
	Data Define
****************************************************************/

uint8_t switchLevel;

/****************************************************************
	Function: CommSelect(void);
	Description: 통신 설정 선택 함수

	현재 main.c 에서 이 함수를 불러올 필요 있음
****************************************************************/
void CommSelect_100ms(void){
#if(ProjModeState)
		//로봇팔모드
		Robot_CommCheck();
#else
		//리모콘모드
		Remote_CommCheck();
#endif
}

/****************************************************************
	Function: Remote_CommCheck(void);
	Description: 리모콘 현재 통신모드 체크
****************************************************************/
void Remote_CommCheck(void){
	uint8_t switchLevel = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4);

	if(switchLevel == 1){
		// 스위치가 블루투스 쪽일때
		if(BT_IsPaired())
		{
			if(CAN_IsConnected())
			{
				currentCommMode = COMM_MODE_ERROR; //Dismatch Error
				// ErrorState.DisMatchPairing = 1
			}
			else
			{
				currentCommMode = COMM_MODE_BT;
			}
		}
		else
		{
			if(CAN_IsConnected())
			{
				currentCommMode = COMM_MODE_ERROR; //Dismatch Error
				// ErrorState.DisMatchPairing = 1
			}
			else
			{
				currentCommMode = COMM_MODE_IDLE;
			}
		}
	}
	else
	{
		if(CAN_IsConnected())
		{
			currentCommMode = COMM_MODE_CAN;
		}
		else
		{
			if(BT_IsPaired())
			{
				currentCommMode = COMM_MODE_ERROR;
			}
			else
			{
				currentCommMode = COMM_MODE_IDLE;
			}
		}
	}


}
/****************************************************************
	Function: Robot_CommCheck(void);
	Description: 로봇팔 현재 통신모드 체크
****************************************************************/
void Robot_CommCheck(void){
	if(CAN_IsConnected()) {
	        currentCommMode = COMM_MODE_CAN;
	    } else if(BT_IsPaired()) {
	        currentCommMode = COMM_MODE_BT;
	    } else {
	        currentCommMode = COMM_MODE_IDLE;
	    }
}
