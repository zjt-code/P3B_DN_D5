/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  temp_sensor.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  6/11/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TEMP_SENSOR_H
#define __TEMP_SENSOR_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "stdbool.h"

/* Private define ------------------------------------------------------------*/




/* Private typedef -----------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/
void temp_sensor_init(void);
void temp_sensor_start_meas(void);
bool temp_sensor_get_temp(int16_t* pTemp);
bool temp_sensor_is_busy(void);
#endif /* __TEMP_SENSOR_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/

