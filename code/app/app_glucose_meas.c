/******************** (C) COPYRIGHT 2022 陈苏阳 ********************************
* File Name          :  app_glucose_meas.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  22/12/2022
* Description        :
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#if !defined(LOG_TAG)
#define LOG_TAG                   "APP_GLUCOSE_MEAS"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_DEBUG


#include <string.h>
#include <stdio.h>
#include "cgms_sst.h"
#include "app_util.h"
#include "cgms_socp.h"
#include "cgms_racp.h"
#include "cgms_db.h"
#include "cgms_meas.h"
#include "app_glucose_meas.h"
#include "cgms_crc.h"
#include "app_global.h"
#include "app_battery.h"
#include "ble_customss.h"
#include "afe.h"
#include "math.h"
#include <elog.h>
#include "gatt_db.h"
#include "em_wdog.h"
#include "simplegluco.h"
#include "cgms_error_fault.h"
#include "cgms_timer.h"
#include "cur_filter.h"
#include "matrix.h"
#include "polyfit.h"
/* Private variables ---------------------------------------------------------*/

static volatile app_glucose_meas_type_t g_AppGlucoseMeasType = APP_GLUCOSE_MEAS_TYPE_USER_MEAS;  // 血糖测量类型
static volatile uint8_t g_ucGlucoseMeasTimeCnt = 0;                             // 血糖测量与转换定时器时间计数
static volatile uint16_t g_usGlucoseRecordsCurrentOffset = 0;                   // 当前血糖记录索引(从0开始)
static volatile uint16_t g_usGlucoseElectricCurrent = 0;                        // 测量计算出来的血糖浓度质量(在这里实际用于存储电流值,单位:0.01nA)
static volatile float g_fGlucoseConcentration = 0.0f;                           // 测量计算出来的实时血糖浓度(单位:mmol/L)
static volatile uint8_t g_ucAvgElectricCurrentCalTempArrayCnt = 0;              // 当前用于计算平均电流的临时数据数量
static volatile float g_fAvgElectricCurrentCalTempArray[APP_GLUCOSE_MEAS_AVG_ELECTRIC_CURRENT_CAL_TEMP_ARRAY_SIZE];                   // 用于计算平均电流的临时数据数组
static volatile  uint8_t g_ucAppBatteryInitMeasDoneFlag = 0;                     // 电量初始转换完成标志位
sl_sleeptimer_timer_handle_t g_AppGlucoseMeasTimer;                    // 应用层血糖测量定时器
sl_sleeptimer_timer_handle_t g_AppGlucoseMeasRecordSendTimer;          // 应用层血糖测量记录发送定时器
sl_sleeptimer_timer_handle_t g_AppBatteryMeasTimer;                    // 应用层电量测量定时器

#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
static uint32_t g_uiCgmWorkTimeCnt = 0;                                // CGM运行时间(单位秒)
#endif
/* Private function prototypes -----------------------------------------------*/
void app_battery_meas_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data);
/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           陈苏阳@2023-10-20
* Function Name  :  app_glucose_get_records_current_offset
* Description    :  获取当前血糖记录索引
* Input          :  void
* Output         :  None
* Return         :  uint16_t
*******************************************************************************/
uint16_t app_glucose_get_records_current_offset(void)
{
    return g_usGlucoseRecordsCurrentOffset;
}

/*******************************************************************************
*                           陈苏阳@2023-03-23
* Function Name  :  app_glucose_avg_voltage_cal_add_data
* Description    :  平均电流计算_添加新的电流数据
* Input          :  double dElectricCurrent
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_avg_electric_current_cal_add_data(double dElectricCurrent)
{
    // 如果数据数量没有超限,则将新数据添加进数组
    if (g_ucAvgElectricCurrentCalTempArrayCnt < APP_GLUCOSE_MEAS_AVG_ELECTRIC_CURRENT_CAL_TEMP_ARRAY_SIZE)
    {
        g_fAvgElectricCurrentCalTempArray[g_ucAvgElectricCurrentCalTempArrayCnt] = (float)dElectricCurrent;
        g_ucAvgElectricCurrentCalTempArrayCnt++;
    }
}

/*******************************************************************************
*                           陈苏阳@2023-06-28
* Function Name  :  app_glucose_avg_electric_current_get_electric_current_array
* Description    :  获取电流数组
* Input          :  void
* Output         :  None
* Return         :  float*
*******************************************************************************/
float* app_glucose_avg_electric_current_get_electric_current_array(void)
{
    // 如果数组中的数据不足20个,则补齐
    if (g_ucAvgElectricCurrentCalTempArrayCnt < 20)
    {
        for (uint8_t i = g_ucAvgElectricCurrentCalTempArrayCnt; i < 20; i++)
        {
            if (i > 0)
            {
                g_fAvgElectricCurrentCalTempArray[i] = g_fAvgElectricCurrentCalTempArray[i - 1];
            }
        }
    }
    return &g_fAvgElectricCurrentCalTempArray[0];
}


