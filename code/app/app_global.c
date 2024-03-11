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
#if !defined(LOG_TAG)
#define LOG_TAG                   "APP_GLOBAL"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_WARN

#include "app_global.h"
#include "app.h"
#include "ble_update_connect_param.h"
#include "app_glucose_meas.h"
#include "string.h"
#include "ble_adv.h"
#include <elog.h>
#include "sl_bt_api.h"
#include "cgms_prm.h"
#include "ble_customss.h"
/* Private variables ---------------------------------------------------------*/
app_state_t g_app_state;
event_info_t g_EventInfoArray[APP_EVENT_MAX_NUM];
        
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
        p->ulConenctedTimeCnt = 0;
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
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true && app_global_get_app_state()->BleConnectInfo[i].usBleConidx == usConnectionHandle)
        {
            memset(&(app_global_get_app_state()->BleConnectInfo[i]), 0x00, sizeof(app_global_get_app_state()->BleConnectInfo[i]));
            app_global_get_app_state()->BleConnectInfo[i].bIsConnected = true;
            app_global_get_app_state()->BleConnectInfo[i].usBleConidx = usConnectionHandle;
            app_global_get_app_state()->BleConnectInfo[i].bIsUpdateConnectParameter = false;
            app_global_get_app_state()->BleConnectInfo[i].ulConenctedTimeCnt = 0;
            return;
        }
    }

    // 找到一个未连接的位置,添加信息
    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == false)
        {
            memset(&(app_global_get_app_state()->BleConnectInfo[i]),0x00,sizeof(app_global_get_app_state()->BleConnectInfo[i]));
            app_global_get_app_state()->BleConnectInfo[i].bIsConnected = true;
            app_global_get_app_state()->BleConnectInfo[i].usBleConidx = usConnectionHandle;
            app_global_get_app_state()->BleConnectInfo[i].bIsUpdateConnectParameter = false;
            app_global_get_app_state()->BleConnectInfo[i].ulConenctedTimeCnt = 0;
            break;
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
    log_i("app_event_ble_connected_callback:%d", usConnectionHandle);

    app_global_get_app_state()->bSentRacpSuccess = true;

    app_global_get_app_state()->bSentSocpSuccess = true;

    // 添加一个新连接
    app_add_new_ble_connect(usConnectionHandle);

    // 启动连接参数更新定时器
    ble_update_connect_param_start();

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
    log_i("app_event_ble_param_updated_callback:%d,%d,%d,%d", usConnectionHandle, usConnectInterval, usConnectLatency, usConnectTimeout);

    for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
    {
        // 找到对应的连接信息,并记录当前的连接参数
        if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true && app_global_get_app_state()->BleConnectInfo[i].usBleConidx == usConnectionHandle)
        {
            app_global_get_app_state()->BleConnectInfo[i].usConnectInterval = usConnectInterval;
            app_global_get_app_state()->BleConnectInfo[i].usConnectLatency = usConnectLatency;
            app_global_get_app_state()->BleConnectInfo[i].usConnectTimeout = usConnectTimeout;

            // 判断当前连接参数是否符合要求
            if (ble_update_connect_param_is_pass(usConnectionHandle))
            {
                // 更新标志位
                app_global_get_app_state()->BleConnectInfo[i].bIsUpdateConnectParameter = true;
            }
            else
            {
                // 如果不符合要求,启动连接参数更新定时器
                ble_update_connect_param_start();
            }
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
    log_i("app_event_ble_disconnect_callback:%d", usConnectionHandle);

    // 删除连接
    app_remove_ble_connect(usConnectionHandle);

    // 历史数据发送标志位取消 todo:这边会有冲突问题,待完善
    app_global_get_app_state()->bRecordSendFlag = false;

    // 清除密码效验成功标志
    //app_global_get_app_state()->bCgmsPwdVerifyOk = false;

    // 关闭发送历史数据定时器
    app_glucose_meas_record_send_stop();

    ble_update_connect_param_stop();
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

    // 初始化事件处理
    event_init();

    // 添加更新ble连接参数事件
    event_add(MAIN_LOOP_EVENT_BLE_UPDATE_CONNECT_PARAMETERS, ble_update_connect_param_handle);

    // 初始化血糖算法
    //simpleGlucoInit();

    // 初始化启动时间
    cgms_sst_init();

    // 设置CGM状态为CGM结束
    app_global_get_app_state()->status = CGM_MEASUREMENT_SENSOR_STATUS_SESSION_STOPPED;

    // 更新CGM状态Char内容
    att_get_cgm_status()->ucRunStatus = app_global_get_app_state()->status;

    // 初始化历史数据存储部分
    cgms_db_init();

    // 应用层血糖测量初始化
    app_glucose_meas_init();
}


/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  event_init
* Description    :  初始化事件处理
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void event_init(void)
{
    // 清空数组
    for (uint32_t i = 0; i < APP_EVENT_MAX_NUM; i++)
    {
        memset(&g_EventInfoArray[i], 0x00, sizeof(event_info_t));
    }
}

/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  event_push
* Description    :  推送事件
* Input          :  uint32_t uiEventId
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t event_push(uint32_t uiEventId)
{
    // 发送事件
    if (sl_bt_external_signal(uiEventId) == SL_STATUS_OK)return 1;
    return 0;
}

/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  event_add
* Description    :  添加事件
* Input          :  uint32_t uiEventId
* Input          :  event_callback_t CallBack
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t event_add(uint32_t uiEventId, event_callback_t CallBack)
{
    // 先判断数组中是否已经有本事件
    for (uint32_t i = 0; i < APP_EVENT_MAX_NUM; i++)
    {
        if (g_EventInfoArray[i].uiEventId == (0x01 << uiEventId))
        {
            g_EventInfoArray[i].CallBack = CallBack;
            return 1;
        }
    }

    // 寻找空闲位置添加事件信息
    for (uint32_t i = 0; i < APP_EVENT_MAX_NUM; i++)
    {
        if (g_EventInfoArray[i].uiEventId == 0 && g_EventInfoArray[i].CallBack == NULL)
        {
            g_EventInfoArray[i].uiEventId = (0x01 << uiEventId);
            g_EventInfoArray[i].CallBack = CallBack;
            return 1;
        }
    }
    return 0;
}

/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  event_handler
* Description    :  处理事件
* Input          :  uint32_t uiEventId
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t event_handler(uint32_t uiEventId)
{
    for (uint32_t i = 0; i < APP_EVENT_MAX_NUM; i++)
    {
        if (g_EventInfoArray[i].uiEventId == (0x01 << uiEventId) && g_EventInfoArray[i].CallBack != NULL)
        {
            log_d("event_handler:%d", uiEventId);
            g_EventInfoArray[i].CallBack();
            return 1;
        }
    }
    log_w("event_handler: unknown evnet id");
    return 0;
}



/*******************************************************************************
*                           陈苏阳@2023-11-15
* Function Name  :  rtc_get_curr_time
* Description    :  获取当前时间(单位ms)
* Input          :  void
* Output         :  None
* Return         :  uint32_t
*******************************************************************************/
uint32_t rtc_get_curr_time(void)
{
    uint64_t ulms;
    sl_sleeptimer_tick64_to_ms(sl_sleeptimer_get_tick_count64(), &ulms);
    return (uint32_t)ulms;
}


/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




