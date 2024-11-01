/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  app_battery.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  15/11/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_BATTERY_H
#define __APP_BATTERY_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "stdbool.h"

/* Private define ------------------------------------------------------------*/

typedef enum
{
    APP_BATTERY_FSM_INIT = 0,                                       // 电量组件初始化
    APP_BATTERY_FSM_CONVERSION,                                     // 电量组件转换中
    APP_BATTERY_FSM_READED,                                         // 电量组件读取完成
}app_battery_fsm_e;


typedef enum
{
    APP_BATTERY_LEVEL_VOL_MAX = 1600,                               // mV
    APP_BATTERY_LEVEL_VOL_100 = 1500,                               // mV
    APP_BATTERY_LEVEL_VOL_10 = 1300,                                // mV
    APP_BATTERY_LEVEL_VOL_5 = 1250,                                 // mV
    APP_BATTERY_LEVEL_VOL_1 = 1200                                  // mV
}app_battery_level_vol_e;

typedef struct
{
    uint16_t usVol;
    uint8_t ucLevel;
}app_battery_vol_level_point_t;

/* Private typedef -----------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


void app_battery_init(void);
uint16_t app_battery_read_battery_vol(void);
uint32_t app_battery_get_run_time(void);
void app_battery_timer_handler(uint16_t usInterval);
#endif /* __APP_BATTERY_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/

