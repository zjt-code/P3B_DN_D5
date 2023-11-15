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
#define LOG_LVL                    ELOG_LVL_INFO


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
/* Private variables ---------------------------------------------------------*/

static uint8_t g_ucGlucoseMeasInterval = 0;                            // 血糖测量间隔(ADC测量间隔,默认10S)
static uint8_t g_ucGlucoseMeasTimeCnt = 0;                             // 血糖测量与转换定时器时间计数
static uint16_t g_usGlucoseRecordsCurrentOffset = 0;                   // 当前血糖记录索引(从0开始)
static uint16_t g_usGlucoseElectricCurrent = 0;                        // 测量计算出来的血糖浓度质量(在这里实际用于存储电流值,单位:0.01nA)
static float g_fGlucoseConcentration = 0.0f;                           // 测量计算出来的实时血糖浓度(单位:mmol/L)
static uint8_t g_ucAvgElectricCurrentCalTempArrayCnt = 0;              // 当前用于计算平均电流的临时数据数量
static float g_dAvgElectricCurrentCalTempArray[APP_GLUCOSE_MEAS_AVG_ELECTRIC_CURRENT_CAL_TEMP_ARRAY_SIZE];                   // 用于计算平均电流的临时数据数组
static uint16_t g_usAppBatteryTimeDiv = 0;	                            // 电量检测定时任务分频计数
static uint8_t g_ucAppBatteryInitMeasDoneFlag = 0;                     // 电量初始转换完成标志位
static uint32_t g_UiListRtcTime = 0;                                   // 最后一次的RTC时间


sl_sleeptimer_timer_handle_t g_AppGlucoseMeasTimer;                    // 应用层血糖测量定时器
sl_sleeptimer_timer_handle_t g_AppGlucoseMeasRecordSendTimer;          // 应用层血糖测量记录发送定时器
sl_sleeptimer_timer_handle_t g_AppBatteryMeasTimer;                    // 应用层电量测量定时器
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
void app_glucose_avg_electric_current_cal_add_data(float dElectricCurrent)
{
    // 如果数据数量没有超限,则将新数据添加进数组
    if (g_ucAvgElectricCurrentCalTempArrayCnt < APP_GLUCOSE_MEAS_AVG_ELECTRIC_CURRENT_CAL_TEMP_ARRAY_SIZE)
    {
        g_dAvgElectricCurrentCalTempArray[g_ucAvgElectricCurrentCalTempArrayCnt] = dElectricCurrent;
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
    // 如果数组中的数据不足18个,则补齐
    if (g_ucAvgElectricCurrentCalTempArrayCnt < 18)
    {
        for (uint8_t i = g_ucAvgElectricCurrentCalTempArrayCnt; i < 18; i++)
        {
            if (i > 0)
            {
                g_dAvgElectricCurrentCalTempArray[i] = g_dAvgElectricCurrentCalTempArray[i - 1];
            }
        }
    }
    return g_dAvgElectricCurrentCalTempArray;
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
        g_dAvgElectricCurrentCalTempArray[i] = 0.0;
    }
}

/*******************************************************************************
*                           陈苏阳@2023-03-23
* Function Name  :  app_glucose_avg_electric_current_cal_get
* Description    :  平均电流计算_根据目前已有数据,获取当前平均电流
* Input          :  void
* Output         :  None
* Return         :  float
*******************************************************************************/
float app_glucose_avg_electric_current_cal_get(void)
{
	float dAvg = 0.0;
    // 累加临时数据
    for (uint8_t i = 0; i < g_ucAvgElectricCurrentCalTempArrayCnt; i++)
    {
        dAvg += g_dAvgElectricCurrentCalTempArray[i];
    }
    // 如果临时数据和,与临时数据数量都不为0,则计算平均
    if (fabs(dAvg) > 1e-15 && g_ucAvgElectricCurrentCalTempArrayCnt)
    {
        return dAvg / (float)g_ucAvgElectricCurrentCalTempArrayCnt;
    }
    else
    {
        return 0.0;
    }
}


