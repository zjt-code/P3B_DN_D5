/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_socp.c
* Author             :  陈苏阳
* CPU Type         	 :  RSL15
* IDE                :  Onsemi IDE
* Version            :  V1.0
* Date               :  27/3/2023
* Description        :
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#if !defined(LOG_TAG)
#define LOG_TAG                   "CGMS_SOCP"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_DEBUG

#include <stdint.h>
#include "stdio.h"
#include <string.h>
#include <app.h>
#include "cgms_sst.h"
#include "cgms_socp.h"
#include "cgms_crc.h"
#include "app_util.h"
#include "app_global.h"
#if ((USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL) ||(USE_BLE_PROTOCOL==GN_2_PROTOCOL))
#include "cgms_aes128.h"
#include "utility.h"
#endif

#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
#include "cgms_racp.h"
#include "simplegluco.h"
#endif
#include "ble_customss.h"
#include "sl_bt_api.h"
#include "app_glucose_meas.h"
#include "cgms_prm.h"
#include "btl_interface.h"
#include <elog.h>
#include "app_glucose_meas.h"
#include "cgms_debug_db.h"
#include "simplegluco.h"
#include "cur_filter.h"
#include "afe.h"
/* Private variables ---------------------------------------------------------*/
#define NRF_BLE_CGMS_PLUS_INFINTE                     0x07FE
#define NRF_BLE_CGMS_MINUS_INFINTE                    0x0802
static bool g_bBleSocpNotifyIsEnableFlag = false;						// BLE SOCP通知使能标志位
#if ((USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL) ||(USE_BLE_PROTOCOL==GN_2_PROTOCOL))
static bool g_bProduction=false;
static uint16_t g_usBleProtocolPassword =0;
#endif
//extern float sfCurrBg;//add by woo
uint8_t	caliTag;
uint8_t calData[15];
uint8_t calDataFlag = 0;
uint32_t simDelta = 0;
uint32_t simMax = 0;
uint32_t simMin = 0;
/* Private function prototypes -----------------------------------------------*/
static ret_code_t socp_send(ble_event_info_t BleEventInfo, ble_socp_rsp_t SocpRspDatapacket);
bool cgms_socp_check_production_cmd(uint8_t* pData, uint16_t usLen);
uint8_t ble_socp_encode(const ble_socp_rsp_t* pSocpRsp, uint8_t* pData);
/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  ble_socp_notify_enable
* Description    :  通知被使能
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_socp_notify_enable(void)
{
    g_bBleSocpNotifyIsEnableFlag = true;
}

/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  ble_socp_notify_disable
* Description    :  通知被失能
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_socp_notify_disable(void)
{
    g_bBleSocpNotifyIsEnableFlag = false;
}

/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  ble_socp_notify_is_enable
* Description    :  判断当前是否使能通知
* Input          :  void
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool ble_socp_notify_is_enable(void)
{
    return g_bBleSocpNotifyIsEnableFlag;
}


/*******************************************************************************
*                           陈苏阳@2023-03-27
* Function Name  :  ble_socp_decode
* Description    :  解码SOCP特征收到的数据包
* Input          :  uint8_t ucDataLen
* Input          :  uint8_t * pData
* Input          :  ble_cgms_socp_datapacket_t * pSocpDatapacket
* Output         :  None
* Return         :  void
*******************************************************************************/
static void ble_socp_decode(uint8_t ucDataLen, uint8_t* pData, ble_socp_datapacket_t* pSocpDatapacket)
{
    pSocpDatapacket->ucOpCode = 0xFF;
    pSocpDatapacket->ucDataLen = 0;
    pSocpDatapacket->pData = NULL;
    // 如果数据包长度大于0
    if (ucDataLen > 0)
    {
        // 数据包第一个字节为操作码
        pSocpDatapacket->ucOpCode = pData[0];
    }
    if (ucDataLen > 1)
    {
        // 如果数据包长度大于1,则除了第一个字节是操作码以外,后面的都是携带的数据
        pSocpDatapacket->ucDataLen = ucDataLen - 1;
        pSocpDatapacket->pData = &pData[1];
    }
}


