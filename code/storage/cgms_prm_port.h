/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_prm_port.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  6/11/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CGMS_PRM_PORT_H
#define __CGMS_PRM_PORT_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

/* Private define ------------------------------------------------------------*/



/* Private typedef -----------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
uint8_t cgms_prm_flash_init(void);
uint8_t cgms_prm_flash_erase(uint32_t uiAddr, uint32_t uiEraseSize);
uint8_t cgms_prm_flash_write(uint32_t uiAddr, uint8_t* pWriteData, uint16_t usWriteLen);
uint8_t cgms_prm_flash_read(uint32_t uiAddr, uint8_t* pReadData, uint16_t usReadLen);
#endif /* __CGMS_PRM_PORT_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/