/*******************************************************************************
*                           陈苏阳@2022-12-22
* Function Name  :  app_glucose_meas_set_glucose_meas_interval
* Description    :  设置血糖测量间隔
* Input          :  uint8_t ucGlucoseMeasInterval
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_set_glucose_meas_interval(uint8_t ucGlucoseMeasInterval)
{
    g_ucGlucoseMeasInterval = ucGlucoseMeasInterval;
}


/*******************************************************************************
*                           陈苏阳@2022-12-22
* Function Name  :  app_glucose_meas_get_glucose_meas_interval
* Description    :  获取血糖测量间隔
* Input          :  void
* Output         :  None
* Return         :  uint16_t
*******************************************************************************/
uint16_t app_glucose_meas_get_glucose_meas_interval(void)
{
    return g_ucGlucoseMeasInterval;
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
    log_i("ElectricCurrent:");
    for (uint8_t i = 0; i < 18; i++)
    {
        log_i("index:%d,%f", i, pAvgElectricCurrentCalTempArray[i]);
    }
    
    /*
    // 提供下标
    usSampleCnt = g_usGlucoseRecordsCurrentOffset;

    sfCurrI0 = cur_filter(pAvgElectricCurrentCalTempArray, g_usGlucoseRecordsCurrentOffset);
    g_usGlucoseQuality = (uint16_t)(sfCurrI0);  //0.01nA

    // 计算并输出血糖结果
    simpleGlucoCalc(&g_fGlucoseConcentration);
    */
    g_usGlucoseElectricCurrent = 10;
    g_fGlucoseConcentration = 11.1f;

    cgms_meas_t rec;
    memset(&rec, 0, sizeof(cgms_meas_t));

    rec.usGlucose = (uint16_t)((double)g_fGlucoseConcentration * 100.0);
    rec.usCurrent = (uint16_t)((double)g_usGlucoseElectricCurrent * 100.0);
    rec.usOffset = g_usGlucoseRecordsCurrentOffset;
    log_i("nrf_ble_cgms_meas_create  %d,%d,%d", rec.usOffset, rec.usGlucose, rec.usCurrent);

    // 存储历史记录
    ret_code_t err_code = cgms_db_record_add(&rec);
    // 通过BLE发送本次的测量记录

	if((is_measurement_notify())&&(app_global_get_app_state()->bSentSuccess==true)&&(app_global_get_app_state()->bBleConnected==true))
	{
		app_global_get_app_state()->bSentSuccess=false;

		for(uint8_t i=0;i<BLE_MAX_CONNECTED_NUM;i++)
		{
			// 如果发现一个连接
			if(app_global_get_app_state()->BleConnectInfo[i].bIsConnected == true)
			{
				ble_event_info_t BleEventInfo;
				BleEventInfo.ucConidx = app_global_get_app_state()->BleConnectInfo[i].usBleConidx;
			    err_code = cgms_meas_send(BleEventInfo, rec);
			}
		}
	}
    
    app_global_get_app_state()->time_offset = g_usGlucoseRecordsCurrentOffset;

    // 更新CGM状态char的offset
    att_get_cgm_status()->usNumberOfReadings = app_global_get_app_state()->time_offset + 1;
    att_update_cgm_status_char_data_crc();

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

    if (g_usAppBatteryTimeDiv < 30 * 60)
    {
        g_usAppBatteryTimeDiv++;
    }
    else
    {
        // 触发电量采集定时器处理函数
        app_battery_timer_handler(5);

        g_usAppBatteryTimeDiv = 0;
    }
}