/*******************************************************************************
*                           陈苏阳@2023-03-27
* Function Name  :  ble_socp_encode
* Description    :  编码要发送的SOCP数据包
* Input          :  const ble_socp_rsp_t * pSocpRsp
* Input          :  uint8_t * p_data
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t ble_socp_encode(const ble_socp_rsp_t* pSocpRsp, uint8_t* pData)
{
    uint8_t ucLen = 0;
    if (pData != NULL)
    {
        // 提取操作码并放到数据包中
        pData[ucLen++] = pSocpRsp->ucOpCode;

        // 如果是以下操作码(回应包中不含数据)
        if (
            (pSocpRsp->ucOpCode != SOCP_READ_CGM_COMMUNICATION_INTERVAL_RESPONSE)
            && (pSocpRsp->ucOpCode != SOCP_READ_GLUCOSE_CALIBRATION_VALUE_RESPONSE)
            && (pSocpRsp->ucOpCode != SOCP_READ_PRM_RESPONSE)
            && (pSocpRsp->ucOpCode!= SOCP_READ_AD_CALI_DATA)
            )
        {
            // 则直接在后面添加回应操作码以及回应结果码
            pData[ucLen++] = pSocpRsp->ucReqOpcode;
            pData[ucLen++] = pSocpRsp->ucRspCode;
        }

        // 否则在后面添加数据
        for (uint8_t i = 0; i < pSocpRsp->ucSizeVal; i++)
        {
            pData[ucLen++] = pSocpRsp->ucRespVal[i];
        }
    }
    // 计算并添加CRC
    ucLen += uint16_encode(do_crc(pData, ucLen), &pData[ucLen]);

    return ucLen;
}

/*******************************************************************************
*                           陈苏阳@2023-03-27
* Function Name  :  socp_send
* Description    :  发送SOCP数据包
* Input          :  ble_event_info_t BleEventInfo
* Input          :  ble_socp_rsp_t SocpRspDatapacket
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
static ret_code_t socp_send(ble_event_info_t BleEventInfo, ble_socp_rsp_t SocpRspDatapacket)
{
    uint8_t EncodedRespDatapacketBuffer[16];
    // 编码数据包
    uint8_t ucLen = ble_socp_encode(&SocpRspDatapacket, EncodedRespDatapacketBuffer);
    // 发送数据包
    elog_hexdump("socp_send", 8, EncodedRespDatapacketBuffer, ucLen);

#if ((USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL) ||(USE_BLE_PROTOCOL==GN_2_PROTOCOL))
    //如果当前处理的是生产命令,则跳过加密,如果是正常命令,则走加密流程发包
    if (g_bProduction == false)
    {
        // 根据通讯协议对原始数据包进行填充和加密
        ucLen = datapacket_padding_and_encrpty(EncodedRespDatapacketBuffer, EncodedRespDatapacketBuffer, ucLen);
    }
    //发送数据包
    elog_hexdump("socp_send(encrpty)", 8, EncodedRespDatapacketBuffer, ucLen);
#endif

    if ((ble_socp_notify_is_enable()) && app_have_a_active_ble_connect())
    {
        sl_status_t sc;
        sc = sl_bt_gatt_server_send_notification(BleEventInfo.ucConidx, BleEventInfo.usHandle, ucLen, EncodedRespDatapacketBuffer);
        if (sc == SL_STATUS_OK)
        {
            //app_global_get_app_state()->bSentSocpSuccess = false;
            return RET_CODE_SUCCESS;
        }
        else
        {
            log_w("sl_bt_gatt_server_send_notification fail:%d", sc);
        }
    }
    else
    {
        if (ble_socp_notify_is_enable() == false)
        {
            log_w("ble socp notify not enable");
        }
        if (!app_have_a_active_ble_connect())
        {
            log_w("ble connected is false");
        }
    }
    return RET_CODE_FAIL;
}

/*******************************************************************************
*                           陈苏阳@2024-02-20
* Function Name  :  cgms_socp_stop_session_event_callback
* Description    :  停止测量事件回调
* Input          :  uint32_t uiArg
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_socp_stop_session_event_callback(__attribute__((unused))  uint32_t uiArg)
{
    // 停止CGM
    app_glucose_meas_stop_session_handler(CGM_MEASUREMENT_SENSOR_STATUS_SESSION_COMMAND_STOPPED);
    app_glucose_meas_stop();
    cmgs_db_force_write_flash();
}

/*******************************************************************************
*                           陈苏阳@2024-02-22
* Function Name  :  cgms_socp_start_session_event_callback
* Description    :  开始测量事件回调
* Input          :  uint32_t uiArg
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_socp_start_session_event_callback(uint32_t uiArg)
{
    // 血糖测量开始
    app_glucose_meas_start(uiArg);

    // 清空历史数据
    cgms_db_reset();

    // 更新CGM Status中的运行状态为极化中
    app_global_get_app_state()->Status = CGM_MEASUREMENT_SENSOR_STATUS_SESSION_RUNNING;
    att_get_cgm_status()->ucRunStatus = app_global_get_app_state()->Status;

    // 更新CGM状态char的内容
    att_update_cgm_status_char_data();
}

/*******************************************************************************
*                           陈苏阳@2024-06-24
* Function Name  :  cgms_socp_write_glucose_calibration_value
* Description    :  写入血糖校准值
* Input          :  ble_event_info_t BleEventInfo,
* Input          :  ble_socp_datapacket_t SocpRequest
* Input          :  bool bCrcPass
* Output         :  ble_socp_rsp_t * pRspRequest
* Return         :  void
*******************************************************************************/
void cgms_socp_write_glucose_calibration_value(__attribute__((unused))  ble_event_info_t BleEventInfo, ble_socp_datapacket_t  SocpRequest, bool bCrcPass, ble_socp_rsp_t* pRspRequest)
{
#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
    // 判断长度是否正确
    if (SocpRequest.ucDataLen < 7)
    {
        // 命令长度错误
        pRspRequest->ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_COMMAND_LEN_ERR;
        return;
    }

    // 效验命令的CRC
    if (!bCrcPass)
    {
        // CRC错误
        pRspRequest->ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_CRC_ERR;
        return;
    }

    // 填充结构体
    ble_cgms_socp_write_glucose_calibration_datapacket_t SocpWriteGlucoseCalibrationDatapacket;
    memcpy(&SocpWriteGlucoseCalibrationDatapacket, SocpRequest.pData, sizeof(SocpWriteGlucoseCalibrationDatapacket));

    // 判断当前血糖趋势是否稳定
    if ((app_global_get_app_state()->CgmTrend != CGM_TREND_DOWN_DOWN) && (app_global_get_app_state()->CgmTrend != CGM_TREND_STABLE) && (app_global_get_app_state()->CgmTrend != CGM_TREND_SLOW_UP))
    {
        pRspRequest->ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_GLUCOSE_FLUCTUATE;
        return;
    }

    // 判断当前运行状态是否为正常
    if (att_get_cgm_status()->ucRunStatus != CGM_MEASUREMENT_SENSOR_STATUS_SESSION_RUNNING)
    {
        pRspRequest->ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_SENSOR_ABNORMAL;
        return;
    }

    // 判断校准值是否在合理区间
    if ((SocpWriteGlucoseCalibrationDatapacket.usCalibration >= 222) || (SocpWriteGlucoseCalibrationDatapacket.usCalibration <= 22))
    {
        pRspRequest->ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_RANGE_OUT;
        return;
    }
    // 判断是否处于第一天,返回极化中无法校准
    if (app_glucose_get_records_current_offset() < 480 - 1)
    {
        pRspRequest->ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_ON_THE_FIRST_DAY;
        return;
    }
    // 判断是否有跳点
    if (!gvg_get_result() && !gluco_check_bg((float)(SocpWriteGlucoseCalibrationDatapacket.usCalibration / 10.0f)))
    {
        pRspRequest->ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_ON_THE_30_MIN;
    }

    usBfFlg = 1;
    sfCurrBg = (float)(SocpWriteGlucoseCalibrationDatapacket.usCalibration / 10.0f);
    log_i("sfCurrBg:%f  Offset:%d\r\n", (float)(SocpWriteGlucoseCalibrationDatapacket.usCalibration / 10.0f), SocpWriteGlucoseCalibrationDatapacket.usOffset);
    pRspRequest->ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_SUCCESS;
#else
    if (app_global_is_session_runing())
    {
        calDataFlag = 1;
        uint16_t concentration = uint16_decode(SocpRequest.pData);
        if (concentration == 0)concentration = 1;
        usBfFlg = 1;
        sfCurrBg = (float)concentration / 100.0f;
        log_i("sfCurrBg:%f\r\n", sfCurrBg);
        pRspRequest->ucRspCode = SOCP_RSP_SUCCESS;
    }
    else
    {
        // 状态错误
        pRspRequest->ucRspCode = SOCP_RSP_PROCEDURE_NOT_COMPLETED;
    }
#endif
}

