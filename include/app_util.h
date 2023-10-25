/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  app_util.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  16/10/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_UTIL_H
#define __APP_UTIL_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

/* Private define ------------------------------------------------------------*/

#define OFFSET_SECOND   946684800
#define SECOND_OF_DAY   86400

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t week;
    uint8_t hour;
    uint8_t minute;
    uint8_t sec;
}time_info_t;


/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
uint8_t uint16_encode(uint16_t value, uint8_t* p_encoded_data);
uint16_t uint16_decode(const uint8_t* p_encoded_data);
uint8_t uint24_encode(uint32_t value, uint8_t* p_encoded_data);
void swmLogHex(uint32_t ucLevl, uint8_t* pData, uint32_t uiLen);
uint32_t get_second_time(time_info_t* pDateTime);
void get_date_time_from_second(uint32_t lSec, time_info_t* pDateTime);
#endif /* __APP_UTIL_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/



