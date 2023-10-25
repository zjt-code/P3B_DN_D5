/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  ble_update_connect_param.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  25/10/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BLE_UPDATE_CONNECT_PARAM_H
#define __BLE_UPDATE_CONNECT_PARAM_H

/* Includes ------------------------------------------------------------------*/
#include "app_global.h"
#include "stdbool.h"
#include "stdint.h"
/* Private define ------------------------------------------------------------*/


/* Private typedef -----------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
bool ble_update_connect_param_is_pass(uint16_t usConnectionHandle);
void ble_update_connect_param_stop(uint16_t usConnectionHandle);
void ble_update_connect_param_all_stop(void);
void ble_update_connect_param_start(uint16_t usConnectionHandle);
void ble_update_connect_param_timer_handle(uint16_t usConnectionHandle);
#endif /* __BLE_UPDATE_CONNECT_PARAM_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/

