/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_racp.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  24/10/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include <stdint.h>
#include <string.h>
#include <app.h>
#include "cgms_racp.h"
#include "app_util.h"
#include "cgms_db.h"
#include "cgms_aes128.h"
#include "sl_bt_api.h"
#include "cgms_crc.h"
#include "app_glucose_meas.h"
//#include "app_glucose_meas.h"
/* Private variables ---------------------------------------------------------*/
static bool g_bBleRacpNotifyIsEnableFlag = false;						// BLE SOCP通知使能标志位
/* Private function prototypes -----------------------------------------------*/
uint8_t ble_racp_encode(ble_cgms_racp_datapacket_t* pRacpDatapacket, uint8_t* pData);
void ble_racp_decode(uint8_t ucDataLen, uint8_t* pData, ble_cgms_racp_datapacket_t* pRacpDatapacket);

/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  ble_racp_notify_enable
* Description    :  通知被使能
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_racp_notify_enable(void)
{
	g_bBleRacpNotifyIsEnableFlag = true;
}

/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  ble_racp_notify_disable
* Description    :  通知被失能
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_racp_notify_disable(void)
{
	g_bBleRacpNotifyIsEnableFlag = false;
}

/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  ble_racp_notify_is_enable
* Description    :  判断当前是否使能通知
* Input          :  void
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool ble_racp_notify_is_enable(void)
{
    return g_bBleRacpNotifyIsEnableFlag;
}

/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  ble_racp_decode
* Description    :  解码RACP数据包
* Input          :  uint8_t ucDataLen
* Input          :  uint8_t * pData
* Input          :  ble_cgms_racp_datapacket_t * pRacpDatapacket
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_racp_decode(uint8_t ucDataLen, uint8_t* pData, ble_cgms_racp_datapacket_t* pRacpDatapacket)
{
    pRacpDatapacket->ucDataLen = 0;

    if (ucDataLen > 0)
    {
		pRacpDatapacket->ucOpCode = pData[0];
    }
    if (ucDataLen > 1)
    {
        pRacpDatapacket->ucOperator = pData[1];
    }
    if (ucDataLen > 2)
    {
        pRacpDatapacket->ucDataLen = ucDataLen - 2;
        pRacpDatapacket->pData = &pData[2];
    }
}


/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  ble_racp_encode
* Description    :  编码RACP数据包
* Input          :  ble_cgms_racp_datapacket_t * pRacpDatapacket
* Input          :  uint8_t * pData
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t ble_racp_encode(ble_cgms_racp_datapacket_t* pRacpDatapacket, uint8_t* pData)
{
	uint8_t ucLen = 0;
	if (pData != NULL)
	{
		pData[ucLen++] = pRacpDatapacket->ucOpCode;
		pData[ucLen++] = pRacpDatapacket->ucOperator;

		for (uint8_t i = 0; i < pRacpDatapacket->ucDataLen; i++)pData[ucLen++] = pRacpDatapacket->pData[i];
	}

	uint16_t usCrcValue = do_crc(pData, ucLen);
	ucLen += uint16_encode(usCrcValue, &pData[ucLen]);
	return ucLen;
}



