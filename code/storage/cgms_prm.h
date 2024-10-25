/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_prm.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  30/11/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CGMS_PRM_H
#define __CGMS_PRM_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "cgms_db.h"
#include "app.h"


typedef struct
{
    uint8_t           prmWMY[4];
    uint16_t          SN;
    int16_t           DacVolOffset;
    int16_t           AdcK;
    int16_t           AdcB;
    uint16_t          usCrc16;
}__attribute__((packed)) prm_t;

typedef struct
{
    uint8_t ucDataValidFlag;                                        // 数据有效标志位
    cgm_measurement_sensor_state_t LastCgmState;                    // 上一次的CGM工作状态
    float fUseSensorK;                                              // 上一次使用的传感器Code
    uint32_t uiLastCgmSessionStartTime;                             // 上一次的启动时间
    uint8_t ucLastTimeZone;                                         // 上一次使用的时区
    uint8_t ucLastStartBy;                                          // 上一次的启动方式
    uint8_t ucLastStartByVersion[3];                                // 上一次启动时APP的版本
    uint8_t ucLastPasswordExist;                                    // 上一次是否被设置过密码
    uint16_t usLastPassword;                                        // 上一次设置的密码
    uint8_t ucCgmSessionCnt;                                        // CGM使用次数
    uint8_t ucNone[8];                                             // 未使用
    uint16_t usCrc16;
}__attribute__((packed)) user_usage_data_t;


extern user_usage_data_t g_UserUsageData;
extern prm_t g_PrmDb;
extern uint8_t g_ucSn[];

/* Private function prototypes -----------------------------------------------*/
ret_code_t cgms_prm_get_sn(char* buff);
void cgms_prm_db_power_on_init(void);
uint8_t* cgms_prm_get_sn_p(void);
ret_code_t cgms_prm_db_write_flash(void);
ret_code_t cgms_prm_db_write_user_usage_data(user_usage_data_t* pData);
ret_code_t cgms_prm_db_read_user_usage_data(user_usage_data_t* pData);
void cgms_prm_db_print_user_usage_data(user_usage_data_t* pData);
#endif /* __CGMS_PRM_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/