/*******************************************************************************
*                           陈苏阳@2024-06-24
* Function Name  :  cgms_socp_start_the_session
* Description    :  开始新的CGM测量周期
* Input          :  ble_event_info_t BleEventInfo,
* Input          :  ble_socp_datapacket_t SocpRequest
* Input          :  bool bCrcPass
* Output         :  ble_socp_rsp_t * pRspRequest
* Return         :  void
*******************************************************************************/
void cgms_socp_start_the_session(__attribute__((unused))  ble_event_info_t BleEventInfo,__attribute__((unused)) ble_socp_datapacket_t  SocpRequest,bool bCrcPass, ble_socp_rsp_t* pRspRequest)
{
#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
    
    // 效验命令的CRC
    if (!bCrcPass)
    {
        log_e("crc error");
        // CRC错误
        pRspRequest->ucRspCode = SOCP_START_THE_SESSION_RSP_CODE_CRC_ERR;
        return;
    }
    
    // 填充结构体
    ble_cgms_socp_start_the_session_datapacket_t SocpStartTheSessionDatapacket;
    memcpy(&SocpStartTheSessionDatapacket, SocpRequest.pData, sizeof(SocpStartTheSessionDatapacket));

    // 如果当前CGM已经运行或者处于极化中
    if ((att_get_cgm_status()->ucRunStatus == CGM_MEASUREMENT_SENSOR_STATUS_SESSION_RUNNING) || (att_get_cgm_status()->ucRunStatus == CGM_MEASUREMENT_SENSOR_STATUS_SESSION_WARM_UP))
    {
        log_e("state==runing or state==warm_up");
        // 返回对应错误码
        pRspRequest->ucRspCode = SOCP_START_THE_SESSION_RSP_CODE_IS_STARTED;
    }
    else
    {
        // 如果CGM因为APP的停止命令而处于已经停止的状态
        if (att_get_cgm_status()->ucRunStatus == CGM_MEASUREMENT_SENSOR_STATUS_SESSION_COMMAND_STOPPED)
        {
            log_e("state==command stoped");
            // 则不能重新启动,返回错误码
            pRspRequest->ucRspCode = SOCP_START_THE_SESSION_RSP_CODE_IS_STOPED;
            return;
        }

        // 如果CGM已到期
        if (att_get_cgm_status()->ucRunStatus == CGM_MEASUREMENT_SENSOR_STATUS_SENSION_EXPRIED)
        {
            log_e("state==is end");
            // 则不能重新启动,返回错误码
            pRspRequest->ucRspCode = SOCP_START_THE_SESSION_RSP_CODE_IS_END;
            return;
        }

        // 计算工厂校准码
        float fTmpSensorK = (float)SocpStartTheSessionDatapacket.usFactoryCode / 1000.0f;
        log_d("SensorK:%f", fTmpSensorK);
        // 如果工厂校准码不合法
        if ((fTmpSensorK < SOCP_SET_SENSOR_CODE_MIN_ERR_VAL) || (fTmpSensorK > SOCP_SET_SENSOR_CODE_MAX_ERR_VAL))
        {
            log_w("SensorK err");
            // 返回错误码
            pRspRequest->ucRspCode = SOCP_START_THE_SESSION_RSP_CODE_SENSOR_CODE_ERR;
            return;
        }
        else
        {
            sensorK = fTmpSensorK;
            // 生效工厂校准码
            cur_get_cur_error_value(fTmpSensorK);
            log_i("cur_get_cur_error_value(%f)", fTmpSensorK);

            extern float cur_error_min_value;
            extern float cur_error_max_value;

            log_i("cur_error_min_value(%f)", cur_error_min_value);
            log_i("cur_error_max_value(%f)", cur_error_max_value);

            // 更新CGM Status中的工厂校准码
            att_get_cgm_status()->usFactoryCode = SocpStartTheSessionDatapacket.usFactoryCode;
        }

        // 设置返回结果
        pRspRequest->ucRspCode = SOCP_START_THE_SESSION_RSP_CODE_SUCCESS;

        // 更新feature char中的启动来源(高四位)
        att_get_feature()->ucStartBy = SocpStartTheSessionDatapacket.ucFrom << 4;

        // 更新start time char中的启动时间
        att_get_start_time()->uiStartTime = SocpStartTheSessionDatapacket.uiStartTime;

        // 更新start time char中的时区
        att_get_start_time()->ucTimeZone = SocpStartTheSessionDatapacket.ucTimeZone;

        // 更新CGM Status中的运行状态为极化中
        app_global_get_app_state()->Status = CGM_MEASUREMENT_SENSOR_STATUS_SESSION_WARM_UP;
        att_get_cgm_status()->ucRunStatus = app_global_get_app_state()->Status;

        // 更新Feature char的CRC
        att_update_feature_char_data_crc();

        // 更新Start Time char的CRC
        att_update_start_time_char_data_crc();

        // 更新CGM Status char的CRC
        att_update_cgm_status_char_data_crc();

        // 更新启动时间
        cgms_update_sst_and_time_zone(SocpStartTheSessionDatapacket.uiStartTime, SocpStartTheSessionDatapacket.ucTimeZone);

        // 更新record_index中的SST
        cgms_db_record_index_update_sst(g_mSST);

        // 清空历史数据
        cgms_db_reset();

        log_i("!!SOCP_START_THE_SESSION   OK!!");

        // 开始应用层血糖测量
        event_push(MAIN_LOOP_EVENT_SOCP_START_SESSION_EVENT, APP_GLUCOSE_MEAS_TYPE_USER_MEAS);
    }

#else
    if (app_global_is_session_runing())
    {
        // 状态错误
        pRspRequest->ucRspCode = SOCP_RSP_PROCEDURE_NOT_COMPLETED;
    }
    else
    {
        log_i("!!SOCP_START_THE_SESSION   OK!!");
        pRspRequest->ucRspCode = SOCP_RSP_SUCCESS;
        event_push(MAIN_LOOP_EVENT_SOCP_START_SESSION_EVENT, APP_GLUCOSE_MEAS_TYPE_USER_MEAS);
    }
#endif
}

