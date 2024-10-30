/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  app_global.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  23/10/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_GLOBAL_H
#define __APP_GLOBAL_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "stdbool.h"
#include "sl_sleeptimer.h"
/* Private define ------------------------------------------------------------*/


#define SOFT_VER         "0.1.7"              // 软件版本号


/*************************Main Loop事件ID*****************************/


typedef enum
{
 MAIN_LOOP_EVENT_NONE,                              // 无效事件
 MAIN_LOOP_EVENT_BLE_UPDATE_CONNECT_PARAMETERS,     // 更新BLE连接参数事件
 MAIN_LOOP_EVENT_AFE_WAKEUP_TIMER,                  // AFE周期唤醒定时器事件
 MAIN_LOOP_EVENT_AFE_MEASURE_TIMER,                 // AFE触发测量定时器事件
 MAIN_LOOP_EVENT_AFE_CONFIG_AFTER_TIMER,            // AFE配置后处理定时器事件
 MAIN_LOOP_EVENT_AFE_IRQ,                           // AFE中断触发事件
 MAIN_LOOP_EVENT_TEMP_SENSOR_READ_START_TIMER,      // 温度传感器读取开始定时器事件
 MAIN_LOOP_EVENT_TEMP_SENSOR_READ_END_TIMER,        // 温度传感器读取结束定时器事件
 MAIN_LOOP_EVENT_APP_GLUCOSE_MEAS_1MIN_TIMER,       // 应用层血糖测量1分钟定时器事件
 MAIN_LOOP_EVENT_APP_GLUCOSE_MEAS_FIRST_TIMER,      // 应用层第一次血糖测量定时器事件
 MAIN_LOOP_EVENT_APP_GLUCOSE_MEAS_RECORD_SEND_TIMER,// 应用层血糖测量的记录发送定时器事件
 MAIN_LOOP_EVENT_APP_BATTERY_MEAS_TIMER,            // 应用层电量测量定时器事件
 MAIN_LOOP_EVENT_SOCP_START_SESSION_EVENT,          // 开始CGM事件
 MAIN_LOOP_EVENT_SOCP_STOP_SESSION_EVENT,           // 停止CGM事件
 MAIN_LOOP_EVENT_SOCP_WRITE_CGM_COMMUNICATION_INTERVAL_EVENT,   // 写CGM通讯间隔事件

}main_Loop_event_t;
/**********************************************************************/




/*******************************存储部分********************************/
#define CGMS_DB_MAX_RECORDS                         (6720)                                                      // 设备中最大可存储的历史记录条数
#define CGMS_ONE_PAGE_SIZE     	                    256                                                         // Flash中可擦除的最小单位byte数(一般是一个Page的大小)

#define CMGS_ONE_PAGE_REC_NUM                       (CGMS_ONE_PAGE_SIZE / sizeof(one_record_storage_unit_t))    // Flash中一个Page可以存储的历史数据条数

#define NVR0_ADDR                                   0X00080000
#define NVR4_ADDR                                   0x00080400
#define NVR5_ADDR                                   0x00080500
#define NVR6_ADDR                                   0x00080600

#define PRM_DB_ADDR                                 (NVR4_ADDR)
#define PRM_DB_END_ADDR                             (0x00179FFF+(0x1000))                                       // 参数数据存储地址(结束)


#define HARD_FAULT_INFO_ADDR                        (0x0017A000)                                                // hard_fault信息存储地址(开始)
#define HARD_FAULT_INFO_ADDR_END                    (0x0017AFFF)                                                // hard_fault信息存储地址(结束)
#define HARD_FAULT_INFO_PAGE_NUM                    (1)                                                         // hard_fault信息存储区一共拥有的page数量


/**********************************************************************/



/*******************************设置部分********************************/
#define GLUCOSE_MEAS_INTERVAL                       (3*60)             // 血糖测量间隔(单位:秒)
#define APP_EVENT_MAX_NUM                           32                 // 最大的APP事件数量

#define SOCP_SKIP_CRC_CHECK                         1                  // 是否跳过CRC检查
#define SOCP_SKIP_SEC_AUTH_CHECK                    1                  // 是否跳过安全认证检查

#define SOCP_SET_SENSOR_CODE_MAX_ERR_VAL            (1.8f)             // 设置传感器Code时会触发非法Code的最大值边界
#define SOCP_SET_SENSOR_CODE_MIN_ERR_VAL            (0.3f)             // 设置传感器Code时会触发非法Code的最小值边界

/**********************************************************************/



/*******************************BLE通讯部分*****************************/