/*******************************************************************************
*                           陈苏阳@2023-03-23
* Function Name  :  app_glucose_avg_electric_current_cal_init
* Description    :  平均电流计算_初始化(清除之前的电流数据)
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_avg_electric_current_cal_init(void)
{
    g_ucAvgElectricCurrentCalTempArrayCnt = 0;
    for (uint8_t i = 0; i < APP_GLUCOSE_MEAS_AVG_ELECTRIC_CURRENT_CAL_TEMP_ARRAY_SIZE; i++)
    {
        g_fAvgElectricCurrentCalTempArray[i] = 0.0f;
    }
}

/*******************************************************************************
*                           陈苏阳@2023-03-23
* Function Name  :  app_glucose_avg_electric_current_cal_get
* Description    :  平均电流计算_根据目前已有数据,获取当前平均电流
* Input          :  void
* Output         :  None
* Return         :  double
*******************************************************************************/
double app_glucose_avg_electric_current_cal_get(void)
{
    double dAvg = 0.0;
    // 累加临时数据
    for (uint8_t i = 0; i < g_ucAvgElectricCurrentCalTempArrayCnt; i++)
    {
        dAvg += g_fAvgElectricCurrentCalTempArray[i];
    }
    // 如果临时数据和,与临时数据数量都不为0,则计算平均
    if (fabs(dAvg) > 1e-15 && g_ucAvgElectricCurrentCalTempArrayCnt)
    {
        return dAvg / (double)g_ucAvgElectricCurrentCalTempArrayCnt;
    }
    else
    {
        return 0.0;
    }
}



/*******************************************************************************
*                           陈苏阳@2022-12-22
* Function Name  :  app_glucose_meas_get_glucose_quality
* Description    :  获取测量计算出来的血糖浓度质量(在这里实际用于存储电流值,单位:0.01nA)
* Input          :  void
* Output         :  None
* Return         :  uint16_t
*******************************************************************************/
uint16_t app_glucose_meas_get_glucose_quality(void)
{
    return g_usGlucoseElectricCurrent;
}


/*******************************************************************************
*                           陈苏阳@2024-10-21
* Function Name  :  app_glucose_cal_abnormal_state
* Description    :  计算异常状态
* Input          :  cgm_measurement_sensor_state_t RawState
* Input          :  float fCurrent
* Input          :  float fCv
* Output         :  None
* Return         :  cgm_measurement_sensor_state_t
*******************************************************************************/
cgm_measurement_sensor_state_t app_glucose_cal_abnormal_state(cgm_measurement_sensor_state_t RawState, float fCurrent, float fCv)
{
    // 如果算法报传感器异常
    if (RawState == CGM_MEASUREMENT_SENSOR_STATUS_SESSION_SENSOR_ABNORMAL)
    {
        // 如果电流小于等于0.1nA
        if (fCurrent <= 0.1f)
        {
            // 报小电流异常
            return CGM_MEASUREMENT_SENSOR_STATUS_SESSION_CURRENT_TOO_LOW;
        }
        // 如果电流大于等于50nA
        else if (fCurrent >= 50)
        {
            // 报大电流异常
            return CGM_MEASUREMENT_SENSOR_STATUS_SESSION_CURRENT_TOO_HIGH;
        }

        // 如果CV大于0.2
        if (fCv > 0.2)
        {
            // 报CV异常
            return CGM_MEASUREMENT_SENSOR_STATUS_SESSION_CV_ERR;
        }

        // 否则报原始的0x22传感器异常
        return RawState;
    }
    else
    {
        return RawState;
    }
}