/*******************************************************************************
*                           陈苏阳@2024-06-24
* Function Name  :  cgms_socp_stop_the_session
* Description    :  停止当前的CGM测量周期
* Input          :  ble_event_info_t BleEventInfo,
* Input          :  ble_socp_datapacket_t SocpRequest
* Input          :  bool bCrcPass
* Output         :  ble_socp_rsp_t * pRspRequest
* Return         :  void
*******************************************************************************/
void cgms_socp_stop_the_session(__attribute__((unused))  ble_event_info_t BleEventInfo, __attribute__((unused))  ble_socp_datapacket_t  SocpRequest, bool bCrcPass, ble_socp_rsp_t* pRspRequest)
{
#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)

    uint8_t ucIsStopedFlag = 0;
    // 判断发射器当前是否已经处于停止状态
    switch (att_get_cgm_status()->ucRunStatus)
    {
    case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_STOPPED:							// CGM结束
    case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_COMMAND_STOPPED:					// 由于APP发送停止命令导致的停止
    case CGM_MEASUREMENT_SENSOR_STATUS_SENSION_EXPRIED:							// CGM到期停止
    case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_SENSOR_ABNORMAL:					// 传感器异常,等待恢复(预设3小时)
    case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_INEFFECTIVE_IMPLANTATION:		// 无效植入,请更换传感器(数据停止采样,蓝牙广播继续)
    case CGM_MEASUREMENT_SENSOR_STATUS_UNEXPECTED_STOP1:						// 意外停止(情况1)
    case CGM_MEASUREMENT_SENSOR_STATUS_UNEXPECTED_STOP2:						// 意外停止(情况2)
    {
        // 发射器已经处于停止状态
        ucIsStopedFlag = 1;
        break;
    }
    default:
    {
        break;
    }
    }

    // 发射器已经处于停止状态
    if (ucIsStopedFlag)
    {
        // 返回错误码
        pRspRequest->ucRspCode = SOCP_STOP_THE_SESSION_RSP_CODE_IS_STOPED;
        return;
    }

    // 更新CGM Status中的运行状态为停止状态
    att_get_cgm_status()->ucRunStatus = CGM_MEASUREMENT_SENSOR_STATUS_SESSION_COMMAND_STOPPED;

    // 更新CGM Status char的CRC
    att_update_cgm_status_char_data_crc();

    // 停止血糖测量
    app_glucose_meas_stop();

#else
    if (app_global_is_session_runing())
    {
        pRspRequest->ucOpCode = SOCP_RSP_SUCCESS;
        event_push(MAIN_LOOP_EVENT_SOCP_STOP_SESSION_EVENT, (void*)NULL);
    }
    else
    {
        // 状态错误
        pRspRequest->ucRspCode = SOCP_RSP_PROCEDURE_NOT_COMPLETED;
    }
#endif
}

/*******************************************************************************
*                           陈苏阳@2024-06-24
* Function Name  :  cgms_socp_read_hard_fault_info
* Description    :  读取硬错误信息
* Input          :  ble_event_info_t BleEventInfo,
* Input          :  ble_socp_datapacket_t SocpRequest
* Input          :  bool bCrcPass
* Output         :  ble_socp_rsp_t * pRspRequest
* Return         :  void
*******************************************************************************/
void cgms_socp_read_hard_fault_info(ble_event_info_t BleEventInfo, __attribute__((unused))  ble_socp_datapacket_t  SocpRequest, bool bCrcPass, ble_socp_rsp_t* pRspRequest)
{
    log_i("SOCP_READ_HARD_FAULT_INFO");
    pRspRequest->ucOpCode = SOCP_RSP_SUCCESS;
    cgms_debug_db_read();
    pRspRequest->ucSizeVal = 0;
    for (uint8_t i = 0; i < CGMS_DEBUG_MAX_KV_NUM; i++)
    {
        char cKey[12];
        uint32_t uiData;

        if (debug_get_kv(i, cKey, (uint8_t*)&uiData))
        {
            log_i("index:%d key:%s val:0x%xd", i, cKey, uiData);
            memcpy(&(pRspRequest->ucRespVal[pRspRequest->ucSizeVal]), &uiData, 4);
            pRspRequest->ucSizeVal += 4;
        }
        else
        {
            break;
        }

        if (pRspRequest->ucSizeVal >= 16)
        {
            socp_send(BleEventInfo, (*pRspRequest));
            pRspRequest->ucSizeVal = 0;
        }
    }
    cgms_debug_clear_all_kv();

    socp_send(BleEventInfo, *pRspRequest);
}


/*******************************************************************************
*                           陈苏阳@2024-06-24
* Function Name  :  cgms_socp_write_sensor_code
* Description    :  写入传感器Code
* Input          :  ble_event_info_t BleEventInfo,
* Input          :  ble_socp_datapacket_t SocpRequest
* Input          :  bool bCrcPass
* Output         :  ble_socp_rsp_t * pRspRequest
* Return         :  void
*******************************************************************************/
void cgms_socp_write_sensor_code(__attribute__((unused)) ble_event_info_t BleEventInfo, ble_socp_datapacket_t  SocpRequest, bool bCrcPass, ble_socp_rsp_t* pRspRequest)
{
    uint16_t usSensorCode;
    usSensorCode = uint16_decode(SocpRequest.pData + 1);

    // 如果Code为0,则说明Code无效
    if (usSensorCode == 0)
    {
        pRspRequest->ucRspCode = SOCP_RSP_INVALID_OPERAND;
    }
    else
    {
    // 更新CGM Status中的工厂校准码
    att_get_cgm_status()->usFactoryCode = usSensorCode;
    // 更新CGM状态char的内容
    att_update_cgm_status_char_data();
    //设置传感器Code
    sensorK = (float)usSensorCode / 1000.0f;
    cur_get_cur_error_value(sensorK);
    log_i("cur_get_cur_error_value(%f)", sensorK);
    log_i("sensor code update:%d", usSensorCode);

    extern float cur_error_min_value;
    extern float cur_error_max_value;

    log_i("cur_error_min_value(%f)", cur_error_min_value);
    log_i("cur_error_max_value(%f)", cur_error_max_value);

        log_i("sensor code update done");
        pRspRequest->ucRspCode = SOCP_RSP_SUCCESS;
        app_global_get_app_state()->isfs = true;
        //cur_get_cur_error_value(sensorK);

          // 在CGM status Char中设置code码
        att_get_cgm_status()->usFactoryCode = usSensorCode;
        // 更新CGM状态char的内容
        att_update_cgm_status_char_data();
    }
}

