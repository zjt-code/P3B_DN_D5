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
#include "stdbool.h"
/* Private define ------------------------------------------------------------*/



/* Private typedef -----------------------------------------------------------*/

typedef void (*afe_irq_callback)(void);

/* Private variables ---------------------------------------------------------*/




/* Private function prototypes -----------------------------------------------*/
bool afe_is_working(void);
void afe_init(void);
void afe_stop(void);
void afe_start(void);
bool afe_new_data_is_ready(void);
bool afe_get_new_data(double* pNewData);
void afe_register_irq_callback(afe_irq_callback callback);
#endif /* __AFE_H */

/******************* (C) COPYRIGHT 2023 ������ **** END OF FILE ****************/

