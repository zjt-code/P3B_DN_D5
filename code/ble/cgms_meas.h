/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_meas.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  23/10/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CGMS_MEAS_H
#define __CGMS_MEAS_H

/* Includes ------------------------------------------------------------------*/
#include "app_global.h"
/* Private define ------------------------------------------------------------*/



/* Private typedef -----------------------------------------------------------*/

// 测量数据标志位
typedef enum
{
    CGMS_MEAS_HISTORY_FLAG_HISTORY = (0x43),   // 历史数据标志位_历史数据
    CGMS_MEAS_HISTORY_FLAG_REAL = (0x83),           // 历史数据标志位_实时数据
}cgms_meas_history_flag;


typedef enum
{
    CGMS_HISTORY_SPECIAL_DATAPACKET_CODE_SEND_DONE = 0xAA,             // 历史数据发送完成
    CGMS_HISTORY_SPECIAL_DATAPACKET_CODE_SEND_ERR = 0xBB               // 历史数据发送错误
}cgm_history_special_datapacket_code_t;

// 历史数据特殊数据包
typedef struct
{
    uint8_t ucDatapacketCode;
    uint8_t uc_Reserved[13];
    uint16_t usCRC16;
}__attribute__((packed)) cgms_history_special_datapcket_t;


// CGM测量数据结构体
typedef struct
{
    uint8_t usHistoryFlag;                          // 历史数据标志位(0x43==历史数据,0x83==实时数据)
    uint16_t usGlucose;                             // 血糖浓度(mmol/L * 10)
    uint16_t usOffset;                              // 血糖数据的index(从启动CGM(=0)开始)
    uint16_t usCurrent;                             // 电极电流(nA * 100)
}__attribute__((packed)) cgms_meas_t;


/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
ret_code_t cgms_meas_send(ble_event_info_t BleEventInfo, cgms_meas_t Rec);
void ble_meas_notify_enable(void);
void ble_meas_notify_disable(void);
bool ble_meas_notify_is_enable(void);
ret_code_t cgms_meas_special_send(ble_event_info_t BleEventInfo, cgms_history_special_datapcket_t CgmsHistorySpecialDatapcket);



#endif /* __CGMS_MEAS_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/