/* 生产商信息 (onsemi Company ID) */
#define APP_COMPANY_ID                              0x0362
#define APP_COMPLETE_LIST_16_BIT_UUID               0x181F

#define BLE_CONNECT_PARAM_UPDATE_DELAY              10                  // 从连接建立到发起更新连接参数所需的延时时间(S)
#define BLE_NORMAL_INTERVAL_MIN                     72                 // 期望的BLE连接间隔(最小值)(*1.25ms)
#define BLE_NORMAL_INTERVAL_MAX                     100                // 期望的BLE连接间隔(最大值)(*1.25ms)
#define BLE_NORMAL_LATENCY                          7                  // 期望的BLE连接可跳过的包数
#define BLE_NORMAL_TIMEOUT                          500                // 期望的BLE超时时间
#define BLE_MAX_CONNECTED_NUM                       1                  // 设备可以同时被连接的最大数量
#define P3_PROTOCOL									(0)				   // P3通讯协议宏定义
#define P3_ENCRYPT_PROTOCOL						    (1)				   // P3加密协议宏定义
#define GN_2_PROTOCOL						     	(2)				   // GN-2加密协议宏定义
#define USE_BLE_PROTOCOL							(GN_2_PROTOCOL)	   // 使用的BLE通讯协议格式
#define BLE_ADV_NAME_PREFIXES                       "DN\0"             // 蓝牙广播名前缀


/**********************************************************************/

/* Private typedef -----------------------------------------------------------*/

// 事件回调
typedef void (*event_callback_t)(uint32_t uiArg);


// 事件信息结构体
typedef struct
{
    uint32_t uiEventId;
    event_callback_t CallBack;
    uint32_t uiArg;
}event_info_t;


// BLE通知队列序号
typedef enum  __attribute__((__packed__))
{
    CGMS_MEAS_HIS_SEQNUM = 1,
    CGMS_MEAS_REALTIME_SEQNUM = 2,
    CGMS_STATUS_SEQNUM = 3,
    CGMS_SOCP_SEQUM = 4,
    CGMS_RACP_SEQUM = 5,
}ble_notity_seq_num_t;


// 返回值
typedef enum
{
    RET_CODE_SUCCESS = 0x00,
    RET_CODE_FAIL = 0x01,
}ret_code_t;

#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)

// CGM测量传感器状态
typedef enum  __attribute__((__packed__))
{
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_RUNNING = 0x00,                         // CGM运行中
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_STOPPED = 0x01,                         // CGM没有运行
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_COMMAND_STOPPED = 0x02,                 // 由于APP发送停止命令导致的停止
    CGM_MEASUREMENT_SENSOR_STATUS_SENSION_EXPRIED = 0x08,                         // CGM到期停止
    CGM_MEASUREMENT_SENSOR_STATUS_UNEXPECTED_STOP1 = 0x0E,                        // 意外停止(情况1)
    CGM_MEASUREMENT_SENSOR_STATUS_UNEXPECTED_STOP2 = 0x0F,                         // 意外停止(情况2)
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_SENSOR_ABNORMAL = 0x22,                 // 传感器异常,等待恢复(预设3小时)
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_SENSOR_ERROR = 0x23,                    // 传感器错误,请更换
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_INEFFECTIVE_IMPLANTATION = 0x31,        // 无效植入,请更换传感器(数据停止采样,蓝牙广播继续)
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_WARM_UP = 0x33,                         // 极化中
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_WARM_UP_FAIL = 0x34,                    // 极化失败
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_CURRENT_TOO_HIGH = 0x41,                // 大电流
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_CURRENT_TOO_LOW = 0x42,                 // 小电流
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_CV_ERR = 0x43,                          // CV异常
}cgm_measurement_sensor_state_t;

#else
// CGM测量传感器状态
typedef enum  __attribute__((__packed__))
{
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_RUNNING = 0x00,                         // CGM运行中
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_COMMAND_STOPPED = 0x02,                 // 由于APP发送停止命令导致的停止
    CGM_MEASUREMENT_SENSOR_STATUS_SESSION_M3RESET_STOPPED = 0x01,                 // 由于MCU复位导致的停止
    CGM_MEASUREMENT_SENSOR_STATUS_SENSION_EXPRIED = 0x80,                         // CGM到期停止
}cgm_measurement_sensor_state_t;
#endif

// 血糖质量
typedef enum 
{
    CGM_QUALITY_NORMAL = 0x00,                      // 血糖正常
    CGM_QUALITY_LOW = 0x01,                         // 血糖过低
    CGM_QUALITY_HIGH = 0x02,                        // 血糖过高
}cgm_quality_t;