/*******************************************************************************
*                           陈苏阳@2024-06-24
* Function Name  :  cgms_socp_start_fota
* Description    :  触发固件升级
* Input          :  ble_event_info_t BleEventInfo,
* Input          :  ble_socp_datapacket_t SocpRequest
* Input          :  bool bCrcPass
* Output         :  ble_socp_rsp_t * pRspRequest
* Return         :  void
*******************************************************************************/
void cgms_socp_start_fota(__attribute__((unused)) ble_event_info_t BleEventInfo, __attribute__((unused)) ble_socp_datapacket_t  SocpRequest, bool bCrcPass, ble_socp_rsp_t* pRspRequest)
{
    // 触发进入OTA
    app_global_ota_start();
    pRspRequest->ucRspCode = SOCP_RSP_SUCCESS;
}


/*******************************************************************************
*                           陈苏阳@2024-06-24
* Function Name  :  cgms_socp_read_reset_reg
* Description    :  读取复位原因寄存器
* Input          :  ble_event_info_t BleEventInfo,
* Input          :  ble_socp_datapacket_t SocpRequest
* Output         :  ble_socp_rsp_t * pRspRequest
* Return         :  void
*******************************************************************************/
void cgms_socp_read_reset_reg(__attribute__((unused)) ble_event_info_t BleEventInfo,__attribute__((unused))  ble_socp_datapacket_t  SocpRequest, bool bCrcPass, ble_socp_rsp_t* pRspRequest)
{
    extern uint32_t g_uiRstCause ;
    memcpy(&(pRspRequest->ucRespVal[0]), &g_uiRstCause , 4);
    pRspRequest->ucRespVal[4] = 0x00;
    pRspRequest->ucRespVal[5] = 0x00;
    pRspRequest->ucRespVal[6] = 0x00;
    pRspRequest->ucRespVal[7] = 0x00;
    pRspRequest->ucSizeVal = 8;
}

/*******************************************************************************
*                           陈苏阳@2024-06-24
* Function Name  :  cgms_socp_start_ad_cali
* Description    :  校准ADC
* Input          :  ble_event_info_t BleEventInfo,
* Input          :  ble_socp_datapacket_t SocpRequest
* Input          :  bool bCrcPass
* Output         :  ble_socp_rsp_t * pRspRequest
* Return         :  void
*******************************************************************************/
void cgms_socp_start_ad_cali(__attribute__((unused)) ble_event_info_t BleEventInfo,__attribute__((unused))  ble_socp_datapacket_t  SocpRequest, bool bCrcPass, ble_socp_rsp_t* pRspRequest)
{
    if (app_global_is_session_runing())
    {
        // 状态错误
        pRspRequest->ucRspCode = SOCP_RSP_PROCEDURE_NOT_COMPLETED;
    }
    else
    {
        log_i("!!SOCP_START_AD_CALI   OK!!");
        pRspRequest->ucRspCode = SOCP_RSP_SUCCESS;
        event_push(MAIN_LOOP_EVENT_SOCP_START_SESSION_EVENT, APP_GLUCOSE_MEAS_TYPE_FACTORY_MEAS);
    }
}

/*******************************************************************************
*                           陈苏阳@2024-06-24
* Function Name  :  cgms_socp_read_ad_cali_data
* Description    :  读取校准ADC时的数据
* Input          :  ble_event_info_t BleEventInfo,
* Input          :  ble_socp_datapacket_t SocpRequest
* Input          :  bool bCrcPass
* Output         :  ble_socp_rsp_t * pRspRequest
* Return         :  void
*******************************************************************************/
void cgms_socp_read_ad_cali_data(__attribute__((unused)) ble_event_info_t BleEventInfo,__attribute__((unused))  ble_socp_datapacket_t  SocpRequest, bool bCrcPass, ble_socp_rsp_t* pRspRequest)
{
    uint32_t uiMeasElectricCurrent = 0;
    app_glucose_meas_get_factory_meas_electric_current(&uiMeasElectricCurrent);
    log_i("read adc cali data:%d", uiMeasElectricCurrent);
    memcpy(&(pRspRequest->ucRespVal[0]), &uiMeasElectricCurrent, 4);
    pRspRequest->ucSizeVal = 4;
    pRspRequest->ucRspCode = SOCP_RSP_SUCCESS;
    calDataFlag = 0;
}

