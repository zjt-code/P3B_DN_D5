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
    RACP_OPCODE_RESERVED = 0x00,       /**< Record Access Control Point opcode - Reserved for future use. */
    RACP_OPCODE_DELETE_RECS = 0x02,       /**< Record Access Control Point opcode - Delete stored records. */
    RACP_OPCODE_ABORT_OPERATION = 0x03,       /**< Record Access Control Point opcode - Abort operation. */
    RACP_OPCODE_REPORT_NUM_RECS = 0x04,       /**< Record Access Control Point opcode - Report number of stored records. */
    RACP_OPCODE_NUM_RECS_RESPONSE = 0x05,       /**< Record Access Control Point opcode - Number of stored records response. */
    RACP_OPCODE_RESPONSE_CODE = 0x06,       /**< Record Access Control Point opcode - Response code. */
    RACP_OPCODE_EXIT = 0X62,
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
    RACP_RESPONSE_RESULT_OPCODE_UNSUPPORTED = 0x02,                 // 命令不支持
    RACP_RESPONSE_RESULT_INVALID_OPERATOR = 0x03,                   // 无效操作数
    RACP_RESPONSE_RESULT_OPERATOR_UNSUPPORTED = 0x04,               // 操作数不支持
    RACP_RESPONSE_RESULT_INVALID_OPERAND = 0x05,                    // 其他错误
    RACP_RESPONSE_RESULT_NO_RECORDS_FOUND = 0x06,                   // 没有记录被找到
    RACP_RESPONSE_RESULT_ABORT_FAILED = 0x07,       /**< Record Access Control Point response code - Abort could not be completed. */
    RACP_RESPONSE_RESULT_PROCEDURE_NOT_DONE = 0x08,       /**< Record Access Control Point response code - Procedure could not be completed. */
    RACP_RESPONSE_RESULT_OPERAND_UNSUPPORTED = 0x09,       /**< Record Access Control Point response code - Unsupported operand. */

}racp_response_t;


// RACP数据包结构体
typedef struct
{
    uint8_t   ucOpCode;                             // 操作码
    uint8_t   ucOperator;                           // 操作数
    uint8_t   ucDataLen;                            // 所携带数据的长度
    uint8_t  ucData[16];                            // 所携带数据
}__attribute__((packed)) ble_cgms_racp_datapacket_t;

/* Private typedef -----------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
void ble_racp_notify_enable(void);
void ble_racp_notify_disable(void);
void on_racp_value_write(ble_event_info_t BleEventInfo, uint16_t usLen, uint8_t* pData);



#endif /* __CGMS_RACP_H */
