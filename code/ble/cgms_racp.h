/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_racp.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  24/10/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CGMS_RACP_H
#define __CGMS_RACP_H

/* Includes ------------------------------------------------------------------*/
#include "app_global.h"
/* Private define ------------------------------------------------------------*/

// 历史数据操作码
typedef enum
{
    RACP_OPCODE_REPORT_RECS = 0x01,                 // 读取历史数据
    RACP_OPCODE_RESPONSE = 0x1C,                    // 回应包
}racp_opcode_t;


typedef enum
{
    RACP_OPERATOR_NULL = 0x0,
    RACP_OPERATOR_ALL = 1,                          // 获取全部历史数据
    RACP_OPERATOR_LESS_OR_EQUAL = 0x2,              // 获取小于或等于提供的index的数据
    RACP_OPERATOR_GREATER_OR_EQUAL = 0x3,           // 获取大于或等于提供的index的数据
    RACP_OPERATOR_RANGE = 0x4,                      // 获取在index(包括在内)范围内的数据
    RACP_OPERATOR_FIRST = 0x5,                      // 获取第一个数据
    RACP_OPERATOR_LAST = 0x6,                       // 获取最后一个数据
}racp_operator_t;


typedef enum
{
    RACP_RESPONSE_RESULT_RESERVED = 0x00,                              // 保留未使用
    RACP_RESPONSE_RESULT_SUCCESS = 0x01,                               // 执行成功
    RACP_RESPONSE_RESULT_COMMAND_FORMAT_ERR = 0x02,                    // 命令格式不正确
    RACP_RESPONSE_RESULT_COMMAND_LEN_ERR = 0x03,                       // 命令长度不正确
    RACP_RESPONSE_RESULT_COMMAND_START_INDEX_RANGE_OUT = 0x04,         // 起始序列号超过最大值
    RACP_RESPONSE_RESULT_COMMAND_OTHER_ERR = 0x05,                     // 其他错误
    RACP_RESPONSE_RESULT_COMMAND_BUSY = 0x06,                          // 前一条获取历史数据命令处理中
}racp_response_t;


// RACP数据包结构体
typedef struct
{
    uint8_t   ucOpCode;                             // 操作码
    uint8_t   ucOperator;                           // 操作数
    uint8_t   ucDataLen;                            // 所携带数据的长度
    uint8_t* pData;                                 // 所携带数据的指针
}__attribute__((packed)) ble_cgms_racp_datapacket_t;

/* Private typedef -----------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void racp_response_code_send(ble_event_info_t BleEventInfo, uint8_t ucOpcode, uint8_t ucValue);
void on_racp_value_write(ble_event_info_t BleEventInfo, uint16_t usLen, uint8_t* pData);



#endif /* __CGMS_RACP_H */
