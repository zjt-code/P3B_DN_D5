/******************** (C) COPYRIGHT 2024 ³ÂËÕÑô ********************************
* File Name          :  utility.h
* Author             :  ³ÂËÕÑô
* CPU Type         	 :  NRF52840
* IDE                :  IAR 8.11
* Version            :  V1.0
* Date               :  19/9/2024
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UTILITY_H
#define __UTILITY_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
/* Private define ------------------------------------------------------------*/



/* Private typedef -----------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

uint8_t datapacket_padding_and_encrpty(uint8_t* pDestin, uint8_t* pSpSourcerc, uint8_t ucSrcLen);
void print_reset_cause(uint32_t uiResetCause);
#endif /* __UTILITY_H */

/******************* (C) COPYRIGHT 2024 ³ÂËÕÑô **** END OF FILE ****************/

