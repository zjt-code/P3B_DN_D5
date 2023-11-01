/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_aes128.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  24/10/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CGMS_AES128_H
#define __CGMS_AES128_H

/* Includes ------------------------------------------------------------------*/
#include "mbedtls/aes.h"


/* Private define ------------------------------------------------------------*/



/* Private typedef -----------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
uint8_t mbedtls_aes_pkcspadding(uint8_t * data, uint8_t data_len);
void cgms_aes128_update_key(uint8_t* value);

void cgms_aes128_encrpty(uint8_t* plain, uint8_t* cipher);

void cgms_aes128_decrpty(uint8_t* cipher, uint8_t* plain_decrypt);

#endif /* __CGMS_AES128_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/

