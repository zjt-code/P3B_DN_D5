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
#if !defined(LOG_TAG)
#define LOG_TAG                   "CGMS_MEAS"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_INFO


#include <stdint.h>
#include <string.h>
#include "cgms_meas.h"
#include "cgms_crc.h"
#include "ble_customss.h"
#include "app_util.h"
#include "cgms_aes128.h"
#include <elog.h>
/* Private variables ---------------------------------------------------------*/
static bool g_bBleMeasNotifyIsEnableFlag = false;						// BLE Meas通知使能标志位
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  ble_meas_notify_enable
* Description    :  通知被使能
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_meas_notify_enable(void)
{
    g_bBleMeasNotifyIsEnableFlag = true;
}

/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  ble_meas_notify_disable
* Description    :  通知被失能
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_meas_notify_disable(void)
{
    g_bBleMeasNotifyIsEnableFlag = false;
}

/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  ble_meas_notify_is_enable
* Description    :  判断当前是否使能通知
* Input          :  void
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool ble_meas_notify_is_enable(void)
{
    return g_bBleMeasNotifyIsEnableFlag;
}



/*******************************************************************************
*                           陈苏阳@2023-11-15
* Function Name  :  cgms_meas_special_send
* Description    :  BLE发送特殊血糖数据包
* Input          :  ble_event_info_t BleEventInfo
* Input          :  cgms_history_special_datapcket_t CgmsHistorySpecialDatapcket
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
ret_code_t cgms_meas_special_send(ble_event_info_t BleEventInfo, cgms_history_special_datapcket_t CgmsHistorySpecialDatapcket)
{
    if ((ble_meas_notify_is_enable()) && (app_global_get_app_state()->bSentSocpSuccess == true) && (app_global_get_app_state()->bBleConnected == true))
    {
        uint8_t ucLen = sizeof(CgmsHistorySpecialDatapcket);
        uint8_t ucDatapacketBuffer[20];
        memcpy(ucDatapacketBuffer, &CgmsHistorySpecialDatapcket, ucLen);
        elog_hexdump("cgms_meas_send", 16, ucDatapacketBuffer, ucLen);

#ifdef CGMS_ENCRYPT_ENABLE

        mbedtls_aes_pkcspadding(&CgmsHistorySpecialDatapcket, ucLen);
        ucLen = 16;
        uint8_t cipher[16];
        cgms_aes128_encrpty(ucDatapacketBuffer, cipher);
        memcpy(ucDatapacketBuffer, cipher, 16);
#endif

        // 发送数据
        sl_status_t sc;
        sc = sl_bt_gatt_server_send_indication(BleEventInfo.ucConidx, BleEventInfo.usHandle, ucLen, ucDatapacketBuffer);
        if (sc == RET_CODE_SUCCESS)
        {
            app_global_get_app_state()->bRecordSendFlag = false;
            app_global_get_app_state()->bSentSocpSuccess = false;
            return RET_CODE_SUCCESS;
        }
    }
    return RET_CODE_FAIL;
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
    if (ble_meas_notify_is_enable()  && (app_global_get_app_state()->bBleConnected == true))
    {
        uint8_t ucLen = sizeof(Rec);
        uint8_t ucDatapacketBuffer[20];
        memcpy(ucDatapacketBuffer, &Rec, ucLen);
        elog_hexdump("cgms_meas_send", 8, ucDatapacketBuffer, ucLen);

#ifdef CGMS_ENCRYPT_ENABLE

        mbedtls_aes_pkcspadding(&Rec, ucLen);
        ucLen = 16;
        uint8_t cipher[16];
        cgms_aes128_encrpty(ucDatapacketBuffer, cipher);
        memcpy(ucDatapacketBuffer, cipher, 16);
#endif
        elog_hexdump("cgms_meas_send(encrpty)", 8, ucDatapacketBuffer, ucLen);

        // 发送数据
        sl_status_t sc;
        sc = sl_bt_gatt_server_send_notification(BleEventInfo.ucConidx, BleEventInfo.usHandle, ucLen, ucDatapacketBuffer);
        if (sc == RET_CODE_SUCCESS)
        {
            log_i("send OK");
            app_global_get_app_state()->bRecordSendFlag = false;
            app_global_get_app_state()->bSentSocpSuccess = false;
            return RET_CODE_SUCCESS;
        }
        else
        {
            log_e("sl_bt_gatt_server_send_notification fail:%d", sc);
        }
    }
    return RET_CODE_FAIL;
}





/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/