/*******************************************************************************
*                           陈苏阳@2024-06-24
* Function Name  :  cgms_socp_write_prm
* Description    :  写入参数
* Input          :  ble_event_info_t BleEventInfo,
* Input          :  ble_socp_datapacket_t SocpRequest
* Input          :  bool bCrcPass
* Output         :  ble_socp_rsp_t * pRspRequest
* Return         :  void
*******************************************************************************/
void cgms_socp_write_prm(__attribute__((unused)) ble_event_info_t BleEventInfo, ble_socp_datapacket_t  SocpRequest, bool bCrcPass, ble_socp_rsp_t* pRspRequest)
{
    if (SocpRequest.ucDataLen > 16)
    {
        pRspRequest->ucRspCode = SOCP_RSP_INVALID_OPERAND;
    }
    else
    {
        uint8_t ucIndex = 0;
        uint8_t ucPrmNo = 0;
        int16_t sTmp;
        ucPrmNo = SocpRequest.pData[ucIndex++];

        switch (ucPrmNo)
        {
            // 写SN
        case SOCP_PRM_NO_WRITE_OR_READ_SN:
        {
            pRspRequest->ucRspCode = SOCP_RSP_SUCCESS;
            g_PrmDb.prmWMY[0] = *(SocpRequest.pData + ucIndex);
            ucIndex += 1;
            g_PrmDb.prmWMY[1] = *(SocpRequest.pData + ucIndex);
            ucIndex += 1;
            g_PrmDb.prmWMY[2] = *(SocpRequest.pData + ucIndex);
            ucIndex += 1;
            g_PrmDb.prmWMY[3] = 0;
            sTmp = uint16_decode(SocpRequest.pData + ucIndex);
            g_PrmDb.SN = (int16_t)sTmp;
			log_i("write SN:%s%04d",g_PrmDb.prmWMY,g_PrmDb.SN);
            ucIndex += 2;
            g_PrmDb.Crc16 = do_crc((uint8_t*)&g_PrmDb, sizeof(g_PrmDb) - 2);
            break;
        }
        // 写AFE电压偏移
        case SOCP_PRM_WRITE_AFE_VOL_OFFISET:
        {
            pRspRequest->ucRspCode = SOCP_RSP_SUCCESS;
            memcpy(&g_PrmDb.DacVolOffset, &(SocpRequest.pData[ucIndex]), 2);
            log_i("write dac vol offset:%d", g_PrmDb.DacVolOffset);
            g_PrmDb.Crc16 = do_crc((uint8_t*)&g_PrmDb, sizeof(g_PrmDb) - 2);

            // 准实时地更新AFE的电压偏移
            update_vol_offset(g_PrmDb.DacVolOffset);
            break;
        }
        // 写AFE系数K
        case SOCP_PRM_WRITE_AFE_COEFFICIENT_K:
        {
            pRspRequest->ucRspCode = SOCP_RSP_SUCCESS;
            memcpy(&g_PrmDb.AdcK, &(SocpRequest.pData[ucIndex]), 2);
            log_i("write adc k:%d", g_PrmDb.AdcK);
            g_PrmDb.Crc16 = do_crc((uint8_t*)&g_PrmDb, sizeof(g_PrmDb) - 2);
            break;
        }
        // 写AFE系数B
        case SOCP_PRM_WRITE_AFE_COEFFICIENT_B:
        {
            pRspRequest->ucRspCode = SOCP_RSP_SUCCESS;
            memcpy(&g_PrmDb.AdcB, &(SocpRequest.pData[ucIndex]), 2);
            log_i("write adc b:%d", g_PrmDb.AdcB);
            g_PrmDb.Crc16 = do_crc((uint8_t*)&g_PrmDb, sizeof(g_PrmDb) - 2);
            break;
        }
#if (USE_BLE_PROTOCOL!=GN_2_PROTOCOL)
        // 写启动时间
        case SOCP_PRM_NO_WRITE_START_TIME:
        {
            ble_cgms_socp_write_start_time_datapacket_data_t SetStartTimeData;
            memcpy(&SetStartTimeData, SocpRequest.pData, sizeof(SetStartTimeData));
            pRspRequest->ucRspCode = SOCP_RSP_SUCCESS;
            cgms_update_sst_and_time_zone(SetStartTimeData.usYear, SetStartTimeData.ucMonth, SetStartTimeData.ucDay, SetStartTimeData.ucHour, SetStartTimeData.ucMinute, SetStartTimeData.ucSecond, SetStartTimeData.ucTimeZone, SetStartTimeData.ucDataSaveingTime);
            break;
        }
#endif
        // 保存参数
        case SOCP_PRM_NO_SAVE_PRM:
        {
            log_i("save prm data");
            cgms_prm_db_write_flash();
            pRspRequest->ucRspCode = SOCP_RSP_SUCCESS;
            break;
        }
        // 未知命令
        default:
        {
            pRspRequest->ucRspCode = SOCP_RSP_INVALID_OPERAND;
            break;
        }
        }
    }
}


/*******************************************************************************
*                           陈苏阳@2024-06-24
* Function Name  :  cgms_socp_read_prm
* Description    :  读取参数
* Input          :  ble_event_info_t BleEventInfo,
* Input          :  ble_socp_datapacket_t SocpRequest
* Input          :  bool bCrcPass
* Output         :  ble_socp_rsp_t * pRspRequest
* Return         :  void
*******************************************************************************/
void cgms_socp_read_prm(__attribute__((unused)) ble_event_info_t BleEventInfo, ble_socp_datapacket_t  SocpRequest, bool bCrcPass, ble_socp_rsp_t* pRspRequest)
{
    if (SocpRequest.ucDataLen > 4)
    {
        pRspRequest->ucRspCode = SOCP_RSP_INVALID_OPERAND;
    }
    else
    {
        uint8_t ucIndex = 0;
        uint8_t ucPrmNo = 0;
        ucPrmNo = SocpRequest.pData[0];
        switch (ucPrmNo)
        {
            // 读启动时间
        case SOCP_PRM_NO_READ_START_TIME:
        {
            pRspRequest->ucOpCode = SOCP_READ_PRM_RESPONSE;
            *(pRspRequest->ucRespVal) = ucPrmNo;
            ucIndex += 1;
            uint16_encode(g_mSST.date_time.time_info.year, pRspRequest->ucRespVal + ucIndex);
            ucIndex += 2;
            *(pRspRequest->ucRespVal + ucIndex) = g_mSST.date_time.time_info.month;
            ucIndex += 1;
            *(pRspRequest->ucRespVal + ucIndex) = g_mSST.date_time.time_info.day;
            ucIndex += 1;
            *(pRspRequest->ucRespVal + ucIndex) = g_mSST.date_time.time_info.hour;
            ucIndex += 1;
            *(pRspRequest->ucRespVal + ucIndex) = g_mSST.date_time.time_info.minute;
            ucIndex += 1;
            *(pRspRequest->ucRespVal + ucIndex) = g_mSST.date_time.time_info.sec;
            ucIndex += 1;
            *(pRspRequest->ucRespVal + ucIndex) = g_mSST.date_time.time_zone;
            ucIndex += 1;
            *(pRspRequest->ucRespVal + ucIndex) = g_mSST.date_time.time_info.month;
            ucIndex += 1;
            *(pRspRequest->ucRespVal + ucIndex) = g_mSST.dst;
            ucIndex += 1;
            pRspRequest->ucSizeVal = ucIndex;
            break;
        }
        default:
        {
            pRspRequest->ucRspCode = SOCP_RSP_INVALID_OPERAND;
            break;
        }
        }
    }
}


/*******************************************************************************
*                           陈苏阳@2024-10-11
* Function Name  :  cgms_socp_get_history_data
* Description    :  获取历史数据
* Input          :  ble_event_info_t BleEventInfo,
* Input          :  ble_socp_datapacket_t SocpRequest
* Input          :  bool bCrcPass
* Output         :  ble_socp_rsp_t * pRspRequest
* Return         :  void
*******************************************************************************/
void cgms_socp_get_history_data(__attribute__((unused)) ble_event_info_t BleEventInfo, ble_socp_datapacket_t  SocpRequest, bool bCrcPass, ble_socp_rsp_t* pRspRequest)
{
    // 效验命令的CRC
    if (!bCrcPass)
    {
        log_e("crc error");
        // CRC错误
        pRspRequest->ucRspCode = SOCP_GET_HISTORY_DATA_RSP_CODE_OTHER_ERR;
        return;
    }

    // 填充结构体
    ble_cgms_socp_get_history_data_datapacket_t SocpGetHistoryDataDatapacket;
    memcpy(&SocpGetHistoryDataDatapacket, SocpRequest.pData, sizeof(SocpGetHistoryDataDatapacket));

    log_i("racp_findMeasDB %d/%d", SocpGetHistoryDataDatapacket.usStartIndex, SocpGetHistoryDataDatapacket.usStopIndex);
    
    // 历史数据索引&提取
    if (racp_findMeasDB(RACP_OPERATOR_RANGE, SocpGetHistoryDataDatapacket.usStartIndex, SocpGetHistoryDataDatapacket.usStopIndex))
    {

        pRspRequest->ucRspCode = SOCP_GET_HISTORY_DATA_RSP_CODE_SUCCESS;

        // 清空发送累计
        app_global_get_app_state()->RecordOptInfo.usRacpRecordSendCnt = 0;

        // 记录本次历史数据操作的BLE事件信息
        app_global_get_app_state()->RecordOptInfo.BleEventInfo = BleEventInfo;

        // 开始发送数据记录
        app_glucose_meas_record_send_start();
        return;

    }
    else
    {
        log_w("RACP_RESPONSE_RESULT_NO_RECORDS_FOUND");
        pRspRequest->ucRspCode = SOCP_GET_HISTORY_DATA_RSP_CODE_OTHER_ERR;
        return;
    }
}