// 血糖趋势
typedef enum  __attribute__((__packed__))
{
    CGM_TREND_INVILD = 0x00,                        // 血糖趋势_无效趋势
    CGM_TREND_FAST_DOWN = 0x01,                     // 血糖趋势_快速下降
    CGM_TREND_DOWN_DOWN = 0x02,                     // 血糖趋势_下降
    CGM_TREND_SLOW_DOWN = 0x03,                     // 血糖趋势_缓慢下降
    CGM_TREND_STABLE = 0x04,                        // 血糖趋势_平稳
    CGM_TREND_SLOW_UP = 0x05,                       // 血糖趋势_缓慢上升
    CGM_TREND_UP_UP = 0x06,                         // 血糖趋势_上升
    CGM_TREND_FAST_UP = 0x07                        // 血糖趋势_快速上升
}cgm_trend_t;


// BLE事件信息
typedef struct
{
    uint8_t ucConidx;                               // BLE连接句柄
    uint16_t usHandle;
    uint16_t usOperation;
}__attribute__((packed)) ble_event_info_t;


// 历史数据操作信息结构体
typedef struct
{
    uint16_t usRacpRecordStartIndex;                // 开始发送的记录index
    uint16_t usRacpRecordEndIndex;                  // 结束发送的记录index
    uint16_t usRacpRecordCnt;                       // 要发送的记录个数
    uint16_t usRacpRecordSendCnt;                   // 已经发送的记录个数
    ble_event_info_t BleEventInfo;                  // 当前操作所使用的BLE事件
}RecordOptInfo_t;


// BLE连接信息结构体
typedef struct
{
    bool bIsConnected;                              // 当前是否连接
    bool bIsUpdateConnectParameter;                 // 是否已经触发更新连接参数
    uint16_t usBleConidx;                           // BLE连接句柄
    uint16_t usConnectInterval;                     // 连接间隔
    uint16_t usConnectLatency;                      // 可跳过的包数
    uint16_t usConnectTimeout;                      // 连接超时时间
    uint64_t ulConenctedTimeCnt;                    // 连接建立时长(S)
}BleConnectInfo_t;

typedef struct
{
    BleConnectInfo_t BleConnectInfo[BLE_MAX_CONNECTED_NUM];            // 当前BLE连接信息数组
    #if (USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL) 
    bool bCgmsPwdVerifyOk;                                             // 当前密码是否验证成功
    uint16_t usPasswordSaved;                                          // 保存的密码
	  #elif (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
    bool bCgmsPwdVerifyOk;                                             // 当前密码是否验证成功
    uint16_t usPasswordSaved;                                          // 保存的密码
    #endif
    bool bSentRacpSuccess;                                             // RACP发送完成标志位
    bool bSentSocpSuccess;                                             // COAP发送完成标志位
    bool bSentMeasSuccess;                                             // MEAS发送完成标志位
    bool bRecordSendFlag;                                              // 当前是否正在发送历史数据标志位
    RecordOptInfo_t RecordOptInfo;                                     // 当前正在运行的历史数据操作信息
    bool bIsSessionStarted;                                            // 当前是否正在运行CGM
    uint16_t usSessionRunTime;                                         // CGM运行时间
    uint16_t usTimeOffset;                                             // 时间下标(相对于会话开始时间的偏移量)
    cgm_measurement_sensor_state_t Status;                             // 传感器状态
    cgm_trend_t CgmTrend;                                              // 当前血糖趋势
    bool isfs;
    uint16_t start_hdl;
}app_state_t;

/* Private variables ---------------------------------------------------------*/
app_state_t* app_global_get_app_state(void);
bool app_global_is_session_runing(void);
bool app_have_a_active_ble_connect(void);
void app_event_ble_connected_callback(uint16_t usConnectionHandle);
void app_event_ble_param_updated_callback(uint16_t usConnectionHandle, uint16_t usConnectInterval, uint16_t usConnectLatency, uint16_t usConnectTimeout);
void app_event_ble_disconnect_callback(uint16_t usConnectionHandle);
void app_init(void);
void event_init(void);
uint8_t event_push(uint32_t uiEventId,uint32_t uiArg);
uint8_t event_add(uint32_t uiEventId, event_callback_t CallBack);
uint8_t event_handler(uint32_t uiEventId);
uint32_t rtc_get_curr_time(void);
void app_global_ota_start(void);
/* Private function prototypes -----------------------------------------------*/

#endif /* __APP_GLOBAL_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/

