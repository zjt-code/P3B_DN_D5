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
#include "sl_sleeptimer.h"
#include "ble_update_connect_param.h"
#include "app.h"
#include <elog.h>
/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/



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
    BleConnectInfo_t* p = (BleConnectInfo_t*)data;
    if (p)
    {
        // 如果当前保持连接,且未更新连接参数
        if (p->bIsConnected && p->bIsUpdateConnectParameter == false)
        {
            // 触发开始更新连接参数
            ble_update_connect_param(p->usBleConidx);
        }
    }
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
* Function Name  :  ble_update_connect_param
* Description    :  更新连接参数
* Input          :  uint16_t usConnectionHandle
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_update_connect_param(uint16_t usConnectionHandle)
{
    sl_bt_connection_set_parameters(usConnectionHandle, BLE_PRE_INTERVAL_MIN, BLE_PRE_INTERVAL_MAX, BLE_PRE_LATENCY, BLE_PRE_TIMEOUT, 0xffff, 0xffff);
}


/*******************************************************************************
*                           陈苏阳@2023-10-25
* Function Name  :  ble_update_connect_param_stop
* Description    :  停止ble连接参数更新
* Input          :  uint16_t usConnectionHandle
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_update_connect_param_stop(uint16_t usConnectionHandle)
{
    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        // 找到对应的连接信息
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true && app_global_get_app_state()->BleConnectInfo[i].usBleConidx == usConnectionHandle)
        {
            sl_sleeptimer_stop_timer(&(app_global_get_app_state()->BleConnectInfo[i].BleUpdateConnectParamTimer));
        }
    }
}

/*******************************************************************************
*                           陈苏阳@2023-10-25
* Function Name  :  ble_update_connect_param_all_stop
* Description    :  停止全部连接的BLE连接参数更新
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_update_connect_param_all_stop(void)
{
    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        // 找到对应的连接信息
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true)
        {
            sl_sleeptimer_stop_timer(&(app_global_get_app_state()->BleConnectInfo[i].BleUpdateConnectParamTimer));
        }
    }
}

/*******************************************************************************
*                           陈苏阳@2023-10-25
* Function Name  :  ble_update_connect_param_start
* Description    :  开始更新BLE连接参数
* Input          :  uint16_t usConnectionHandle
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_update_connect_param_start(uint16_t usConnectionHandle)
{
    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        // 找到对应的连接信息
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true && app_global_get_app_state()->BleConnectInfo[i].usBleConidx == usConnectionHandle)
        {
            if (app_global_get_app_state()->BleConnectInfo[i].bIsUpdateConnectParameter == false)
            {
                // 如果当前连接参数不符合要求
                if (ble_update_connect_param_is_pass(usConnectionHandle) == false)
                {
                    // 触发连接参数更新定时器
                    sl_sleeptimer_start_timer(&(app_global_get_app_state()->BleConnectInfo[i].BleUpdateConnectParamTimer), sl_sleeptimer_ms_to_tick(BLE_CONNECT_PARAM_UPDATE_DELAY),ble_update_connect_param_timer_callback,(void*)&app_global_get_app_state()->BleConnectInfo[i],0,0);
                }
                else
                {
                    // 如果当前连接参数符合要求
                    app_global_get_app_state()->BleConnectInfo[i].bIsUpdateConnectParameter = true;
                }
            }
        }
    }
}


/*******************************************************************************
*                           陈苏阳@2023-10-25
* Function Name  :  ble_update_connect_param_timer_handle
* Description    :  BLE连接参数更新定时器处理函数
* Input          :  uint16_t usConnectionHandle
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_update_connect_param_timer_handle(uint16_t usConnectionHandle)
{
    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        // 找到对应的连接信息
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true && app_global_get_app_state()->BleConnectInfo[i].usBleConidx == usConnectionHandle)
        {
            if (app_global_get_app_state()->BleConnectInfo[i].bIsUpdateConnectParameter == false)
            {
                // 如果当前连接参数不符合要求
                if (ble_update_connect_param_is_pass(usConnectionHandle) == false)
                {
                    // 申请更新连接参数
                    ble_update_connect_param(usConnectionHandle);

                    // 触发连接参数更新定时器  todo:多连接时需要额外处理
                    sl_sleeptimer_restart_timer(&(app_global_get_app_state()->BleConnectInfo[i].BleUpdateConnectParamTimer),BLE_CONNECT_PARAM_UPDATE_DELAY*1000,ble_update_connect_param_timer_callback,(void*)NULL,0,0);
                }
                else
                {
                    // 如果当前连接参数符合要求
                    app_global_get_app_state()->BleConnectInfo[i].bIsUpdateConnectParameter = true;
                }
            }
        }
    }
}



/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