/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  racp_response_code_send
* Description    :  发送历史数据回应包
* Input          :  ble_event_info_t BleEventInfo
* Input          :  uint8_t ucOpcode
* Input          :  uint8_t ucValue
* Output         :  None
* Return         :  void
*******************************************************************************/
void racp_response_code_send(ble_event_info_t BleEventInfo, uint8_t ucOpcode, uint8_t ucValue)
{
	uint8_t ucEncodedRacp[25];
    uint8_t  ucLen;
	ble_cgms_racp_datapacket_t RacpDatapacket;
	RacpDatapacket.ucOpCode = RACP_OPCODE_RESPONSE;
	RacpDatapacket.ucOperator = ucOpcode;
	RacpDatapacket.pData = &ucValue;
	RacpDatapacket.ucDataLen = 1;

    ucLen = ble_racp_encode(&RacpDatapacket, ucEncodedRacp);

#ifdef CGMS_ENCRYPT_ENABLE
    uint8_t cipher[16];
    mbedtls_aes_pkcspadding(ucEncodedRacp, ucLen);
    ucLen = 16;
    cgms_aes128_encrpty(ucEncodedRacp, cipher);
    memcpy(ucEncodedRacp, cipher, 16);
#endif

    if ((ble_racp_notify_is_enable()) && (app_global_get_app_state()->bRecordSendFlag == true) && (app_global_get_app_state()->bBleConnected == true))
    {
        sl_status_t sc;
        sc = sl_bt_gatt_server_send_indication(BleEventInfo.ucConidx, BleEventInfo.usHandle, ucLen, ucEncodedRacp);
		app_global_get_app_state()->bRecordSendFlag = false;
    }

}




/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  racp_findMeasDB
* Description    :  历史数据索引&提取
* Input          :  uint8_t filter
* Input          :  uint16_t operand1
* Input          :  uint16_t operand2
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
static uint8_t racp_findMeasDB(uint8_t filter, uint16_t operand1, uint16_t operand2)
{

	if (0 == cgms_db_get_records_num())return  0;


	switch (filter)
	{
	case RACP_OPERATOR_GREATER_OR_EQUAL:
	{
		if (operand1 >= cgms_db_get_records_num())
		{
			app_global_get_app_state()->RecordOptInfo.usRacpRecordCnt = 0;
			return 0;
		}
		app_global_get_app_state()->RecordOptInfo.usRacpRecordEndIndex = cgms_db_get_records_num() - 1;
		app_global_get_app_state()->RecordOptInfo.usRacpRecordStartIndex = operand1;
		app_global_get_app_state()->RecordOptInfo.usRacpRecordCnt = (app_global_get_app_state()->RecordOptInfo.usRacpRecordEndIndex - app_global_get_app_state()->RecordOptInfo.usRacpRecordStartIndex + 1);
		return 1;
	}
	case RACP_OPERATOR_FIRST:
	{
		app_global_get_app_state()->RecordOptInfo.usRacpRecordStartIndex = 0;
		app_global_get_app_state()->RecordOptInfo.usRacpRecordEndIndex = 0;
		app_global_get_app_state()->RecordOptInfo.usRacpRecordCnt = 1;
		return 1;
	}
	case RACP_OPERATOR_LAST:
	{
		app_global_get_app_state()->RecordOptInfo.usRacpRecordStartIndex = cgms_db_get_records_num() - 1;
		app_global_get_app_state()->RecordOptInfo.usRacpRecordEndIndex = app_global_get_app_state()->RecordOptInfo.usRacpRecordStartIndex;
		app_global_get_app_state()->RecordOptInfo.usRacpRecordCnt = 1;
		return 1;
	}
	case RACP_OPERATOR_LESS_OR_EQUAL:
	{
		app_global_get_app_state()->RecordOptInfo.usRacpRecordEndIndex = operand1 > (cgms_db_get_records_num() - 1) ? (cgms_db_get_records_num() - 1) : operand1;
		app_global_get_app_state()->RecordOptInfo.usRacpRecordStartIndex = 0;
		app_global_get_app_state()->RecordOptInfo.usRacpRecordCnt = (app_global_get_app_state()->RecordOptInfo.usRacpRecordEndIndex - app_global_get_app_state()->RecordOptInfo.usRacpRecordStartIndex + 1);
		return 1;
	}
	case RACP_OPERATOR_ALL:
	case RACP_OPERATOR_RANGE:
	{
		if (operand1 > operand2)return 0;
		if (operand1 >= cgms_db_get_records_num())
		{
			app_global_get_app_state()->RecordOptInfo.usRacpRecordCnt = 0;
			return 0;
		}
		app_global_get_app_state()->RecordOptInfo.usRacpRecordEndIndex = operand2 > (cgms_db_get_records_num() - 1) ? (cgms_db_get_records_num() - 1) : operand2;
		app_global_get_app_state()->RecordOptInfo.usRacpRecordStartIndex = operand1;
		app_global_get_app_state()->RecordOptInfo.usRacpRecordCnt = (app_global_get_app_state()->RecordOptInfo.usRacpRecordEndIndex - app_global_get_app_state()->RecordOptInfo.usRacpRecordStartIndex + 1);
		return 1;
	}
	default:
		break;
	}
	return 0;
}


