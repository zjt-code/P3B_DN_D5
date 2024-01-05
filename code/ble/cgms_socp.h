/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_socp.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  27/3/2023
* Description        :
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CGMS_SOCP_H
#define __CGMS_SOCP_H

/* Includes ------------------------------------------------------------------*/
#include "app_global.h"
/* Private define ------------------------------------------------------------*/



/* Private typedef -----------------------------------------------------------*/

// SOCP回应数据包结构体
typedef struct
{
    uint8_t ucOpCode;                               // 操作码
    uint8_t ucReqOpcode;                            // 此回应包的原始请求包的操作码
    uint8_t ucRspCode;                              // 回应码
    uint8_t ucRespVal[20];                          // 回应数据
    uint8_t ucSizeVal;                              // 回应数据长度
}__attribute__((packed)) ble_socp_rsp_t;


// SOCP数据包结构体
typedef struct
{
    uint8_t   ucOpCode;                             // 操作码
    uint8_t   ucDataLen;                            // 所携带数据的长度
    uint8_t* pData;                                 // 所携带数据的指针
}__attribute__((packed)) ble_cgms_socp_datapacket_t;


// SOCP数据包_启动发射器命令包_结构体
typedef struct
{
    uint8_t ucOpCode;                               // 操作码
    uint32_t uiStartTime;                           // 启动时间
    uint16_t usFactoryCode;                         // 工厂校准码.
    uint8_t ucTimeZone;                             // 时区
    uint8_t ucReserved1;                            // 保留1
    uint8_t ucFrom;                                 // 启动来源标识符(安卓为0x01,iOS为0x02,接收器为0x03)
    uint8_t ucSoftwareVersion[3];                   // 启动用的APP或者接收器的软件版本号
    uint16_t usCRC16;                               // CRC16
}__attribute__((packed)) ble_cgms_socp_start_the_session_datapacket_t;


// SOCP数据包_输入参比血糖命令包_结构体
typedef struct
{
    uint8_t ucOpCode;                               // 操作码
    uint16_t usCalibration;                         // 用户输入的参比血糖,转化为mmol/L的单位,取1位小数并乘以10
    uint16_t usOffset;                              // 该条参比血糖发送的时候,对应的前一个血糖广播数据的序列号
    uint16_t usCRC16;                               // CRC16
}__attribute__((packed)) ble_cgms_socp_write_glucose_calibration_datapacket_t;


// SOCP数据包_写入参数_ProNo=1的命令包_结构体
typedef struct
{
    uint8_t ucPrmNo;
    int16_t sPrmVD1;
    int16_t sPrmRL1;
    int16_t sPrmOffset;
    int16_t sPrmAD1;
    int16_t sPrmRL2;
    int16_t sPrmAD2;
}__attribute__((packed)) ble_cgms_socp_write_prm_type_1_t;


// SOCP数据包_写入参数_ProNo=3的命令包_结构体
typedef struct
{
    uint8_t ucPrmNo;
    uint8_t ucPrmDX0;
    uint8_t ucPrmDX1;
    uint8_t ucPrmDX2;
    uint8_t ucPrmDX3;
    uint8_t ucPrmDX4;
    uint8_t ucPrmDX5;
    uint8_t ucPrmDX6;
    uint8_t ucPrmDX7;
    uint8_t ucPrmDX8;
    uint8_t ucPrmDX9;
    uint8_t ucPrmDXA;
    uint8_t ucPrmDXB;
    uint16_t usPrmDXC;
}__attribute__((packed)) ble_cgms_socp_write_prm_type_3_t;



// SOCP数据包_写入参数_ProNo=4的命令包_结构体
typedef struct
{
    uint8_t ucPrmNo;
    uint8_t ucPrmWMY[3];
    uint16_t usSN;
}__attribute__((packed)) ble_cgms_socp_write_prm_type_4_t;


// SOCP数据包_设置密码命令包_结构体
typedef struct
{
    uint8_t ucOpCode;                               // 操作码
    uint16_t usPassword;                            // 密码
    uint8_t ucSn[11];                               // 发射器SN
    uint16_t usCRC16;                               // CRC16
}__attribute__((packed)) ble_cgms_socp_set_password_datapacket_t;


// SOCP数据包_验证密码命令包_结构体
typedef struct
{
    uint8_t ucOpCode;                               // 操作码
    uint8_t ucOperation;                            // 缓冲数值
    uint16_t usPassword;                            // 密码
    uint16_t usCRC16;                               // CRC16
}__attribute__((packed)) ble_cgms_socp_verify_password_datapacket_t;



