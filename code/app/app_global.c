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
#define LOG_LVL                    ELOG_LVL_INFO

#include "app_battery.h"
#include "app_global.h"
#include "app.h"
#include "ble_update_connect_param.h"
#include "app_glucose_meas.h"
#include "string.h"
#include "ble_adv.h"
#include <elog.h>
#include "sl_bt_api.h"
#include "cgms_prm.h"
#include "cgms_db.h"
#include "cgms_db_port.h"
#include "ble_customss.h"
#include "cgms_socp.h"
#include "afe.h"
#include "btl_interface.h"
#include "gatt_db.h"
#include "cgms_debug_db.h"
#include "bms003_bist_if.h"
#include "em_emu.h"
#include "utility.h"
#include "cm_backtrace.h"
/* Private variables ---------------------------------------------------------*/
app_state_t g_app_state;
event_info_t g_EventInfoArray[APP_EVENT_MAX_NUM];
sl_sleeptimer_timer_handle_t g_OtaDelayParamTimer;
char g_cBleMacString[13];
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
    switch (app_global_get_app_state()->Status)
    {
    case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_RUNNING:
    case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_SENSOR_ABNORMAL:
    case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_SENSOR_ERROR:
    case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_INEFFECTIVE_IMPLANTATION:
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
    memset(&(app_global_get_app_state()->BleConnectInfo), 0x00, sizeof(app_global_get_app_state()->BleConnectInfo));
    app_global_get_app_state()->BleConnectInfo.bIsConnected = true;
    app_global_get_app_state()->BleConnectInfo.usBleConidx = usConnectionHandle;
    app_global_get_app_state()->BleConnectInfo.bIsUpdateConnectParameter = false;
    app_global_get_app_state()->BleConnectInfo.ulConenctedTimeCnt = 0;
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
    // 初始化结构体
    app_ble_connect_info_init(&(app_global_get_app_state()->BleConnectInfo));
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
    if (app_global_get_app_state()->BleConnectInfo.bIsConnected == true)return true;
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

    // 找到对应的连接信息,并记录当前的连接参数
    if (app_global_get_app_state()->BleConnectInfo.bIsConnected == true && app_global_get_app_state()->BleConnectInfo.usBleConidx == usConnectionHandle)
    {
        app_global_get_app_state()->BleConnectInfo.usConnectInterval = usConnectInterval;
        app_global_get_app_state()->BleConnectInfo.usConnectLatency = usConnectLatency;
        app_global_get_app_state()->BleConnectInfo.usConnectTimeout = usConnectTimeout;

        // 判断当前连接参数是否符合要求
        if (ble_update_connect_param_is_pass(usConnectionHandle))
        {
            // 更新标志位
            app_global_get_app_state()->BleConnectInfo.bIsUpdateConnectParameter = true;
        }
        else
        {
            // 如果不符合要求,启动连接参数更新定时器
            ble_update_connect_param_start();
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

    // 历史数据发送标志位取消
    app_global_get_app_state()->bRecordSendFlag = false;

    // 清除密码效验成功标志
    app_global_get_app_state()->bCgmsPwdVerifyOk = false;

    // 清除工厂模式标志位
    app_global_get_app_state()->bProduction = false;

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
    extern uint32_t g_uiRstCause;
    log_i("sys init  ver:%s", SOFT_VER);
    log_i("ResetCause:0x%x", g_uiRstCause);
    print_reset_cause(g_uiRstCause);
    cm_backtrace_init("P3A", "0.0.1", SOFT_VER);

    // 初始化bms003的校准
    bms003_bist_init();

    log_i("bms003 bist done");

    // SPI接口初始化
    sl_spidrv_init_instances();

    // 初始化历史数据的flash接口
    cgms_db_flash_init();

    // 打印debug信息
    cgms_debug_db_print();

    // 清空现有的键值对
    cgms_debug_clear_all_kv();

    // 参数存储上电初始化
    cgms_prm_db_power_on_init();

    // 修改设备信息服务中的软件版本号
    sl_bt_gatt_server_write_attribute_value(gattdb_software_revision_string, 0, 5, (uint8_t*)SOFT_VER);

#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
    bd_addr CurAddr;
    uint8_t AddressType;
    // 获取本发射器的Mac地址
    sl_status_t Status = sl_bt_system_get_identity_address(&CurAddr, &AddressType);
    if (Status == SL_STATUS_OK)
    {
        snprintf(g_cBleMacString,13,"%02X%02X%02X%02X%02X%02X", CurAddr.addr[5], CurAddr.addr[4], CurAddr.addr[3], CurAddr.addr[2], CurAddr.addr[1], CurAddr.addr[0]);

        // 修改设备信息服务中的Ble Mac地址
        sl_bt_gatt_server_write_attribute_value(gattdb_ble_mac_string, 0, 12, (uint8_t*)g_cBleMacString);
    }
#endif

    // 清空app状态
    app_state_t* pAppState = app_global_get_app_state();
    memset(pAppState, 0x00, sizeof(app_state_t));

    // 初始化BLE连接信息
    app_ble_connect_info_init(&(app_global_get_app_state()->BleConnectInfo));

    // 初始化事件处理
    event_init();
    // 添加事件
    event_add(MAIN_LOOP_EVENT_BLE_UPDATE_CONNECT_PARAMETERS, ble_update_connect_param_handle);
    event_add(MAIN_LOOP_EVENT_SOCP_START_SESSION_EVENT, cgms_socp_start_session_event_callback);
    event_add(MAIN_LOOP_EVENT_SOCP_STOP_SESSION_EVENT, cgms_socp_stop_session_event_callback);

    // 初始化启动时间
    cgms_sst_init();

    // 打印用户使用数据
    cgms_prm_db_print_user_usage_data(&g_UserUsageData);

#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)

    att_get_feature()->ucWorkTime = 14;                         // 工作时间,如3天,7天,14天,用于计算结束时间,默认0x0E
    att_get_feature()->ucSampleTime = 3;                       // 数据发送间隔3分钟或5分钟,默认0x03
    if (g_UserUsageData.ucDataValidFlag == 0x01 && g_UserUsageData.LastCgmState == CGM_MEASUREMENT_SENSOR_STATUS_SESSION_RUNNING)
    {
        // 还原启动方式
        att_get_feature()->ucStartBy = g_UserUsageData.ucLastStartBy;
        att_get_feature()->ucStartByVersion[0] = g_UserUsageData.ucLastStartByVersion[0];
        att_get_feature()->ucStartByVersion[1] = g_UserUsageData.ucLastStartByVersion[1];
        att_get_feature()->ucStartByVersion[2] = g_UserUsageData.ucLastStartByVersion[2];

        // 设置密码
        att_get_feature()->ucPasswordExist = 0x01;
        cgms_socp_set_ble_protocol_password(g_UserUsageData.usLastPassword);
        // 设置CGM状态为CGM意外停止
        if ((g_uiRstCause & EMU_RSTCAUSE_WDOG0) || (g_uiRstCause & EMU_RSTCAUSE_LOCKUP) || (g_uiRstCause & EMU_RSTCAUSE_SYSREQ))
        {
            app_global_get_app_state()->Status = CGM_MEASUREMENT_SENSOR_STATUS_UNEXPECTED_STOP1;
        }
        else
        {
            app_global_get_app_state()->Status = CGM_MEASUREMENT_SENSOR_STATUS_UNEXPECTED_STOP2;
        }

        // 还原工厂校准码
        att_get_cgm_status()->usFactoryCode = (uint16_t)(g_UserUsageData.fUseSensorK * 1000);

        // 更新start time char中的启动时间
        att_get_start_time()->uiStartTime = g_UserUsageData.uiLastCgmSessionStartTime;
        att_get_start_time()->ucTimeZone = g_UserUsageData.ucLastTimeZone;


        // 更新Start Time和Feature char的CRC
        att_update_start_time_char_data_crc();
        att_update_feature_char_data_crc();
    }
    else
    {
        // 更新Feature char的CRC
        att_update_feature_char_data_crc();
        // 设置CGM状态为CGM没有运行
        app_global_get_app_state()->Status = CGM_MEASUREMENT_SENSOR_STATUS_SESSION_STOPPED;
    }
#else
    // 设置CGM状态为CGM结束
    app_global_get_app_state()->Status = CGM_MEASUREMENT_SENSOR_STATUS_SESSION_M3RESET_STOPPED;
#endif

    // 更新CGM状态Char内容
    att_get_cgm_status()->ucRunStatus = app_global_get_app_state()->Status;
    att_update_cgm_status_char_data();

    // 初始化历史数据存储部分
    cgms_db_init();

    // 应用层血糖测量初始化
    app_glucose_meas_init();

    // 初始化AFE
    afe_init();
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
* Input          :  uint32_t uiArg
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t event_push(uint32_t uiEventId, uint32_t uiArg)
{
    for (uint32_t i = 0; i < APP_EVENT_MAX_NUM; i++)
    {
        if (g_EventInfoArray[i].uiEventId == (uint32_t)(0x01 << uiEventId))
        {
            g_EventInfoArray[i].uiArg = uiArg;
            break;
        }
    }
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
        if (g_EventInfoArray[i].uiEventId == (uint32_t)(0x01 << uiEventId))
        {
            g_EventInfoArray[i].uiArg = (void *)NULL;
            g_EventInfoArray[i].CallBack = CallBack;
            log_d("add event id:%d", uiEventId);
            return 1;
        }
    }

    // 寻找空闲位置添加事件信息
    for (uint32_t i = 0; i < APP_EVENT_MAX_NUM; i++)
    {
        if (g_EventInfoArray[i].uiEventId == 0 && g_EventInfoArray[i].CallBack == NULL)
        {
            g_EventInfoArray[i].uiArg = (void *)NULL;
            g_EventInfoArray[i].uiEventId = (0x01 << uiEventId);
            g_EventInfoArray[i].CallBack = CallBack;
            log_d("add event id:%d", uiEventId);
            return 1;
        }
    }
    log_e("add event fail,id:%d", uiEventId);
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
        if (g_EventInfoArray[i].uiEventId == (uint32_t)(0x01 << uiEventId) && g_EventInfoArray[i].CallBack != NULL)
        {
            log_d("event_handler:%d", uiEventId);
            g_EventInfoArray[i].CallBack(g_EventInfoArray[i].uiArg);
            return 1;
        }
    }
    log_w("event_handler: unknown evnet id(%d)", uiEventId);
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


/*******************************************************************************
*                           陈苏阳@2024-03-26
* Function Name  :  app_global_ota_delay_param_timer_callback
* Description    :  OTA延时回调
* Input          :  sl_sleeptimer_timer_handle_t * handle
* Input          :  void * data
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_global_ota_delay_param_timer_callback(__attribute__((unused))  sl_sleeptimer_timer_handle_t* handle,__attribute__((unused))  void* data)
{
    bootloader_rebootAndInstall();
}

/*******************************************************************************
*                           陈苏阳@2024-03-26
* Function Name  :  app_global_ota_start
* Description    :  触发OTA
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_global_ota_start(void)
{
    // 触发连接参数更新定时器
    sl_sleeptimer_restart_timer(&g_OtaDelayParamTimer, sl_sleeptimer_ms_to_tick(1000), app_global_ota_delay_param_timer_callback, NULL, 0, 0);
}
/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