/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  on_racp_value_write
* Description    :  收到RACP数据包处理函数
* Input          :  ble_event_info_t BleEventInfo
* Input          :  uint16_t usLen
* Input          :  uint8_t* pData
* Output         :  None
* Return         :  void
*******************************************************************************/
void on_racp_value_write(ble_event_info_t BleEventInfo, uint16_t usLen, uint8_t* pData)
{
	uint8_t  ucResponseCode = 0;
	uint16_t usOpand1Start, usOprand2End;
	ble_cgms_racp_datapacket_t RacpDatapacket;

#ifdef CGMS_ENCRYPT_ENABLE
	if (usLen != 16)
	{
		RacpDatapacket.ucOpCode = pData[0];
		ucResponseCode = RACP_RESPONSE_RESULT_COMMAND_LEN_ERR;
	}
	else
	{
		uint8_t decipher[16];
		cgms_aes128_decrpty(pData, decipher);
		memcpy(pData, decipher, usLen);
	}

#endif

	// 如果数据包正常
	if (ucResponseCode == 0)
	{
		// 解码数据包
		ble_racp_decode(usLen, pData, &RacpDatapacket);

		if ((RacpDatapacket.ucOpCode == RACP_OPCODE_REPORT_RECS) && (usLen == 16) && (RacpDatapacket.pData[RacpDatapacket.ucDataLen - 1] != 8))  //decrped cmd, len=16, && AES-pading7
		{
			ucResponseCode = RACP_RESPONSE_RESULT_COMMAND_LEN_ERR;
		}
		else if (RacpDatapacket.ucOpCode != RACP_OPCODE_REPORT_RECS)
		{
			ucResponseCode = RACP_RESPONSE_RESULT_COMMAND_FORMAT_ERR;
		}
		else
		{
			// 效验CRC
			if (do_crc(pData, 8) != 0)
			{
				ucResponseCode = RACP_RESPONSE_RESULT_COMMAND_OTHER_ERR;
			}
			else
			{
				// 判断当前是否有历史数据操作在执行
				if (app_global_get_app_state()->bRecordSendFlag == true)
				{
					ucResponseCode = RACP_RESPONSE_RESULT_COMMAND_BUSY;
				}
				else
				{
					// 提取开始和结束的index
					usOpand1Start = uint16_decode(RacpDatapacket.pData);
					usOprand2End = uint16_decode(&RacpDatapacket.pData[2]);

					// 寻找符合条件的历史数据
					uint8_t ucRet = racp_findMeasDB(RacpDatapacket.ucOpCode, usOpand1Start, usOprand2End);

					// 如果找不到符合的数据
					if (ucRet == 0)
					{
						ucResponseCode = RACP_RESPONSE_RESULT_COMMAND_START_INDEX_RANGE_OUT;
					}
					else
					{
						ucResponseCode = RACP_RESPONSE_RESULT_SUCCESS;

						app_global_get_app_state()->bRecordSendFlag = true;

						// 清空发送累计
						app_global_get_app_state()->RecordOptInfo.usRacpRecordSendCnt = 0;

						// 记录本次历史数据操作的BLE事件信息
						app_global_get_app_state()->RecordOptInfo.BleEventInfo = BleEventInfo;

						// 开始发送数据记录
						app_glucose_meas_record_send_start();
					}
				}
			}
		}
	}

	// 发送回应包
	racp_response_code_send(BleEventInfo, RacpDatapacket.ucOpCode, ucResponseCode);
}



/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/






