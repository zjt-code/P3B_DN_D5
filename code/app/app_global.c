/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  app_global.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  23/10/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "app_global.h"
#include "app.h"
#include "ble_update_connect_param.h"
#include "string.h"
#include "ble_adv.h"
#include "app_log.h"
/* Private variables ---------------------------------------------------------*/
app_state_t g_app_state;

        
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  app_global_get_app_state
* Description    :  获取app_state结构体指针
* Input          :  void
* Output         :  None
* Return         :  app_state_t*
*******************************************************************************/
app_state_t* app_global_get_app_state(void)
{
    return &g_app_state;
}


/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  app_global_is_session_runing
* Description    :  判断CGM是否在运行
* Input          :  void
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool app_global_is_session_runing(void)
{
    switch (app_global_get_app_state()->status)
    {
    case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_RUNNING:
    case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_SENSOR_ABNORMAL:
    case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_SENSOR_ERROR:
    case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_WARM_UP:
        return true;
    default:
        return false;
    }
}

/*******************************************************************************
*                           陈苏阳@2023-10-25
* Function Name  :  app_ble_connect_info_init
* Description    :  初始化一个BLE连接信息结构体内容
* Input          :  BleConnectInfo_t * p
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_ble_connect_info_init(BleConnectInfo_t* p)
{
    if (p)
    {
        p->bIsConnected = false;
        p->bIsUpdateConnectParameter = false;
        p->usBleConidx = 0;
        p->usConnectInterval = 0;
        p->usConnectLatency = 0;
        p->usConnectTimeout = 0;
        p->usUpdateConnectParameterTimerHandle = 0;
    }
}


/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  app_add_new_ble_connect
* Description    :  添加一个新的BLE连接
* Input          :  uint16_t usConnectionHandle
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_add_new_ble_connect(uint16_t usConnectionHandle)
{
    // 如果有重复的,直接退出
    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true && app_global_get_app_state()->BleConnectInfo[i].usBleConidx == usConnectionHandle)return;
    }

    // 找到一个未连接的位置,添加信息
    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == false)
        {
            app_global_get_app_state()->BleConnectInfo[i].bIsConnected = true;
            app_global_get_app_state()->BleConnectInfo[i].usBleConidx = usConnectionHandle;
            app_global_get_app_state()->BleConnectInfo[i].bIsUpdateConnectParameter = false;
        }
    }
}


/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  app_remove_ble_connect
* Description    :  删除一个BLE连接
* Input          :  uint16_t usConnectionHandle
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_remove_ble_connect(uint16_t usConnectionHandle)
{
    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true && app_global_get_app_state()->BleConnectInfo[i].usBleConidx == usConnectionHandle)
        {
            // 初始化结构体
            app_ble_connect_info_init(&(app_global_get_app_state()->BleConnectInfo[i]));
        }
    }
}

/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  app_have_a_active_ble_connect
* Description    :  当前是否有一个活跃的BLE连接
* Input          :  void
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool app_have_a_active_ble_connect(void)
{
    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true)return true;
    }
    return false;
}

/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  app_event_ble_connected_callback
* Description    :  APP事件_BLE已连接回调
* Input          :  uint16_t usConnectionHandle
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_event_ble_connected_callback(uint16_t usConnectionHandle)
{
    // 设置当前全局BLE连接状态为已连接
    app_global_get_app_state()->bBleConnected = true;

    // 添加一个新连接
    app_add_new_ble_connect(usConnectionHandle);

    // 启动连接参数更新定时器
    ble_update_connect_param_start(usConnectionHandle);

}

/*******************************************************************************
*                           陈苏阳@2023-10-25
* Function Name  :  app_event_ble_param_updated_callback
* Description    :  APP事件_BLE连接参数已更新回调
* Input          :  uint16_t usConnectionHandle
* Input          :  uint16_t usConnectInterval
* Input          :  uint16_t usConnectLatency
* Input          :  uint16_t usConnectTimeout
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_event_ble_param_updated_callback(uint16_t usConnectionHandle, uint16_t usConnectInterval, uint16_t usConnectLatency, uint16_t usConnectTimeout)
{
    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        // 找到对应的连接信息,并记录当前的连接参数
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true && app_global_get_app_state()->BleConnectInfo[i].usBleConidx == usConnectionHandle)
        {
            app_global_get_app_state()->BleConnectInfo[i].usConnectInterval = usConnectInterval;
            app_global_get_app_state()->BleConnectInfo[i].usConnectLatency = usConnectLatency;
            app_global_get_app_state()->BleConnectInfo[i].usConnectTimeout = usConnectTimeout;
        }
    }
}




/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  app_event_ble_disconnect_callback
* Description    :  APP事件_BLE已断开回调
* Input          :  uint16_t usConnectionHandle
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_event_ble_disconnect_callback(uint16_t usConnectionHandle)
{
    // 停止连接参数更新
    ble_update_connect_param_stop(usConnectionHandle);

    // 删除连接
    app_remove_ble_connect(usConnectionHandle);

    // 历史数据发送标志位取消 todo:这边会有冲突问题,待完善
    app_global_get_app_state()->bRecordSendFlag = 0;

    // 关闭相关定时器  todo:这边会有冲突问题,待完善
    //ke_timer_clear(APP_TIME2_RECORD_SEND, TASK_APP);
    ble_update_connect_param_all_stop();

    if (app_have_a_active_ble_connect() == false)
    {
        // 设置当前全局BLE连接状态为未连接
        app_global_get_app_state()->bBleConnected = false;
    }
}

/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  app_init
* Description    :  初始化APP层
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_init(void)
{
    // 清空app状态
    app_state_t* pAppState = app_global_get_app_state();
    memset(pAppState, 0x00, sizeof(app_state_t));

    // 初始化BLE连接信息列表
    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        // 初始化结构体
        app_ble_connect_info_init(&(app_global_get_app_state()->BleConnectInfo[i]));
    }

    // 临时设置SN
    ble_adv_set_sn("JN-ABC0000\0");

    // 初始化血糖算法
    //simpleGlucoInit();

    // 初始化启动时间
    //cgms_sst_init();

    // 设置CGM状态为由于MCU复位导致的停止
    app_global_get_app_state()->status = CGM_MEASUREMENT_SENSOR_STATUS_SESSION_M3RESET_STOPPED;

    // 设置血糖测量间隔初始值为10S
    //app_glucose_meas_set_glucose_meas_interval(10); //10s

    // 初始化历史数据存储部分
    //cgms_db_init();
}

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




