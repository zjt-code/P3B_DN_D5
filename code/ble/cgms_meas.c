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
#include "sl_bluetooth.h"
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
    if (ble_meas_notify_is_enable() && app_have_a_active_ble_connect())
    {
        uint8_t ucLen = sizeof(CgmsHistorySpecialDatapcket);
        uint8_t ucDatapacketBuffer[20];
        memcpy(ucDatapacketBuffer, &CgmsHistorySpecialDatapcket, ucLen);
        elog_hexdump("cgms_meas_send", 16, ucDatapacketBuffer, ucLen);

#if ((USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL) ||(USE_BLE_PROTOCOL==GN_2_PROTOCOL))

        mbedtls_aes_pkcspadding(&CgmsHistorySpecialDatapcket, ucLen);
        ucLen = 16;
        uint8_t cipher[16];
        cgms_aes128_encrpty(ucDatapacketBuffer, cipher);
        memcpy(ucDatapacketBuffer, cipher, 16);
#endif

        // 发送数据
        sl_status_t sc;
        sc = sl_bt_gatt_server_send_notification(BleEventInfo.ucConidx, BleEventInfo.usHandle, ucLen, ucDatapacketBuffer);
        if (sc == RET_CODE_SUCCESS)
        {
            app_global_get_app_state()->bRecordSendFlag = false;
            app_global_get_app_state()->bSentSocpSuccess = false;
            return RET_CODE_SUCCESS;
        }
        else
        {
            log_e("sl_bt_gatt_server_send_notification fail:%d,%d,,%d", sc, BleEventInfo.ucConidx, BleEventInfo.usHandle);
        }
    }
    else if(ble_meas_notify_is_enable()==false)
    {
        log_e("ble_meas_notify_is_disable");
    }
    else if (!app_have_a_active_ble_connect())
    {
        log_e("ble is disconneced.");
    }
    return RET_CODE_FAIL;
}


/*******************************************************************************
*                           陈苏阳@2024-02-23
* Function Name  :  cgms_meas_encode
* Description    :  编码数据包
* Input          :  cgms_meas_t Rec
* Input          :  uint8_t * pEncodeedData
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_meas_encode(cgms_meas_t Rec, uint8_t* pEncodeedData)
{
    if (pEncodeedData)
    {
        uint8_t len = 0;
        pEncodeedData[0] = 0x0D;//在安卓端的E2 APP上限制了非0x0D开头的数据都丢弃//len + 2;
        len++;

        pEncodeedData[1] = Rec.usHistoryFlag;
        len++;

        // 编码血糖数据
        len += uint16_encode(Rec.usGlucose, &pEncodeedData[len]); //len =4

        // 编码时间下标
        len += uint16_encode(Rec.usOffset, &pEncodeedData[len]); //len =6

        if (Rec.usHistoryFlag == CGMS_MEAS_HISTORY_FLAG_HISTORY)
        {
            pEncodeedData[len] = 0x80;
        }
        else
        {
            pEncodeedData[len] = 0x00;
        }
        len++;

        // 编码趋势数据(固定为0)
        len += uint16_encode(0, &pEncodeedData[len]);

        // 编码电流数据
        len += uint16_encode(Rec.usCurrent, &pEncodeedData[len]);
        len += uint16_encode(do_crc(pEncodeedData, 14), &pEncodeedData[14]);
        return 16;
    }
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
	uint8_t ucDatapacketBuffer[20];
    if (ble_meas_notify_is_enable() && app_have_a_active_ble_connect())
    {
        uint32_t uiTotalBytes;
        uint32_t uiFreeBytes;
        // 根据协议栈buffer的空余程度来决定是否发送数据
        if (sl_bt_resource_get_status(&uiTotalBytes, &uiFreeBytes) == SL_STATUS_OK)
        {
            if (uiFreeBytes < uiTotalBytes / 2)
            {
                log_w("bluetooth stack memory buffer free bytes is too little:%d/%d", uiFreeBytes,uiTotalBytes);
                return RET_CODE_FAIL;
            }
        }

        
#if ((USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL) ||(USE_BLE_PROTOCOL==GN_2_PROTOCOL))
        uint8_t ucLen = 16;
        uint8_t cipher[16];
        memcpy(ucDatapacketBuffer, &Rec, ucLen);
        mbedtls_aes_pkcspadding(ucDatapacketBuffer, ucLen);
        cgms_aes128_encrpty(ucDatapacketBuffer, cipher);
        memcpy(ucDatapacketBuffer, cipher, 16);
		elog_hexdump("cgms_meas_send(encrpty)", 8, ucDatapacketBuffer, ucLen);
#else
        uint8_t ucLen = cgms_meas_encode(Rec,ucDatapacketBuffer);
        elog_hexdump("cgms_meas_send", 8, ucDatapacketBuffer, ucLen);
        
#endif
        // 发送数据
        sl_status_t sc;
        sc = sl_bt_gatt_server_send_notification(BleEventInfo.ucConidx, BleEventInfo.usHandle, ucLen, ucDatapacketBuffer);
        if (sc == SL_STATUS_OK)
        {
            log_i("send OK");
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





