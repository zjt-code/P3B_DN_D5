/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  afe.h
* Author             :  陈苏阳
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

#define AFE_USE_BMS003          (1)           // 使用暖芯咖的AFE
//#define AFE_USE_LTCGM1272       (1)           // 使用列拓的AFE
//#define AFE_USE_CEM102          (1)           // 使用安森美的AFE

/* Private typedef -----------------------------------------------------------*/

typedef void (*afe_irq_callback)(void);


typedef enum
{
    AFE_RUN_MODE_CONTINUOUS = 0x00,                                 // 连续模式
    AFE_RUN_MODE_SHOT = 0x01                                        // 猝发模式
}afe_run_mode_t;
/* Private variables ---------------------------------------------------------*/




/* Private function prototypes -----------------------------------------------*/
bool afe_is_working(void);
void afe_init(void);
void afe_stop(void);
void afe_start(afe_run_mode_t RunMode);
void afe_shot(uint8_t ucSampleingCnt);
bool afe_new_data_is_ready(void);
bool afe_get_new_data(double* pNewData);
void afe_register_irq_callback(afe_irq_callback callback);
#endif /* __AFE_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/