/*******************************************************************************
*                           陈苏阳@2022-12-15
* Function Name  :  app_glucose_handle
* Description    :  血糖计算(3分钟一次)
* Input          :  float fElectricCurrent
* Output         :  None
* Return         :  void
*******************************************************************************/
static void app_glucose_handle(void)
{
    // 如果当前记录已满,则直接退出
    if (g_usGlucoseRecordsCurrentOffset >= CGMS_DB_MAX_RECORDS)return;

    float* pAvgElectricCurrentCalTempArray = app_glucose_avg_electric_current_get_electric_current_array();
    log_d("ElectricCurrent:");
    for (uint8_t i = 0; i < 20; i++)
    {
        log_d("index:%d,%f", i, pAvgElectricCurrentCalTempArray[i]);
    }

    sfCurrI0 = cur_filter(pAvgElectricCurrentCalTempArray, g_usGlucoseRecordsCurrentOffset);

    simpleGlucoCalc(&g_fGlucoseConcentration, g_usGlucoseRecordsCurrentOffset);
    log_i("simpleGlucoCalc(%f)  sfCurrI0:%f", g_fGlucoseConcentration, sfCurrI0);
    cgms_meas_t rec;
    memset(&rec, 0, sizeof(cgms_meas_t));
    rec.usGlucose = (uint16_t)(g_fGlucoseConcentration * 10.0f);
    rec.usCurrent = (uint16_t)(sfCurrI0 * 100.0f);
    g_usGlucoseElectricCurrent = rec.usCurrent;
    rec.usQuality = 0x00;
    float fCv = cgms_i_cv(sfCurrI0, g_usGlucoseRecordsCurrentOffset);
    uint8_t ucState;
    cgms_error_fault_cal(g_usGlucoseRecordsCurrentOffset, g_fGlucoseConcentration, sfCurrI0, &ucState, fCv); // 计算异常逻辑
    uint8_t ucTrend = cgms_cal_trend(g_fGlucoseConcentration, g_usGlucoseRecordsCurrentOffset);// 计算趋势
    rec.ucCV = (uint8_t)(fCv*100.0f);
    rec.ucTrend = ucTrend;

    // 计算传感器异常状态
    ucState = app_glucose_cal_abnormal_state(ucState, sfCurrI0, fCv);

    rec.ucState = ucState;
    att_get_cgm_status()->ucRunStatus = ucState;


    app_global_get_app_state()->CgmTrend = ucTrend;
    rec.usOffset = g_usGlucoseRecordsCurrentOffset;
    rec.usHistoryFlag = CGMS_MEAS_HISTORY_FLAG_REAL;
    log_i("cgms_meas_create  %d,%d,%d", rec.usOffset, rec.usGlucose, rec.usCurrent);

    // 存储历史记录
    ret_code_t ErrCode = cgms_db_record_add(&rec);
    if (ErrCode != RET_CODE_SUCCESS)
    {
        log_e("cgms_db_record_add fail:%d", ErrCode);
    }
    // 通过BLE发送本次的测量记录

    if ((ble_meas_notify_is_enable()) && app_have_a_active_ble_connect())
    {
        app_global_get_app_state()->bSentMeasSuccess = false;

        for (uint8_t i = 0; i < BLE_MAX_CONNECTED_NUM; i++)
        {
            // 如果发现一个连接
            if (app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true)
            {
                ble_event_info_t BleEventInfo;
                BleEventInfo.ucConidx = app_global_get_app_state()->BleConnectInfo[i].usBleConidx;
                BleEventInfo.usHandle = gattdb_cgm_measurement;
                ErrCode = cgms_meas_send(BleEventInfo, rec);
                if (ErrCode != RET_CODE_SUCCESS)
                {
                    log_e("cgms_meas_send fail:%d", ErrCode);
                }
            }
        }
    }
    else
    {
        if (!ble_meas_notify_is_enable())
        {
            log_w("ble_meas_notify_is_disable");
        }
        if (!app_have_a_active_ble_connect())
        {
            log_w("ble is disconnecded");
        }
    }

    app_global_get_app_state()->usTimeOffset = g_usGlucoseRecordsCurrentOffset;

    // 更新CGM状态char的offset
    att_get_cgm_status()->usNumberOfReadings = app_global_get_app_state()->usTimeOffset + 1;
#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
	// 更新CGM状态char的内容
	att_update_cgm_status_char_data_crc();
#else
    // 更新CGM状态char的内容
    att_update_cgm_status_char_data();
#endif
    // 当前记录索引++
    g_usGlucoseRecordsCurrentOffset++;
}


