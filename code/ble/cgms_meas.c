/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_meas.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  16/5/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/


#include <stdint.h>
#include <string.h>
#include "cgms_meas.h"
//#include "cgms_db.h"
#include "cgms_crc.h"
#include "ble_customss.h"
#include "app_util.h"
#include "cgms_aes128.h"
#include "app_log.h"
/* Private variables ---------------------------------------------------------*/
  
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
*                           陈苏阳@2022-12-21
* Function Name  :  cgms_meas_send_raw
* Description    :  BLE发送血糖测量结果原始数据
* Input          :  ble_event_info_t BleEventInfo
* Input          :  uint8_t * pData
* Input          :  uint8_t ucLen
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
ret_code_t cgms_meas_send_raw(ble_event_info_t BleEventInfo,uint8_t* pData, uint8_t ucLen)
{
    uint8_t ucDatapacketBuffer[20];

    memcpy(ucDatapacketBuffer, pData, ucLen);

    app_log_info("cgms_meas_send:");
    app_log_hexdump_info(ucDatapacketBuffer, ucLen);

#ifdef CGMS_ENCRYPT_ENABLE

    mbedtls_aes_pkcspadding(pData, ucLen);
    ucLen = 16;
    uint8_t cipher[16];
    cgms_aes128_encrpty(ucDatapacketBuffer, cipher);
    memcpy(ucDatapacketBuffer,cipher,16);
#endif

    //GATTC_SendEvtCmd(BleEventInfo.ucConidx, GATTC_NOTIFY, 0, att_get_att_handle(CS_IDX_CGM_MEASUREMENT_VAL), ucLen, ucDatapacketBuffer);
    return 0;
}

/*******************************************************************************
*                           陈苏阳@2022-12-16
* Function Name  :  cgms_meas_send
* Description    :  BLE发送血糖测量结果
* Input          :  ble_event_info_t BleEventInfo
* Input          :  cgms_meas_t Rec
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
ret_code_t cgms_meas_send(ble_event_info_t BleEventInfo, cgms_meas_t Rec)
{
    uint8_t ucSendBuffer[sizeof(cgms_meas_t)];
    // 计算数据长度
    Rec.ucDatapacketLen = sizeof(cgms_meas_t);

    // 计算CRC
    Rec.usCRC16 = do_crc((uint8_t*)&Rec, sizeof(cgms_meas_t) - 2);

    // 发送数据
    return cgms_meas_send_raw(BleEventInfo,ucSendBuffer, sizeof(cgms_meas_t));
}





/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/





