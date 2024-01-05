/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  ble_adv.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  24/10/2023
* Description        :
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BLE_ADV_H
#define __BLE_ADV_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

/* Private define ------------------------------------------------------------*/



/* Private typedef -----------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/
void ble_adv_generate_adv_data(uint8_t* pAdvDataBuffer, uint32_t* pAdvDataLen);
void ble_adv_generate_adv_scan_response_data(uint8_t* pAdvScanRespDataBuffer, uint8_t* ucAdvScanRespDataLen);

#endif /* __BLE_ADV_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/