/*******************************************************************************
*                           陈苏阳@2023-08-14
* Function Name  :  app_glucose_meas_battery_sub_handler
* Description    :  血糖测量中的电量测量子处理函数
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_battery_sub_handler(void)
{
    // 触发电量采集定时器处理函数
    app_battery_timer_handler(60);
}

/*******************************************************************************
*                           陈苏阳@2023-08-14
* Function Name  :  app_glucose_meas_stop_session_handler
* Description    :  血糖测量中的停止CGM子处理函数
* Input          :  uint8_t ucStopReason
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_stop_session_handler(uint8_t ucStopReason)
{
    // AFE停止
    afe_stop();
    // 设置传感器状态为停止,并更新状态
    app_global_get_app_state()->Status = ucStopReason;
    att_get_cgm_status()->ucRunStatus = app_global_get_app_state()->Status;

    // 更新CGM状态char的内容
#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
    att_update_cgm_status_char_data_crc();
#else
    att_update_cgm_status_char_data();
#endif
}



/*******************************************************************************
*                           陈苏阳@2023-08-14
* Function Name  :  app_glucose_meas_afe_meas_handler
* Description    :  血糖测量中的AFE测量子处理函数
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_afe_meas_handler(void)
{
    while (afe_new_data_is_ready())
    {
        double dElectricCurrent;
        if (afe_get_new_data(&dElectricCurrent))
        {
            log_i("app_glucose_avg_electric_current_cal_add_data(%f)", (float)dElectricCurrent);
            // 将新数据添加到临时数据列表,以供后续计算平均值
            app_glucose_avg_electric_current_cal_add_data(dElectricCurrent);
        }
        else
        {
            break;
        }
    }
}


/*******************************************************************************
*                           陈苏阳@2023-08-14
* Function Name  :  app_glucose_meas_glucose_handler
* Description    :  血糖测量中的血糖处理子处理函数(1S一次)
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_glucose_handler(void)
{
    log_d("app_glucose_meas_glucose_handler");
    WDOGn_Feed(WDOG0);
    // 累计时间
    g_ucGlucoseMeasTimeCnt++;

    log_d("g_ucGlucoseMeasTimeCnt:%d\r\n", g_ucGlucoseMeasTimeCnt);
    // 如果当前到了测量间隔时间计数
    if (g_ucGlucoseMeasTimeCnt == APP_GLUCOSE_MEAS_MEAS_INTERVAL)
    {
        log_d("app_glucose_handle:%d\r\n", g_ucAvgElectricCurrentCalTempArrayCnt);

        // 计算当前电流平均值并调用血糖处理函数
        app_glucose_handle();

        // 初始化平均电流计算(清除之前的电流临时数据)
        app_glucose_avg_electric_current_cal_init();

        // 清零测量间隔时间计数
        g_ucGlucoseMeasTimeCnt = 0;
    }
    // 如果到达了猝发采样的开始时间点
    else if (g_ucGlucoseMeasTimeCnt == (APP_GLUCOSE_MEAS_MEAS_INTERVAL - 1))
    {
        // 开始20次猝发采样
        afe_shot(20);
    }

}

/*******************************************************************************
*                           陈苏阳@2023-05-29
* Function Name  :  app_glucose_meas_timer_handler
* Description    :  血糖测量与转换定时处理函数(周期1分钟一次)
* Input          :  uint32_t uiArg
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_handler(uint32_t uiArg)
{

        // 如果当前测量类型为用户测量
        if (g_AppGlucoseMeasType == APP_GLUCOSE_MEAS_TYPE_USER_MEAS)
        {
            // 测量电量
            app_glucose_meas_battery_sub_handler();

            // 如果当前记录已满且BLE的传感器状态属于正在采集的状态
            if (g_usGlucoseRecordsCurrentOffset >= CGMS_DB_MAX_RECORDS && app_global_is_session_runing())
            {
                log_i("app_glucose_meas_stop_session_handler");

                // 停止CGM
                app_glucose_meas_stop_session_handler(CGM_MEASUREMENT_SENSOR_STATUS_SENSION_EXPRIED);

                // 停止本定时器
                app_glucose_meas_stop();
            }

            log_d("app_global_is_session_runing:%d", app_global_is_session_runing() ? 1 : 0);
            // 如果当前已经开始了一次血糖测量周期
            if (app_global_is_session_runing())
            {
            	#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)    	
            	// 增加运行时间
            	g_uiCgmWorkTimeCnt++;
            	log_d("g_uiCgmWorkTimeCnt:%d", g_uiCgmWorkTimeCnt);
            	// 更新运行时间
            	att_get_start_time()->uiRunTime = g_uiCgmWorkTimeCnt;

            	// 更新运行时间CRC
            	att_update_start_time_char_data_crc();	
            	#endif
                // 如果AFE还未开始工作,则启动AFE
                if (afe_is_working() == false)
                {
                    // 开始AFE,使用猝发采样模式
                    afe_start(AFE_RUN_MODE_SHOT);
                }
                else
                {
                    // 测量处理
                    app_glucose_meas_afe_meas_handler();
                }

                // 血糖处理
                app_glucose_meas_glucose_handler();
            }
        }
        // 如果是工厂测量
        else
        {
            // 如果AFE还未开始工作,则启动AFE
            if (afe_is_working() == false)
            {
                // 开始采样,使用连续采样模式
                afe_start(AFE_RUN_MODE_CONTINUOUS);
            }
            else
            {
                // 测量处理
                app_glucose_meas_afe_meas_handler();
            }
        }

}
/*******************************************************************************
*                           陈苏阳@2024-05-09
* Function Name  :  app_glucose_meas_get_factory_meas_electric_current
* Description    :  获取工厂测量电流
* Input          :  uint32_t * pMeasElectricCurrent
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool app_glucose_meas_get_factory_meas_electric_current(uint32_t* pMeasElectricCurrent)
{
    if (pMeasElectricCurrent)
    {
        app_glucose_avg_electric_current_get_electric_current_array();
        uint32_t uiElectricCurrent = (uint32_t)(g_fAvgElectricCurrentCalTempArray[17] * 1000.0f);
        *pMeasElectricCurrent = uiElectricCurrent;
        return true;
    }
    return false;
}
/*******************************************************************************
*                           陈苏阳@2023-11-15
* Function Name  :  app_glucose_meas_timer_callback
* Description    :  应用层血糖测量定时器回调
* Input          :  sl_sleeptimer_timer_handle_t * handle
* Input          :  void * data
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data)
{
    log_d("app_glucose_meas_timer_callback");
    event_push(MAIN_LOOP_EVENT_APP_GLUCOSE_MEAS_1MIN_TIMER, (void*)NULL);
}


/*******************************************************************************
*                           陈苏阳@2022-12-22
* Function Name  :  app_glucose_meas_start
* Description    :  血糖测量开始
* Input          :  app_glucose_meas_type_t GlucoseMeasType
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_start(app_glucose_meas_type_t GlucoseMeasType)
{
    g_AppGlucoseMeasType = GlucoseMeasType;
    log_d("app glucose meas start");
    // 启动一个1分钟的循环定时器
    sl_status_t status = sl_sleeptimer_start_periodic_timer(&g_AppGlucoseMeasTimer, sl_sleeptimer_ms_to_tick(60 * 1000), app_glucose_meas_timer_callback, (void*)NULL, 0, 0);
    if (status != SL_STATUS_OK)
    {
        log_e("sl_sleeptimer_start_timer failed");
    }
    // 初始化平均电流计算
    app_glucose_avg_electric_current_cal_init();
}


/*******************************************************************************
*                           陈苏阳@2022-12-22
* Function Name  :  app_glucose_meas_stop
* Description    :  停止血糖测量
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_stop(void)
{
    log_d("app glucose meas stop");
    sl_sleeptimer_stop_timer(&g_AppGlucoseMeasTimer);
}


/*******************************************************************************
*                           陈苏阳@2023-11-15
* Function Name  :  app_glucose_meas_record_send_timer_callback
* Description    :  应用层血糖测量记录发送定时器回调
* Input          :  sl_sleeptimer_timer_handle_t * handle
* Input          :  void * data
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_record_send_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data)
{
    log_d("app_glucose_meas_record_send_timer_callback");
    event_push(MAIN_LOOP_EVENT_APP_GLUCOSE_MEAS_RECORD_SEND_TIMER, NULL);
}

/*******************************************************************************
*                           陈苏阳@2022-12-22
* Function Name  :  app_glucose_meas_record_send_start
* Description    :  开始发送血糖测量记录
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_record_send_start(void)
{
    log_i("app_glucose_meas_record_send_start");
    // 启动一个100ms的单次定时器
    sl_status_t status = sl_sleeptimer_start_timer(&g_AppGlucoseMeasRecordSendTimer, sl_sleeptimer_ms_to_tick(100), app_glucose_meas_record_send_timer_callback, (void*)NULL, 0, 0);
    if (status != SL_STATUS_OK)
    {
        log_e("sl_sleeptimer_start_timer failed");
    }
    else
    {
        app_global_get_app_state()->bRecordSendFlag = true;
    }
    
}

/*******************************************************************************
*                           陈苏阳@2023-11-21
* Function Name  :  app_glucose_meas_record_send_stop
* Description    :  停止发送血糖测量记录
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_record_send_stop(void)
{
    sl_sleeptimer_stop_timer(&g_AppGlucoseMeasRecordSendTimer);
}


/*******************************************************************************
*                           陈苏阳@2022-12-15
* Function Name  :  app_glucose_meas_record_send_handelr
* Description    :  发送记录定时处理函数
* Input          :  uint32_t uiArg
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_record_send_handelr(uint32_t uiArg)
{
    bool bSendSuccessFlag = true;
    log_i("app_glucose_meas_record_send_handler:%d", app_global_get_app_state()->bRecordSendFlag);

    // 如果没有使能meas char的notify或者没有手机连接上发射器
    if ((!ble_meas_notify_is_enable()) || (!app_have_a_active_ble_connect()))
    {
        // 停止发送历史数据
        app_glucose_meas_record_send_stop();

        // 清空历史数据发送状态信息
        app_global_get_app_state()->RecordOptInfo.usRacpRecordCnt = 0;
        app_global_get_app_state()->RecordOptInfo.usRacpRecordSendCnt = 0;
        app_global_get_app_state()->bRecordSendFlag = false;
        return;
    }

    // 如果发送历史数据标志位置位
    if (app_global_get_app_state()->bRecordSendFlag == true)
    {
        log_i("%d/%d", app_global_get_app_state()->RecordOptInfo.usRacpRecordSendCnt, app_global_get_app_state()->RecordOptInfo.usRacpRecordCnt);

        // 如果还有历史数据没发完
        if (app_global_get_app_state()->RecordOptInfo.usRacpRecordSendCnt < app_global_get_app_state()->RecordOptInfo.usRacpRecordCnt)
        {
            cgms_meas_t HistoryRec;
            // 读取待发送的历史数据
            if (RET_CODE_SUCCESS == cgms_db_record_get(app_global_get_app_state()->RecordOptInfo.usRacpRecordStartIndex, &HistoryRec))
            {
                ble_event_info_t BleEventInfo;
                BleEventInfo = app_global_get_app_state()->RecordOptInfo.BleEventInfo;
                BleEventInfo.usHandle = gattdb_cgm_measurement;

                // 发送历史数据
                if (cgms_meas_send(BleEventInfo, HistoryRec) != RET_CODE_SUCCESS)
                {
                    bSendSuccessFlag = false;
                    log_i("cgms_meas_send err");
                }
            }
            else
            {
                bSendSuccessFlag = false;
                log_i("cgms_db_record_get err");
            }

            // 100ms后继续发送血糖数据记录
            if ((ble_meas_notify_is_enable()) && app_have_a_active_ble_connect())
            {
                // 发送计数累计(只有在发送成功时才累计)
                if (bSendSuccessFlag)
                {
                    app_global_get_app_state()->RecordOptInfo.usRacpRecordSendCnt++;
                    app_global_get_app_state()->RecordOptInfo.usRacpRecordStartIndex++;
                }
                app_glucose_meas_record_send_start();
            }
            else
            {
                log_w("record send end,%d,%d", ble_meas_notify_is_enable() ? 1 : 0, app_have_a_active_ble_connect() ? 1 : 0);
            }
        }
        else
        {
            log_i("record send done");
            // 发送代表历史数据发送完成的"0xAA"数据包
            cgms_history_special_datapcket_t CgmsHistorySpecialDatapcket;

            // 设置code为发送完成
            CgmsHistorySpecialDatapcket.ucDatapacketCode = CGMS_HISTORY_SPECIAL_DATAPACKET_CODE_SEND_DONE;
            CgmsHistorySpecialDatapcket.ucReserved[0] = 0x00;
            CgmsHistorySpecialDatapcket.ucReserved[1] = 0x00;
            // 计算CRC
            CgmsHistorySpecialDatapcket.usCRC16 = do_crc(&CgmsHistorySpecialDatapcket, sizeof(CgmsHistorySpecialDatapcket) - 2);


            ble_event_info_t BleEventInfo;
            BleEventInfo = app_global_get_app_state()->RecordOptInfo.BleEventInfo;
            BleEventInfo.usHandle = gattdb_cgm_measurement;

            ret_code_t ret = cgms_meas_special_send(BleEventInfo, CgmsHistorySpecialDatapcket);
            // 发送特殊血糖数据
            if (ret != RET_CODE_SUCCESS)
            {
                log_e("cgms_meas_special_send fail:%d", ret);
                bSendSuccessFlag = false;
            }
            else
            {
                // 停止发送历史数据
                app_glucose_meas_record_send_stop();

                app_global_get_app_state()->bRecordSendFlag = false;
            }

            // 如果发送失败
            if (bSendSuccessFlag == false)
            {
                // 下次进入时重试
                app_glucose_meas_record_send_start();
            }
        }
    }
}


/*******************************************************************************
*                           陈苏阳@2023-11-15
* Function Name  :  app_battery_meas_handelr
* Description    :  应用层电量测量定时处理
* Input          :  uint32_t uiArg
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_battery_meas_handelr(uint32_t uiArg)
{
    sl_status_t status;
    // 如果当前已经开始CGM,则关闭本定时器,由CGM定时函数来做电量检测
    if (app_global_is_session_runing())return;
    if (g_ucAppBatteryInitMeasDoneFlag < 2)
    {
        // 触发电量采集定时器处理函数
        app_battery_timer_handler(1);

        g_ucAppBatteryInitMeasDoneFlag++;

        // 启动一个1S的单次定时器
        status = sl_sleeptimer_start_timer(&g_AppBatteryMeasTimer, sl_sleeptimer_ms_to_tick(1000), app_battery_meas_timer_callback, (void*)NULL, 0, 0);
        if (status != SL_STATUS_OK)
        {
            log_e("sl_sleeptimer_start_timer failed");
        }
    }
    else
    {
        // 触发电量采集定时器处理函数
        app_battery_timer_handler(5);
        // 启动一个1分钟的单次定时器
        status = sl_sleeptimer_start_timer(&g_AppBatteryMeasTimer, sl_sleeptimer_ms_to_tick(60*1000), app_battery_meas_timer_callback, (void*)NULL, 0, 0);
        if (status != SL_STATUS_OK)
        {
            log_e("sl_sleeptimer_start_timer failed");
        }

    }
}


/*******************************************************************************
*                           陈苏阳@2023-11-15
* Function Name  :  app_battery_meas_timer_callback
* Description    :  应用层电量测量定时器回调
* Input          :  sl_sleeptimer_timer_handle_t * handle
* Input          :  void * data
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_battery_meas_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data)
{
    log_d("app_battery_meas_timer_callback");
    event_push(MAIN_LOOP_EVENT_APP_BATTERY_MEAS_TIMER, NULL);
}


/*******************************************************************************
*                           陈苏阳@2022-12-22
* Function Name  :  app_glucose_meas_init
* Description    :  应用层血糖测量初始化
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_init(void)
{
    // 添加事件
    event_add(MAIN_LOOP_EVENT_APP_GLUCOSE_MEAS_1MIN_TIMER, app_glucose_meas_handler);
    event_add(MAIN_LOOP_EVENT_APP_GLUCOSE_MEAS_RECORD_SEND_TIMER, app_glucose_meas_record_send_handelr);
    event_add(MAIN_LOOP_EVENT_APP_BATTERY_MEAS_TIMER, app_battery_meas_handelr);

    // 初始化血糖算法
    simpleGlucoInit();

    // 启动一个500ms的单次定时器
    sl_status_t status = sl_sleeptimer_start_timer(&g_AppBatteryMeasTimer, sl_sleeptimer_ms_to_tick(500), app_battery_meas_timer_callback, (void*)NULL, 0, 0);
    if (status != SL_STATUS_OK)
    {
        log_e("sl_sleeptimer_start_timer failed:%d", status);
    }
}


/******************* (C) COPYRIGHT 2022 陈苏阳 **** END OF FILE ****************/




