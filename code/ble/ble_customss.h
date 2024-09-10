/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  ble_customss.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  19/10/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BLE_CUSTOMSS_H
#define __BLE_CUSTOMSS_H

/* Includes ------------------------------------------------------------------*/
#include "app_global.h"
/* Private define ------------------------------------------------------------*/


/* 服务UUID */
#define BLE_CGM_SERVICE_SVC_UUID                                    {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0x1F,0x18,0x00,0x00}
#define BLE_CGM_SERVICE_CHARACTERISTIC_CGM_MEASUREMENT_UUID         {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0xA7,0x2A,0x00,0x00}
#define BLE_CGM_SERVICE_CHARACTERISTIC_CGM_SPECIFIC_OPS_UUID        {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0xAC,0x2A,0x00,0x00}
#define BLE_CGM_SERVICE_CHARACTERISTIC_CGM_FEATURE_UUID             {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0xA8,0x2A,0x00,0x00}
#define BLE_CGM_SERVICE_CHARACTERISTIC_CGM_STATE_UUID               {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0xA9,0x2A,0x00,0x00}
#define BLE_CGM_SERVICE_CHARACTERISTIC_CGM_SESSION_START_TIME_UUID  {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0xAA,0x2A,0x00,0x00}
#define BLE_CGM_SERVICE_CHARACTERISTIC_CGM_SESSION_RUN_TIME_UUID    {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0xAB,0x2A,0x00,0x00}
#define BLE_CGM_SERVICE_CHARACTERISTIC_RECORD_ACCESS_CONTROL_UUID   {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0x52,0x2A,0x00,0x00}
#define BLE_CGM_SERVICE_SVC_LOG_UUID                                {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0xAA,0x00,0x00,0x00}
#define BLE_CGM_SERVICE_CHARACTERISTIC_LOG_TX_UUID                  {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0xAA,0x01,0x00,0x00}
#define BLE_CGM_SERVICE_CHARACTERISTIC_LOG_STATUS_UUID              {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,0x00,0x10,0x00,0x00,0xAA,0x02,0x00,0x00}

/* char 数据buffer长度 */
#define BLE_CGM_SERVICE_CGM_MEASUREMENT_MAX_LENGTH                  16
#define BLE_CGM_SERVICE_CGM_SPECIFIC_OPS_MAX_LENGTH                 20
#define BLE_CGM_SERVICE_USER_DESCRIPTION_MAX_LENGTH                 20
#define BLE_CGM_SERVICE_CGM_FEATURE_MAX_LENGTH                      8
#define BLE_CGM_SERVICE_CGM_SESSION_START_TIME_MAX_LENGTH           20
#define BLE_CGM_SERVICE_CGM_SESSION_RUN_TIME_LENGTH                 2
#define BLE_CGM_SERVICE_CGM_STATE_MAX_LENGTH                        5
#define BLE_CGM_SERVICE_RECORD_ACCESS_CONTROL_MAX_LENGTH            20
#define BLE_CGM_SERVICE_CCCD_VALUE_MAX                              2

/* char 名称 */
#define BLE_CGM_SERVICE_CGM_MEASUREMENT_CHARACTERISTIC_NAME                  "CGM_MEASUREMENT"
#define BLE_CGM_SERVICE_CGM_SPECIFIC_OPS_CHARACTERISTIC_NAME                 "CGM_SPECIFIC_OPS"
#define BLE_CGM_SERVICE_CGM_FEATUREG_CHARACTERISTIC_NAME                     "CGM_FEATUREG"
#define BLE_CGM_SERVICE_CGM_STATE_CHARACTERISTIC_NAME                        "CGM_STATUS"
#define BLE_CGM_SERVICE_CGM_SESSION_START_TIME_CHARACTERISTIC_NAME           "CGM_SESSION_START_TIME"
#define BLE_CGM_SERVICE_CGM_SESSION_RUN_TIME_CHARACTERISTIC_NAME             "CGM_SESSION_RUN_TIME"
#define BLE_CGM_SERVICE_RECORD_ACCESS_CONTROL_CHARACTERISTIC_NAME            "RECORD_ACCESS_CONTROL"

/* Private typedef -----------------------------------------------------------*/

enum cs_att
{
    /* Service 0 */
    CS_SERVICE0,                                    // handle 18