/*******************************************************************************
*                           陈苏阳@2023-10-18
* Function Name  :  on_socp_value_write
* Description    :  SOCP收到数据
* Input          :  ble_event_info_t BleEventInfo
* Input          :  uint16_t usLen
* Input          :  uint8_t* pData
* Output         :  None
* Return         :  void
*******************************************************************************/
void on_socp_value_write(ble_event_info_t BleEventInfo, uint16_t usLen, uint8_t* pData)
{
    ble_socp_datapacket_t  SocpRequest;
    ble_socp_rsp_t  RspRequest;

#if ((USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL) ||(USE_BLE_PROTOCOL==GN_2_PROTOCOL))
    uint8_t ucTempDatapacketBuffer[16];

    if (cgms_socp_check_production_cmd(pData, usLen))
    {
        log_d("is production cmd.");
        g_bProduction = true;
    }
    else
    {
        // 如果不是生产测试命令

        // 如果设置了密码
        if (att_get_feature()->ucPasswordExist)
        {
            log_d("password is seted.");
            // 走解密流程(同时后续正常交互的命令也在这边走解密,会直接修改BLE接收buffer)
            cgms_aes128_decrpty(pData, ucTempDatapacketBuffer);
            memcpy(pData, ucTempDatapacketBuffer, 16);

            // 如果当前的命令不是密码验证命令,且当前密码还未验证成功
            if ((pData[0] != SOCP_VERIFY_PWD) && (app_global_get_app_state()->bCgmsPwdVerifyOk == false))
            {
                // 返回操作非法回应包
                RspRequest.ucOpCode = SOCP_RESPONSE_ILLEGAL_CODE;
                RspRequest.ucReqOpcode = 0X00;
                RspRequest.ucRspCode = 0X00;
                RspRequest.ucSizeVal = 0;
                socp_send(BleEventInfo, RspRequest);
                return;
            }
            // 如果是密码验证命令
            else if (pData[0] == SOCP_VERIFY_PWD)
            {
                elog_hexdump("datapacket", 8, pData, 16);
                // 拷贝数据包中的密码
                uint16_t usPassword;
                memcpy(&usPassword, &pData[2], 2);
                // 效验密码
                if (usPassword == g_usBleProtocolPassword)
                {
                    app_global_get_app_state()->bCgmsPwdVerifyOk = true;
                    log_i("password verify pass");
                    RspRequest.ucOpCode = SOCP_RESPONSE_CODE;
                    RspRequest.ucReqOpcode = SOCP_VERIFY_PWD;
                    RspRequest.ucRspCode = 0X01;
                    RspRequest.ucSizeVal = 0;
                    socp_send(BleEventInfo, RspRequest);
                    return;
                }
                else
                {
                    app_global_get_app_state()->bCgmsPwdVerifyOk = false;
                    log_e("password verify fail");
                    RspRequest.ucOpCode = SOCP_RESPONSE_CODE;
                    RspRequest.ucReqOpcode = SOCP_VERIFY_PWD;
                    RspRequest.ucRspCode = 0X04;
                    RspRequest.ucSizeVal = 0;
                    socp_send(BleEventInfo, RspRequest);
                    return;
                }
            }
        }
        // 如果还没设置密码
        else
        {
            log_d("no valid password");
            // 如果也不是设置密码命令
            if ((pData[0] != SOCP_SET_PWD))
            {
                RspRequest.ucOpCode = SOCP_RESPONSE_ILLEGAL_CODE;
                RspRequest.ucReqOpcode = 0X00;
                RspRequest.ucRspCode = 0X00;
                RspRequest.ucSizeVal = 0;
                socp_send(BleEventInfo, RspRequest);
                return;
            }
            else
            {
                // 如果是设置密码命令

                // 长度不正确,返回报错
                if (usLen != 17)
                {
                    log_d("Len is Err");
                    RspRequest.ucOpCode = SOCP_RESPONSE_CODE;
                    RspRequest.ucReqOpcode = SOCP_SET_PWD;
                    RspRequest.ucRspCode = 0X03;
                    RspRequest.ucSizeVal = 0;
                    socp_send(BleEventInfo, RspRequest);
                    return;
                }
                else
                {
                    // 对数据包的数据部分进行解密
                    cgms_aes128_decrpty(&pData[1], ucTempDatapacketBuffer);
                    memcpy(&pData[1], ucTempDatapacketBuffer, 16);
                    elog_hexdump("datapacket decode", 8, pData, 16);

                    // 拷贝SN
                    uint8_t ucSn[8];
                    memset(ucSn, 0x00, sizeof(ucSn));
                    memcpy(ucSn, &pData[3], 7);

                    // 获取发射器存储的SN
                    uint8_t uc_PrmSn[15];
                    memset(uc_PrmSn, 0x00, sizeof(uc_PrmSn));
                    cgms_prm_get_sn(uc_PrmSn);
                    if (strcmp(ucSn, &uc_PrmSn[3]) == 0)
                    {
                        // 拷贝密码
                        memcpy(&g_usBleProtocolPassword, &pData[1], 2);

                        // 更新BLE CGM服务中的feature特征
                        att_get_feature()->ucPasswordExist = 1;
                        att_update_feature_char_data_crc();
                        // 本次BLE连接就算密码验证通过
                        app_global_get_app_state()->bCgmsPwdVerifyOk = true;
                        log_i("Set Password OK,SN:%s  Password:0x%04X", (char*)ucSn, g_usBleProtocolPassword);
                        RspRequest.ucOpCode = SOCP_RESPONSE_CODE;
                        RspRequest.ucReqOpcode = SOCP_SET_PWD;
                        RspRequest.ucRspCode = 0X01;
                        RspRequest.ucSizeVal = 0;
                        socp_send(BleEventInfo, RspRequest);

                        // 更新AES库的key,后续通讯使用新key
                        cgms_aes128_update_key((uint8_t*)&g_usBleProtocolPassword);
                        return;
                    }
                    else
                    {
                        log_e("SN Err,%s!=%s", &uc_PrmSn[3], ucSn);
                        RspRequest.ucOpCode = SOCP_RESPONSE_CODE;
                        RspRequest.ucReqOpcode = SOCP_SET_PWD;
                        RspRequest.ucRspCode = 0X05;
                        RspRequest.ucSizeVal = 0;
                        socp_send(BleEventInfo, RspRequest);
                        return;
                    }

                }
            }
        }
    }
#endif

    bool bCrcPassFlag = false;

    // 解码SOCP数据包并填充结构体
    ble_socp_decode(usLen, pData, &SocpRequest);
    elog_hexdump("socp_rav", 8, pData, usLen);
    log_i("socp_request.opcode:%d", SocpRequest.ucOpCode);

    // 效验命令的CRC
    if (do_crc(pData, usLen) != 0)
    {
        bCrcPassFlag = true;
    }

    RspRequest.ucOpCode = SOCP_RESPONSE_CODE;
    RspRequest.ucReqOpcode = SocpRequest.ucOpCode;
    RspRequest.ucRspCode = SOCP_RSP_OP_CODE_NOT_SUPPORTED;
    RspRequest.ucSizeVal = 0;

    // 根据命令类型来做相应处理
    switch (SocpRequest.ucOpCode)
    {
        // 开始CGM
    case SOCP_START_THE_SESSION:
    {
        cgms_socp_start_the_session(BleEventInfo, SocpRequest, bCrcPassFlag, &RspRequest);
        break;
    }
    // 写入传感器code
    case SOCP_SENSOR_CODE:
    {
        cgms_socp_write_sensor_code(BleEventInfo, SocpRequest, bCrcPassFlag, &RspRequest);
        break;
    }
    // 触发固件升级
    case SOCP_START_FOTA:
    {
        cgms_socp_start_fota(BleEventInfo, SocpRequest, bCrcPassFlag, &RspRequest);
        break;
    }
    // 读取复位原因寄存器
    case SOCP_READ_RESET_REG:
    {
        cgms_socp_read_reset_reg(BleEventInfo, SocpRequest, bCrcPassFlag, &RspRequest);
        break;
    }
    // 读取硬错误信息
    case SOCP_READ_HARD_FAULT_INFO:
    {
        cgms_socp_read_hard_fault_info(BleEventInfo, SocpRequest, bCrcPassFlag, &RspRequest);
        break;
    }
    // 停止CGM
    case SOCP_STOP_THE_SESSION:
    {
        cgms_socp_stop_the_session(BleEventInfo, SocpRequest, bCrcPassFlag, &RspRequest);
        break;
    }
    // 写入血糖校准值
    case SOCP_WRITE_GLUCOSE_CALIBRATION_VALUE:
    {
        cgms_socp_write_glucose_calibration_value(BleEventInfo, SocpRequest, bCrcPassFlag, &RspRequest);
        break;
    }
    // 开始ADC校准
    case SOCP_START_AD_CALI:
    {
        cgms_socp_start_ad_cali(BleEventInfo, SocpRequest, bCrcPassFlag, &RspRequest);
        break;
    }
    // 读取ADC校准时的数据
    case SOCP_READ_AD_CALI_DATA:
    {
        cgms_socp_read_ad_cali_data(BleEventInfo, SocpRequest, bCrcPassFlag, &RspRequest);
        break;
    }
    // 写入参数
    case SOCP_WRITE_PRM:
    {
        cgms_socp_write_prm(BleEventInfo, SocpRequest, bCrcPassFlag, &RspRequest);
        break;
    }
    // 读取参数
    case SOCP_READ_PRM:
    {
        cgms_socp_read_prm(BleEventInfo, SocpRequest, bCrcPassFlag, &RspRequest);
        break;
    }
#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL) 
    // 获取历史数据
    case SOCP_GET_HISTORY_DATA:
    {
        cgms_socp_get_history_data(BleEventInfo, SocpRequest, bCrcPassFlag, &RspRequest);
        break;
    }
#endif
    default:
    {
        RspRequest.ucRspCode = SOCP_RSP_OP_CODE_NOT_SUPPORTED;
        break;
    }
    }

    socp_send(BleEventInfo, RspRequest);
}


