/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  ble_update_connect_param.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  25/10/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#if !defined(LOG_TAG)
#define LOG_TAG                   "BLE_UPDATE_CONNECT_PARAM"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_INFO


#include "sl_sleeptimer.h"
#include "ble_update_connect_param.h"
#include "app.h"
#include "sl_bt_api.h"
#include <elog.h>
/* Private variables ---------------------------------------------------------*/

sl_sleeptimer_timer_handle_t g_BleUpdateConnectParamTimer;
bool g_bBleUpdateConnectParamEnableFlag = false;
/* Private function prototypes -----------------------------------------------*/
bool ble_update_connect_param_is_pass(uint16_t usConnectionHandle);


/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
*                           陈苏阳@2023-11-03
* Function Name  :  ble_update_connect_param_timer_callback
* Description    :  更新连接参数定时器回调
* Input          :  sl_sleeptimer_timer_handle_t * handle
* Input          :  void * data
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_update_connect_param_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
    log_d("ble_update_connect_param_timer_callback");
    // 发送事件
    event_push(MAIN_LOOP_EVENT_BLE_UPDATE_CONNECT_PARAMETERS);
}


/*******************************************************************************
*                           陈苏阳@2023-10-25
* Function Name  :  ble_update_connect_param_is_pass
* Description    :  判断一个连接参数是否可用
* Input          :  uint16_t usConnectionHandle
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool ble_update_connect_param_is_pass(uint16_t usConnectionHandle)
{
    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        // 找到对应的连接信息
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true && app_global_get_app_state()->BleConnectInfo[i].usBleConidx == usConnectionHandle)
        {
            if ((app_global_get_app_state()->BleConnectInfo[i].usConnectInterval < BLE_PRE_INTERVAL_MIN) || (app_global_get_app_state()->BleConnectInfo[i].usConnectInterval > BLE_PRE_INTERVAL_MAX) || (app_global_get_app_state()->BleConnectInfo[i].usConnectLatency != BLE_PRE_LATENCY))
            {
                return false;
            }
        }
    }
    return true;
}


/*******************************************************************************
*                           陈苏阳@2023-10-25
* Function Name  :  ble_update_connect_param_handle
* Description    :  更新连接参数
* Input          :  None
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_update_connect_param_handle(void)
{
    bool bRestartTimerFlag = false;
    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        // 如果当前连接的连接参数还不满足要求
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true && ble_update_connect_param_is_pass(app_global_get_app_state()->BleConnectInfo[i].usBleConidx)==false)
        {
            // 已达到需要更新连接参数的时间
            if (app_global_get_app_state()->BleConnectInfo[i].ulConenctedTimeCnt >= 5)
            {
                // 触发更新
                sl_status_t sc = sl_bt_connection_set_parameters(app_global_get_app_state()->BleConnectInfo[i].usBleConidx, BLE_PRE_INTERVAL_MIN, BLE_PRE_INTERVAL_MAX, BLE_PRE_LATENCY, BLE_PRE_TIMEOUT, 0xffff, 0xffff);
                if (sc != SL_STATUS_OK)
                {
                    log_e("sl_bt_connection_set_parameters fail:%d", sc);
                }
            }
            else
            {
                // 累计时间
                app_global_get_app_state()->BleConnectInfo[i].ulConenctedTimeCnt++;
            }
            bRestartTimerFlag = true;
        }
    }
    // 如果需要重启定时器
    if (bRestartTimerFlag && g_bBleUpdateConnectParamEnableFlag)
    {
        // 触发连接参数更新定时器
        sl_sleeptimer_restart_timer(&g_BleUpdateConnectParamTimer, sl_sleeptimer_ms_to_tick(1000), ble_update_connect_param_timer_callback,NULL, 0, 0);

    }
    
}


/*******************************************************************************
*                           陈苏阳@2023-10-25
* Function Name  :  ble_update_connect_param_stop
* Description    :  停止ble连接参数更新
* Input          :  None
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_update_connect_param_stop(void)
{
    sl_sleeptimer_stop_timer(&g_BleUpdateConnectParamTimer);
    g_bBleUpdateConnectParamEnableFlag = false;
}


/*******************************************************************************
*                           陈苏阳@2023-10-25
* Function Name  :  ble_update_connect_param_start
* Description    :  开始更新BLE连接参数
* Input          :  None
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_update_connect_param_start(void)
{
    // 触发连接参数更新定时器
    sl_sleeptimer_start_timer(&g_BleUpdateConnectParamTimer, sl_sleeptimer_ms_to_tick(1000), ble_update_connect_param_timer_callback, NULL, 0, 0);
    g_bBleUpdateConnectParamEnableFlag = true;
}



/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




