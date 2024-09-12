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
#if !defined(LOG_TAG)
#define LOG_TAG                   "CGMS_RACP"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_DEBUG

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
#include <elog.h>
#if ((USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL) ||(USE_BLE_PROTOCOL==GN_2_PROTOCOL))
#include "cgms_aes128.h"
#endif
//#include "app_glucose_meas.h"
/* Private variables ---------------------------------------------------------*/
static bool g_bBleRacpNotifyIsEnableFlag = false;						// BLE ROCP通知使能标志位
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
        memcpy(pRacpDatapacket->ucData, &pData[2],pRacpDatapacket->ucDataLen);
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

		for (uint8_t i = 0; i < pRacpDatapacket->ucDataLen; i++)pData[ucLen++] = pRacpDatapacket->ucData[i];
	}
#if USE_GN_2_PROTOCOL
	uint16_t usCrcValue = do_crc(pData, ucLen);
	ucLen += uint16_encode(usCrcValue, &pData[ucLen]);
#endif
	return ucLen;
}


#if USE_GN_2_PROTOCOL
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

    if ((ble_racp_notify_is_enable()) && (app_global_get_app_state()->bBleConnected == true))
    {
        sl_status_t sc;
        sc = sl_bt_gatt_server_send_notification(BleEventInfo.ucConidx, BleEventInfo.usHandle, ucLen, ucEncodedRacp);
		if (sc != SL_STATUS_OK)
		{
			log_e("sl_bt_gatt_server_send_notification fail:%d",sc);
		}
    }
}
#endif
/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  racp_response_send
* Description    :  发送历史数据回应包
* Input          :  ble_event_info_t BleEventInfo
* Input          :  racp_response_t ResponseCode
* Input          :  ble_cgms_racp_datapacket_t RacpRspDatapacket
* Output         :  None
* Return         :  void
*******************************************************************************/
void racp_response_send(ble_event_info_t BleEventInfo, racp_response_t ResponseCode,ble_cgms_racp_datapacket_t RacpRspDatapacket)
{
	uint8_t ucEncodedRacp[25];
    uint8_t  ucLen;
    ble_cgms_racp_datapacket_t Datapacket;
    uint8_t ucOpCode = RacpRspDatapacket.ucOpCode;

    if (RACP_OPCODE_REPORT_NUM_RECS == ucOpCode)
    {
        Datapacket.ucOpCode = RACP_OPCODE_NUM_RECS_RESPONSE;
    }
    else
    {
        Datapacket.ucOpCode = RACP_OPCODE_RESPONSE_CODE;
    }
    Datapacket.ucOperator = RACP_OPERATOR_NULL;
    Datapacket.ucDataLen = 2;
    memcpy(Datapacket.ucData,RacpRspDatapacket.ucData,RacpRspDatapacket.ucDataLen);
    if (RACP_OPCODE_REPORT_NUM_RECS == ucOpCode)
    {
        if (RACP_RESPONSE_RESULT_SUCCESS != ResponseCode)
        {
        	Datapacket.ucData[0] = ucOpCode;
        	Datapacket.ucData[1] = ResponseCode;
        }
    }
    else
    {
    	Datapacket.ucData[0] = ucOpCode;
    	Datapacket.ucData[1] = ResponseCode;
    }

    ucLen = ble_racp_encode(&Datapacket, ucEncodedRacp);



    if ((ble_racp_notify_is_enable()) && app_have_a_active_ble_connect())
    {
#if ((USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL) ||(USE_BLE_PROTOCOL==GN_2_PROTOCOL))
        uint8_t ucCipher[16];
        mbedtls_aes_pkcspadding(ucEncodedRacp, ucLen);
        ucLen = 16;
        cgms_aes128_encrpty(ucEncodedRacp, ucCipher);
        memcpy(ucEncodedRacp, ucCipher, ucLen);
        elog_hexdump("racp_send(encrpty)", 8, ucEncodedRacp, ucLen);
#else
        elog_hexdump("racp_send", 8, ucEncodedRacp, ucLen);

#endif

        // 发送数据包
        sl_status_t sc;
        sc = sl_bt_gatt_server_send_notification(BleEventInfo.ucConidx, BleEventInfo.usHandle, ucLen, ucEncodedRacp);
		if (sc != SL_STATUS_OK)
		{
			log_e("sl_bt_gatt_server_send_notification fail:%d",sc);
		}
    }
	else
	{
		if (ble_racp_notify_is_enable() == false)
		{
			log_w("ble racp notify is not enable");
		}
		if (!app_have_a_active_ble_connect())
		{
			log_w("ble is disconnect");
		}
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
*                           陈苏阳@2024-06-25
* Function Name  :  cgms_racp_report_num_recs
* Description    :  报告当前历史数据数量
* Input          :  ble_event_info_t BleEventInfo
* Input          :  ble_cgms_racp_datapacket_t RacpDatapacket
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_racp_report_num_recs(ble_event_info_t BleEventInfo, ble_cgms_racp_datapacket_t RacpDatapacket)
{
    racp_response_t ResponseCode = RACP_RESPONSE_RESULT_SUCCESS;
    ble_cgms_racp_datapacket_t RspDatapacket;
    memset(&RspDatapacket, 0x00, sizeof(RspDatapacket));
    RspDatapacket.ucOpCode = RacpDatapacket.ucOpCode;
    uint16_t usRacpRecordCnt = cgms_db_get_records_num();
    RspDatapacket.ucData[0] = usRacpRecordCnt & 0xFF;
    RspDatapacket.ucData[1] = (usRacpRecordCnt >> 8) & 0xFF;
    RspDatapacket.ucDataLen = 2;
    log_i("report record number:%d", usRacpRecordCnt);
    ResponseCode = RACP_RESPONSE_RESULT_SUCCESS;
    // 发送回应包
    racp_response_send(BleEventInfo, ResponseCode, RspDatapacket);
}

/*******************************************************************************
*                           陈苏阳@2024-06-25
* Function Name  :  cgms_racp_delete_recs
* Description    :  删除历史数据
* Input          :  ble_event_info_t BleEventInfo
* Input          :  ble_cgms_racp_datapacket_t RacpDatapacket
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_racp_delete_recs(ble_event_info_t BleEventInfo, ble_cgms_racp_datapacket_t RacpDatapacket)
{
    racp_response_t ResponseCode = RACP_RESPONSE_RESULT_SUCCESS;
    ble_cgms_racp_datapacket_t RspDatapacket;
    memset(&RspDatapacket, 0x00, sizeof(RspDatapacket));
    RspDatapacket.ucOpCode = RacpDatapacket.ucOpCode;
    ResponseCode = RACP_RESPONSE_RESULT_SUCCESS;
    // 发送回应包
    racp_response_send(BleEventInfo, ResponseCode, RspDatapacket);
}

/*******************************************************************************
*                           陈苏阳@2024-06-25
* Function Name  :  cgms_racp_report_recs
* Description    :  上报历史数据
* Input          :  ble_event_info_t BleEventInfo
* Input          :  ble_cgms_racp_datapacket_t RacpDatapacket
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_racp_report_recs(ble_event_info_t BleEventInfo, ble_cgms_racp_datapacket_t RacpDatapacket)
{
    racp_response_t ResponseCode = RACP_RESPONSE_RESULT_SUCCESS;
    uint16_t usfilterData1 = 0x00;
    uint16_t usfilterData2 = 0x00;
    ble_cgms_racp_datapacket_t RspDatapacket;
    memset(&RspDatapacket, 0x00, sizeof(RspDatapacket));
    RspDatapacket.ucOpCode = RacpDatapacket.ucOpCode;
    // 效验CRC
    if (0 == RacpDatapacket.ucOperator)
    {
        log_w("RACP_RESPONSE_RESULT_INVALID_OPERATOR");
        ResponseCode = RACP_RESPONSE_RESULT_INVALID_OPERATOR;
        // 发送回应包
        racp_response_send(BleEventInfo, ResponseCode, RspDatapacket);
    }

    // 发送历史数据发送结束数据包
    if (RacpDatapacket.ucOperator >= 7)
    {
        log_w("RACP_RESPONSE_RESULT_OPERATOR_UNSUPPORTED");
        ResponseCode = RACP_RESPONSE_RESULT_OPERATOR_UNSUPPORTED;
        // 发送回应包
        racp_response_send(BleEventInfo, ResponseCode, RspDatapacket);
    }

    if (RacpDatapacket.ucOperator == RACP_OPERATOR_LESS_OR_EQUAL || RacpDatapacket.ucOperator == RACP_OPERATOR_GREATER_OR_EQUAL || RacpDatapacket.ucOperator == RACP_OPERATOR_RANGE)
    {

        if (RacpDatapacket.ucData[0] != 0x01)
        {
            log_w("RACP_RESPONSE_RESULT_OPERAND_UNSUPPORTED");
            ResponseCode = RACP_RESPONSE_RESULT_OPERAND_UNSUPPORTED;
            // 发送回应包
            racp_response_send(BleEventInfo, ResponseCode, RspDatapacket);
        }
        usfilterData1 = uint16_decode(&RacpDatapacket.ucData[1]);
    }

    if (RacpDatapacket.ucOperator == RACP_OPERATOR_RANGE) usfilterData2 = uint16_decode(&RacpDatapacket.ucData[3]);

    //Test the operands are valid
    if ((RacpDatapacket.ucOperator == RACP_OPERATOR_ALL && RacpDatapacket.ucDataLen > 0) || (RacpDatapacket.ucOperator == RACP_OPERATOR_RANGE && usfilterData1 > usfilterData2))
    {
        log_w("RACP_RESPONSE_RESULT_INVALID_OPERAND");
        ResponseCode = RACP_RESPONSE_RESULT_INVALID_OPERAND;
        // 发送回应包
        racp_response_send(BleEventInfo, ResponseCode, RspDatapacket);
    }

    log_i("racp_findMeasDB %d/%d", usfilterData1, usfilterData2);
    //Get the starting and ending index of the record meeting requriement  
    if (racp_findMeasDB(RacpDatapacket.ucOperator, usfilterData1, usfilterData2))
    {

        ResponseCode = RACP_RESPONSE_RESULT_SUCCESS;

        // 清空发送累计
        app_global_get_app_state()->RecordOptInfo.usRacpRecordSendCnt = 0;

        // 记录本次历史数据操作的BLE事件信息
        app_global_get_app_state()->RecordOptInfo.BleEventInfo = BleEventInfo;

        // 开始发送数据记录
        app_glucose_meas_record_send_start();
        return;

    }
    //If search is not successful, indicate the result.
    else
    {
        log_w("RACP_RESPONSE_RESULT_NO_RECORDS_FOUND");
        ResponseCode = RACP_RESPONSE_RESULT_NO_RECORDS_FOUND;

        // 发送回应包
        racp_response_send(BleEventInfo, ResponseCode, RspDatapacket);
    }
}

/*******************************************************************************
*                           陈苏阳@2024-06-25
* Function Name  :  cgms_racp_abort_operation
* Description    :  中止操作
* Input          :  ble_event_info_t BleEventInfo
* Input          :  ble_cgms_racp_datapacket_t RacpDatapacket
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_racp_abort_operation(ble_event_info_t BleEventInfo, ble_cgms_racp_datapacket_t RacpDatapacket)
{
    racp_response_t ResponseCode = RACP_RESPONSE_RESULT_SUCCESS;
    ble_cgms_racp_datapacket_t RspDatapacket;
    memset(&RspDatapacket, 0x00, sizeof(RspDatapacket));
    // 停止发送数据记录
    app_glucose_meas_record_send_stop();
    ResponseCode = RACP_RESPONSE_RESULT_SUCCESS;

    // 发送回应包
    racp_response_send(BleEventInfo, ResponseCode, RspDatapacket);
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
    ble_cgms_racp_datapacket_t RacpDatapacket;
    ble_cgms_racp_datapacket_t RspDatapacket;
    racp_response_t ResponseCode = RACP_RESPONSE_RESULT_SUCCESS;
    memset(&RspDatapacket, 0x00, sizeof(RspDatapacket));
#if ((USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL) ||(USE_BLE_PROTOCOL==GN_2_PROTOCOL))
    uint8_t ucTempDatapacketBuffer[16];
    elog_hexdump("racp_rev", 16,pData, usLen);
    cgms_aes128_decrpty(pData, ucTempDatapacketBuffer);
    memcpy(pData, ucTempDatapacketBuffer, 16);
#endif
    ble_racp_decode(usLen, pData, &RacpDatapacket);
    elog_hexdump("datapacket", 16, pData, usLen);
    RspDatapacket.ucOpCode = RacpDatapacket.ucOpCode;

    switch (RacpDatapacket.ucOpCode)
    {
    case RACP_OPCODE_REPORT_RECS:
    {
        cgms_racp_report_recs(BleEventInfo, RacpDatapacket);
        return;
    }
    case RACP_OPCODE_REPORT_NUM_RECS:
    {
        cgms_racp_report_num_recs(BleEventInfo, RacpDatapacket);
        return;
    }
    case RACP_OPCODE_DELETE_RECS:
    {
        cgms_racp_delete_recs(BleEventInfo, RacpDatapacket);
        return;
    }
    case RACP_OPCODE_ABORT_OPERATION:

        cgms_racp_abort_operation(BleEventInfo, RacpDatapacket);
        return;
    default:
    {
        ble_cgms_racp_datapacket_t RspDatapacket;
        memset(&RspDatapacket, 0x00, sizeof(RspDatapacket));
        log_w("RACP_RESPONSE_RESULT_OPCODE_UNSUPPORTED");
        // Respond with error code
        ResponseCode = RACP_RESPONSE_RESULT_OPCODE_UNSUPPORTED;
        // 发送回应包
        racp_response_send(BleEventInfo, ResponseCode, RspDatapacket);
        return;
    }
    }
    // 发送回应包
    racp_response_send(BleEventInfo, ResponseCode, RspDatapacket);
}



/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/