/*******************************************************************************
*                           陈苏阳@2023-10-18
* Function Name  :  cgms_socp_check_production_cmd
* Description    :  检查是否为生产命令
* Input          :  uint8_t * pData
* Input          :  uint16_t usLen
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool cgms_socp_check_production_cmd(uint8_t* pData,__attribute__((unused))  uint16_t usLen)
{
    switch (pData[0])
    {
    // 如果是写参数命令
    case SOCP_WRITE_PRM:
    {
        switch (pData[1])
        {
            // 以下操作为生产操作
            case SOCP_PRM_NO_WRITE_OR_READ_SN:
            case SOCP_PRM_WRITE_AFE_VOL_OFFISET:
            case SOCP_PRM_WRITE_AFE_COEFFICIENT_K:
            case SOCP_PRM_WRITE_AFE_COEFFICIENT_B:
            case SOCP_PRM_NO_SAVE_PRM:
                return true;
            default:
                break;
        }
        break;
    }
    // 如果是读参数命令
    case SOCP_READ_PRM:
    {
        // 以下操作为生产操作
        switch (pData[1])
        {
        case SOCP_PRM_NO_WRITE_OR_READ_SN:
            return true;
        default:
            break;
        }
        break;
    }

    // 如果是校准ADC命令
    case SOCP_START_AD_CALI:
    // 如果是读取校准ADC时的数据命令
    case SOCP_READ_AD_CALI_DATA:
    // 如果是触发进入FOTA模式命令
    case SOCP_START_FOTA:
    // 如果是读取复位原因寄存器命令
    case SOCP_READ_RESET_REG:
    // 如果是读取硬错误信息命令
    case SOCP_READ_HARD_FAULT_INFO:
        // 按生产操作返回
        return true;
    default:
        break;
    }
    return false;
}


/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/