// SOCP数据包_写入参数_参数类型
typedef enum
{
    BLE_CGMS_SOCP_WRITE_PRM_WRITE_CAL_PARA = 0x01,
    BLE_CGMS_SOCP_WRITE_PRM_WRITE_ALGORITHM_COFF = 0x03,
    BLE_CGMS_SOCP_WRITE_PRM_WRITE_SN = 0x04,
    BLE_CGMS_SOCP_WRITE_PRM_WRITE_PARA_TO_FLASH = 0xFE,
}ble_cgms_socp_write_prm_type_t;

typedef enum
{
    SOCP_OPCODE_RESERVED = 0x00,                                       // 保留,暂未使用
    SOCP_WRITE_CGM_COMMUNICATION_INTERVAL = 0x01,                      // 写入CGM通讯间隔
    SOCP_READ_CGM_COMMUNICATION_INTERVAL = 0x02,                       // 读取CGM通讯间隔
    SOCP_READ_CGM_COMMUNICATION_INTERVAL_RESPONSE = 0x03,              // 读取CGM通讯间隔响应包
    SOCP_WRITE_GLUCOSE_CALIBRATION_VALUE = 0x04,                       // 写入血糖校准值
    SOCP_READ_GLUCOSE_CALIBRATION_VALUE = 0x05,                        // 读取血糖校准值
    SOCP_READ_GLUCOSE_CALIBRATION_VALUE_RESPONSE = 0x06,               // 读取血糖校准值响应包
    SOCP_WRITE_PATIENT_HIGH_ALERT_LEVEL = 0x07,                        // 写入病人高血糖报警等级
    SOCP_READ_PATIENT_HIGH_ALERT_LEVEL = 0x08,                         // 读取病人高血糖报警等级
    SOCP_READ_PATIENT_HIGH_ALERT_LEVEL_RESPONSE = 0x09,                // 读取病人高血糖报警等级响应包
    SOCP_WRITE_PATIENT_LOW_ALERT_LEVEL = 0x0A,                         // 写入病人低血糖报警等级
    SOCP_READ_PATIENT_LOW_ALERT_LEVEL = 0x0B,                          // 读取病人低血糖报警等级
    SOCP_READ_PATIENT_LOW_ALERT_LEVEL_RESPONSE = 0x0C,                 // 读取病人低血糖报警等级响应包
    SOCP_SET_HYPO_ALERT_LEVEL = 0x0D,                                  // 设置低血糖警报级别低度警报级别值，单位为mg/dL。此控制点的响应为响应代码
    SOCP_GET_HYPO_ALERT_LEVEL = 0x0E,                                  // 获取低血糖警报级别-对该控制点的正常响应是操作码0x0F。对于错误条件，响应是响应代码
    SOCP_START_THE_SESSION = 0x1A,                                     // 开始新的CGM测量周期
    SOCP_STOP_THE_SESSION = 0x1B,                                      // 停止当前的CGM测量周期
    SOCP_SENSOR_CODE = 0x1D,                                           // 设置传感器Code
    SOCP_START_FOTA = 0xFF,                                            // 触发进入FOTA模式
    SOCP_READ_RESET_REG = 0xFE,                                        // 读取复位原因寄存器
    SOCP_READ_HARD_FAULT_INFO = 0xFD,                                  // 读取硬错误信息
    SOCP_READ_PRM_RESPONSE = 0x60,                                     // 读取参数响应包
    SOCP_WRITE_PRM = 0x61,                                             // 写入参数
    SOCP_READ_PRM = 0x62,                                              // 读取参数
    SOCP_START_AD_CALI = 0x63,                                         // 校准ADC
    SOCP_WRITE_BOND_SECU = 0x65,                                       // 安全绑定
    SOCP_NOMAL_AUTH = 0x66,                                            // 安全验证
    SOCP_SET_PASSWORD = 0x7A,                                          // 设置密码
    SOCP_VERIFY_PWD = 0X7B,                                            // 验证密码
}socp_opcode_t;


// SOCP回应包的回应码
typedef enum
{
    SOCP_RESPONSE_CODE = 0x1C,                                         // 回应码
    SOCP_RESPONSE_ILLEGAL_CODE = 0X1D,                                 // 操作非法回应码
}socp_response_code_t;

// SOCP 启动CGM命令返回值
typedef enum
{
    SOCP_START_THE_SESSION_RSP_CODE_SUCCESS = 0x01,                    // 启动成功
    SOCP_START_THE_SESSION_RSP_CODE_SENSOR_CODE_ERR = 0x02,            // 传感器Code错误
    SOCP_START_THE_SESSION_RSP_CODE_COMMAND_LEN_ERR = 0x03,            // 命令长度不正确
    SOCP_START_THE_SESSION_RSP_CODE_IS_STARTED = 0x04,                 // 发射器已经被启动,不能再次启动
    SOCP_START_THE_SESSION_RSP_CODE_IS_STOPED = 0x05,                  // 发射器已经停止,不能再次启动
    SOCP_START_THE_SESSION_RSP_CODE_IS_END = 0x06,                     // 监测周期已结束,不能再次启动
    SOCP_START_THE_SESSION_RSP_CODE_CRC_ERR = 0x07,                    // CRC错误
    SOCP_START_THE_SESSION_RSP_COD_UNKNOWN_REASON = 0x08,              // 未知原因导致无法启动
}socp_start_the_session_rsp_code_t;

