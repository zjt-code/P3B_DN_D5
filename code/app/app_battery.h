/******************** (C) COPYRIGHT 2023 ³ÂËÕÑô ********************************
* File Name          :  app_battery.h
* Author             :  ³ÂËÕÑô
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



/* Private typedef -----------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
uint32_t app_battery_get_run_time(void);
void app_battery_timer_handler(uint16_t usInterval);
#endif /* __APP_BATTERY_H */

/******************* (C) COPYRIGHT 2023 ³ÂËÕÑô **** END OF FILE ****************/