    /* CGMS measurement*/
    CS_IDX_CGM_MEASUREMENT_CHAR,                    // handle 19
    CS_IDX_CGM_MEASUREMENT_VAL,                     // handle 20
    CS_IDX_CGM_MEASUREMENT_CCC,                     // handle 21

    /* Feature*/
    CS_IDX_CGM_FEATURE_CHAR,                        // handle 22
    CS_IDX_CGM_FEATURE_VAL,                         // handle 23

    /*  CGMS Status*/
    CS_IDX_CGM_STATE_CHAR,                          // handle 24
    CS_IDX_CGM_STATE_VAL,                           // handle 25
    CS_IDX_CGM_STATE_CCC,                           // handle 26

    /* record access control */
    CS_IDX_RECORD_ACCESS_CONTROL_CHAR,              // handle 27
    CS_IDX_RECORD_ACCESS_CONTROL_VAL,               // handle 28
    CS_IDX_RECORD_ACCESS_CONTROL_CCC,               // handle 29

    //	/*  CGMS Start time */
    CS_IDX_CGM_SESSION_START_TIME_CHAR,             // handle 30
    CS_IDX_CGM_SESSION_START_TIME_VAL,              // handle 31

    //	/*  specific ops control*/
    CS_IDX_CGM_SPECIFIC_OPS_CHAR,                   // handle 32
    CS_IDX_CGM_SPECIFIC_OPS_VAL,                    // handle 33
    CS_IDX_CGM_SPECIFIC_OPS_CCC,                    // handle 34

    /* Max number of services and characteristics */
    CS_NB0,
};

typedef struct
{
    uint8_t ucStartBy;                          // 启动来源标识符(安卓为0x01,iOS为0x02,接收器为0x03)
    uint8_t ucStartByVersion[3];                // 启动用的APP或者接收器的软件版本号
    uint8_t ucPasswordExist;                    // 当前是否被设置过密码
    uint8_t ucCrcSupported;                     // 当前是否支持CRC效验
    uint8_t ucFactoryCalibrationSupported;      // 是否支持工厂校准,0x01表示需要手动校准,默认为0x00
    uint8_t ucWorkTime;                         // 工作时间,如3天,7天,14天,用于计算结束时间,默认0x0E
    uint8_t ucSampleTime;                       // 数据发送间隔3分钟或5分钟,默认0x03
    uint16_t usCrc16;                           // CRC16
}__attribute__((packed)) cgm_feature_char_data_t;


typedef struct
{
    uint16_t usNumberOfReadings;                // 表示当前启动后一共有多少个数据,个数为offset+1,APP根据个数判断缺失的数据的个数和区间.
    uint8_t ucRunStatus;                        // 运行状态(使用cgm_measurement_sensor_state_t内的值)
    uint16_t usFactoryCode;                     // 工厂校准码
    #if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
    uint16_t usCrc16;                           // CRC16
    #endif
}__attribute__((packed)) cgm_status_char_data_t;

typedef struct
{
#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
	uint32_t uiStartTime;                       // 启动时间(UTC时间戳)
    uint32_t uiRunTime;                         // 发射器已经启动运行的时间(单位:S)
#else
    uint16_t usYear;                            // 年
    uint8_t ucMonth;                            // 月
    uint8_t ucDay;                              // 日
    uint8_t ucHour;                             // 小时
    uint8_t ucMinute;                           // 分钟
    uint8_t ucSecond;                           // 秒
    uint8_t ucTimeZone;                         // 时区
    uint8_t ucDaySaveingTime;                   // 夏令时
#endif
    uint16_t usCrc16;                           // CRC16
}__attribute__((packed)) cgm_session_start_time_char_data_t;


typedef struct
{
    uint16_t usRunTime;                         // 运行时间
}__attribute__((packed)) cgm_session_run_time_char_data_t;