// SOCP 停止CGM命令返回值
typedef enum
{
    SOCP_STOP_THE_SESSION_RSP_CODE_SUCCESS = 0x01,                     // 停止成功
    SOCP_STOP_THE_SESSION_RSP_CODE_COMMAND_FORMAT_ERR = 0x02,          // 命令格式不正确
    SOCP_STOP_THE_SESSION_RSP_CODE_COMMAND_LEN_ERR = 0x3,              // 命令长度不正确
    SOCP_STOP_THE_SESSION_RSP_CODE_IS_STOPED = 0x04,                   // 该发射器未被启动或者已经停止
    SOCP_STOP_THE_SESSION_RSP_CODE_UNKNOWN_REASON = 0x05,              // 未知原因导致无法停止
}socp_stop_the_session_rsp_code_t;


// SOCP 输入参比血糖命令返回值
typedef enum
{
    SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_SUCCESS = 0x01,            // 校准成功
    SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_COMMAND_FORMAT_ERR = 0x02, // 命令格式不正确
    SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_COMMAND_LEN_ERR = 0x03,    // 命令长度不正确
    SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_CRC_ERR = 0x04,            // 校准不被接受,CRC错误
    SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_WARM_UP = 0x05,            // 极化中,无法校准
    SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_SENSOR_ABNORMAL = 0x06,    // 传感器异常,稍后校准
    SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_IS_END = 0x07,             // 监测结束,无法校准
    SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_RANGE_OUT = 0x08,          // 超出量程,无法校准
    SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_GLUCOSE_FLUCTUATE = 0x09,  // 当前血糖变化快,无法校准
}socp_write_glucose_calibration_rsp_code_t;

// SOCP 通用返回值
typedef enum
{
 SOCP_GENERAL_RSP_RESERVED_FOR_FUTURE_USE = 0x00,                      // 保留未来使用
 SOCP_GENERAL_RSP_SUCCESS = 0x01,                                      // 执行成功
 SOCP_GENERAL_RSP_OP_CODE_NOT_SUPPORTED = 0x02,                        // 操作码不支持
 SOCP_GENERAL_RSP_INVALID_OPERAND = 0x03,                              // 无效数据
 SOCP_GENERAL_RSP_PROCEDURE_NOT_COMPLETED = 0x04,                      // 流程未完成
 SOCP_GENERAL_RSP_OUT_OF_RANGE = 0x05                                  // 超出范围
}socp_general_rsp_code_t;



// SOCP 设置密码命令返回值
typedef enum
{
    SOCP_SET_PASSWORD_RSP_CODE_SUCCESS = 0x01,                         // 设置成功
    SOCP_SET_PASSWORD_RSP_CODE_COMMAND_FORMAT_ERR = 0x02,              // 命令格式错误
    SOCP_SET_PASSWORD_RSP_CODE_COMMAND_LEN_ERR = 0x03,                 // 命令长度错误
    SOCP_SET_PASSWORD_RSP_CODE_HAVE_A_PASSWORD = 0x04,                 // 已有密码
    SOCP_SET_PASSWORD_RSP_CODE_SN_ERR = 0x05,                          // SN错误
    SOCP_SET_PASSWORD_RSP_CODE_DATAPACKET_DECODE_ERR = 0x06,           // 数据包解密失败
}socp_set_password_rsp_code_t;

// SOCP 验证密码命令返回值
typedef enum
{
    SOCP_VERIFY_PASSWORD_RSP_CODE_SUCCESS = 0x01,                      // 验证成功
    SOCP_VERIFY_PASSWORD_RSP_CODE_COMMAND_FORMAT_ERR = 0x02,           // 命令格式错误
    SOCP_VERIFY_PASSWORD_RSP_CODE_COMMAND_LEN_ERR = 0x03,              // 命令长度错误
    SOCP_VERIFY_PASSWORD_RSP_CODE_PASSWORD_ERR = 0x04,                 // 密码错误
    SOCP_VERIFY_PASSWORD_RSP_CODE_OTHER_ERR = 0x05                     // 其他错误
}socp_verify_password_rsp_code_t;





/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
void on_socp_value_write(ble_event_info_t BleEventInfo, uint16_t usLen, uint8_t* pData);
void ble_socp_notify_enable(void);
void ble_socp_notify_disable(void);
bool ble_socp_notify_is_enable(void);
#endif /* __CGMS_SOCP_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/