/*******************************************************************************
*                           陈苏阳@2023-08-14
* Function Name  :  app_glucose_meas_stop_session_handler
* Description    :  血糖测量中的停止CGM子处理函数
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_stop_session_handler(void)
{
    // AFE停止
    afe_stop();

    // 设置传感器状态为停止,并更新状态
    app_global_get_app_state()->status = CGM_MEASUREMENT_SENSOR_STATUS_SESSION_STOPPED;
    att_get_cgm_status()->ucRunStatus = CGM_MEASUREMENT_SENSOR_STATUS_SESSION_STOPPED;
    att_update_cgm_status_char_data_crc();
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
            log_i("app_glucose_avg_electric_current_cal_add_data(%f)", dElectricCurrent);
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
    // 如果测量间隔小于最小间隔,则设置为最小间隔
    if (g_ucGlucoseMeasInterval < APP_GLUCOSE_MEAS_MEAS_INTERVAL_MIN) g_ucGlucoseMeasInterval = APP_GLUCOSE_MEAS_MEAS_INTERVAL_MIN;

    // 如果当前到了测量间隔时间计数
    if (g_ucGlucoseMeasTimeCnt >= (g_ucGlucoseMeasInterval))
    {
        //log_i("app_glucose_handle:%d\r\n", g_ucAvgElectricCurrentCalTempArrayCnt);

        // 计算当前电流平均值并调用血糖处理函数
        app_glucose_handle();

        // 初始化平均电流计算(清除之前的电流临时数据)
        app_glucose_avg_electric_current_cal_init();

        // 清零测量间隔时间计数
        g_ucGlucoseMeasTimeCnt = 0;
    }

    // 累计时间
    g_ucGlucoseMeasTimeCnt++;
}

/*******************************************************************************
*                           陈苏阳@2023-05-29
* Function Name  :  app_glucose_meas_timer_handler
* Description    :  血糖测量与转换定时处理函数(周期1S一次)
* Input          :  ke_msg_id_t const msg_id
* Input          :  void const * param
* Input          :  ke_task_id_t const dest_id
* Input          :  ke_task_id_t const src_id
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_handler(void)
{
    // 获取当前RTC时间
    uint32_t ucNowRtcTime = rtc_get_curr_time();
    // 计算当前RTC时间差值
    int32_t uiRtcTimeDiff = ucNowRtcTime - g_UiListRtcTime;
    // 判断当前是否需要执行1S一次的数据采集
    if (uiRtcTimeDiff >= 1000)
    {
        // 更新RTC时间
    	g_UiListRtcTime = ucNowRtcTime;

        // 测量电量
        app_glucose_meas_battery_sub_handler();

        // 如果当前记录已满且BLE的传感器状态属于正在采集的状态
        if (g_usGlucoseRecordsCurrentOffset >= CGMS_DB_MAX_RECORDS && app_global_is_session_runing())
        {
            log_i("app_glucose_meas_stop_session_handler");

            // 停止CGM
            app_glucose_meas_stop_session_handler();

            // 停止本定时器
            app_glucose_meas_stop();
        }

        // 如果当前已经开始了一次血糖测量周期
        if (app_global_is_session_runing())
        {
            // 如果AFE还未开始工作,则启动AFE
            if (afe_is_working() == false)
            {
                afe_start();
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
    event_push(MAIN_LOOP_EVENT_APP_GLUCOSE_MEAS_1S_TIMER);
}


/*******************************************************************************
*                           陈苏阳@2022-12-22
* Function Name  :  app_glucose_meas_start
* Description    :  血糖测量开始
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_start(void)
{
    log_d("app glucose meas stop");
    // 启动一个1S的循环定时器
    sl_status_t status = sl_sleeptimer_start_periodic_timer(&g_AppGlucoseMeasTimer, sl_sleeptimer_ms_to_tick(1000), app_glucose_meas_timer_callback, (void*)NULL, 0, 0);
    if (status != SL_STATUS_OK)
    {
        log_e("sl_sleeptimer_start_timer failed");
    }
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
    event_push(MAIN_LOOP_EVENT_APP_GLUCOSE_MEAS_RECORD_SEND_TIMER);
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
    
}


/*******************************************************************************
*                           陈苏阳@2022-12-15
* Function Name  :  app_glucose_meas_record_send_handelr
* Description    :  发送记录定时处理函数
* Input          :  None
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_glucose_meas_record_send_handelr(void)
{
    bool bSendSuccessFlag = true;
    log_i("app_glucose_meas_record_send_handler:%d", app_global_get_app_state()->bRecordSendFlag);

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
                // 发送历史数据
                cgms_meas_send(app_global_get_app_state()->RecordOptInfo.BleEventInfo, HistoryRec);
            }
            else
            {
                bSendSuccessFlag = false;
                log_i("cgms_db_record_get err");
            }

            // 100ms后继续发送血糖数据记录
            if ((is_measurement_notify()) && (app_global_get_app_state()->bSentSocpSuccess == true) && (app_global_get_app_state()->bBleConnected == true))
            {
                // 发送计数累计(只有在发送成功时才累计)
                if (bSendSuccessFlag)
                {
                    app_global_get_app_state()->RecordOptInfo.usRacpRecordSendCnt++;
                    app_global_get_app_state()->RecordOptInfo.usRacpRecordStartIndex++;
                }
                app_glucose_meas_record_send_start();
            }
        }
        else
        {
            log_i("record send done");
            // 发送代表历史数据发送完成的"0xAA"数据包
            cgms_history_special_datapcket_t CgmsHistorySpecialDatapcket;

            // 设置code为发送完成
            CgmsHistorySpecialDatapcket.ucDatapacketCode = CGMS_HISTORY_SPECIAL_DATAPACKET_CODE_SEND_DONE;
            // 计算CRC
            CgmsHistorySpecialDatapcket.usCRC16 = do_crc(&CgmsHistorySpecialDatapcket, sizeof(CgmsHistorySpecialDatapcket) - 2);

            // 发送特殊血糖数据
            if (cgms_meas_special_send(app_global_get_app_state()->RecordOptInfo.BleEventInfo, CgmsHistorySpecialDatapcket) != RET_CODE_SUCCESS)
            {
                bSendSuccessFlag = false;
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
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_battery_meas_handelr(void)
{
    sl_status_t status;
    // 如果当前已经开始CGM,则关闭本定时器,由CGM定时函数来做电量检测
    if (app_global_is_session_runing())return;
    if (g_ucAppBatteryInitMeasDoneFlag < 2)
    {
        // 触发电量采集定时器处理函数
        app_battery_timer_handler(0);

        g_ucAppBatteryInitMeasDoneFlag++;

        // 启动一个500ms的单次定时器
        status = sl_sleeptimer_start_timer(&g_AppBatteryMeasTimer, sl_sleeptimer_ms_to_tick(500), app_battery_meas_timer_callback, (void*)NULL, 0, 0);
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
    event_push(MAIN_LOOP_EVENT_APP_BATTERY_MEAS_TIMER);
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
    event_add(MAIN_LOOP_EVENT_APP_GLUCOSE_MEAS_1S_TIMER, app_glucose_meas_handler);
    event_add(MAIN_LOOP_EVENT_APP_GLUCOSE_MEAS_RECORD_SEND_TIMER, app_glucose_meas_record_send_handelr);

    // 启动一个500ms的单次定时器
    sl_status_t status = sl_sleeptimer_start_timer(&g_AppBatteryMeasTimer, sl_sleeptimer_ms_to_tick(500), app_battery_meas_timer_callback, (void*)NULL, 0, 0);
    if (status != SL_STATUS_OK)
    {
        log_e("sl_sleeptimer_start_timer failed");
    }
}


/******************* (C) COPYRIGHT 2022 陈苏阳 **** END OF FILE ****************/