struct ble_customss_service_att_database
{
        #if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
	// 仅读取部分
    cgm_feature_char_data_t CgmFeatureValue;
    cgm_status_char_data_t CgmStatusValue;
    cgm_session_start_time_char_data_t CgmSessionStartTimeValue;
	// 读写部分
    uint8_t CgmsMeasurementValue[CS_CGM_MEASUREMENT_MAX_LENGTH];
    uint8_t RecordAccessControlValue[CS_RECORD_ACCESS_CONTROL_MAX_LENGTH];
    uint8_t CgmSpecificOpsValue[CS_CGM_SPECIFIC_OPS_MAX_LENGTH];
    // 带Notify的char的CCCD
    uint8_t CgmMeasurementCccdValue[CGMS_CCCD_VALUE_MAX];
    uint8_t CgmRecordAccessControlCccdValue[CGMS_CCCD_VALUE_MAX];
    uint8_t CgmSpecificOpsCccdValue[CGMS_CCCD_VALUE_MAX];
    uint8_t CgmStatusCccdValue[CGMS_CCCD_VALUE_MAX];
	#elif (USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL)
	// 仅读取部分
    cgm_feature_char_data_t CgmFeatureValue;
	cgm_session_run_time_char_data_t CgmSessionRunTimeValue;
	cgm_status_char_data_t CgmStatusValue;
    cgm_session_start_time_char_data_t CgmSessionStartTimeValue;
    // 读写部分
    uint8_t CgmsMeasurementValue[BLE_CGM_SERVICE_CGM_MEASUREMENT_MAX_LENGTH];
    uint8_t CgmsRecordAccessControlValue[BLE_CGM_SERVICE_RECORD_ACCESS_CONTROL_MAX_LENGTH];
    uint8_t CgmSpecificOpsValue[BLE_CGM_SERVICE_CGM_SPECIFIC_OPS_MAX_LENGTH];
    // 带Notify的char的CCCD
    uint8_t CgmMeasurementCccdValue[BLE_CGM_SERVICE_CCCD_VALUE_MAX];
    uint8_t CgmRecordAccessControlCccdValue[BLE_CGM_SERVICE_CCCD_VALUE_MAX];
    uint8_t CgmSpecificOpsCccdValue[BLE_CGM_SERVICE_CCCD_VALUE_MAX];
    uint8_t CgmStatusCccdValue[BLE_CGM_SERVICE_CCCD_VALUE_MAX];
	#elif (USE_BLE_PROTOCOL==P3_PROTOCOL)
	cgm_session_run_time_char_data_t CgmSessionRunTimeValue;
	cgm_status_char_data_t CgmStatusValue;
    cgm_session_start_time_char_data_t CgmSessionStartTimeValue;
	uint8_t CgmFeatureValue[18];
    // 读写部分
    uint8_t CgmsMeasurementValue[BLE_CGM_SERVICE_CGM_MEASUREMENT_MAX_LENGTH];
    uint8_t CgmsRecordAccessControlValue[BLE_CGM_SERVICE_RECORD_ACCESS_CONTROL_MAX_LENGTH];
    uint8_t CgmSpecificOpsValue[BLE_CGM_SERVICE_CGM_SPECIFIC_OPS_MAX_LENGTH];
    // 带Notify的char的CCCD
    uint8_t CgmMeasurementCccdValue[BLE_CGM_SERVICE_CCCD_VALUE_MAX];
    uint8_t CgmRecordAccessControlCccdValue[BLE_CGM_SERVICE_CCCD_VALUE_MAX];
    uint8_t CgmSpecificOpsCccdValue[BLE_CGM_SERVICE_CCCD_VALUE_MAX];
    uint8_t CgmStatusCccdValue[BLE_CGM_SERVICE_CCCD_VALUE_MAX];
	#endif
};



/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void att_initialize(void);
void att_set_start_handle(uint16_t handle);
uint16_t att_get_start_handle(void);
cgm_status_char_data_t* att_get_cgm_status(void);


#if (USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL) 
void att_update_feature_char_data_crc(void);
cgm_feature_char_data_t* att_get_feature(void);
#elif (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
void att_update_start_time_char_data_crc(void);
void att_update_cgm_status_char_data_crc(void);
cgm_feature_char_data_t* att_get_feature(void);
void att_update_feature_char_data_crc(void);
#endif
cgm_session_start_time_char_data_t* att_get_start_time(void);
void att_update_start_time_char_data(void);
void att_update_cgm_status_char_data(void);
uint16_t att_get_att_handle(uint16_t attindx);
cgm_session_run_time_char_data_t* att_get_run_time(void);
void att_update_notify_indication(uint16_t handle, uint16_t value);
void att_disable_notify_indication(void);
#endif /* __BLE_CUSTOMSS_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/

