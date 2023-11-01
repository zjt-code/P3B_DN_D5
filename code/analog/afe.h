/******************** (C) COPYRIGHT 2023 ������ ********************************
* File Name          :  afe.h
* Author             :  ������
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  27/10/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AFE_H
#define __AFE_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

/* Private define ------------------------------------------------------------*/



/* Private typedef -----------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

void afe_int_irq_callback(void);
void afe_init(void);
#endif /* __AFE_H */

/******************* (C) COPYRIGHT 2023 ������ **** END OF FILE ****************/

