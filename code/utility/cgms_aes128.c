/******************** (C) COPYRIGHT 2023 陈 ********************************
* File Name          :  cgms_aes128.c
* Author             :  陈
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  24/10/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "mbedtls/aes.h"
#include "cgms_aes128.h"

/* Private variables ---------------------------------------------------------*/
static uint8_t aes_ecb_128_key[16] = { 0x2B,0x7E,0x15,0x16,0x28,0xAE,0xD2,0xA6,0xAB,0xF7,0x15,0x88,0x09,0xCF,0xAC,0xEF };
//static uint8_t aes_ecb_128_key[16] = { 0x2B,0x7E,0xAA,0x16,0x33,0xAE,0xD2,0xA5,0xAB,0xF7,0xAA,0xC5,0x00,0xDD,0xAC,0xEF };
mbedtls_aes_context aes;
        
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/


void cgms_aes128_update_key(uint8_t* value)
{
    aes_ecb_128_key[14] = value[1];
    aes_ecb_128_key[15] = value[0];
}


void cgms_aes128_encrpty(uint8_t* plain, uint8_t* cipher)
{
	 	 mbedtls_aes_setkey_enc(&aes, aes_ecb_128_key, 128);	//设置加密密钥
		 mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, plain, cipher);//ECB加密
}

void cgms_aes128_decrpty(uint8_t* cipher, uint8_t* plain_decrypt)
{
		mbedtls_aes_setkey_dec(&aes, aes_ecb_128_key, 128);//设置解密密钥
		mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, cipher, plain_decrypt);//ECB解密
}

uint8_t mbedtls_aes_pkcspadding(uint8_t* data, uint8_t data_len)
{
    uint8_t padding_len = 0, i = 0;
    padding_len = 16 - data_len;
    if (padding_len >= 16)
    {
        padding_len = 0;
    }
    for (i = data_len; i < 16; i++)
    {
        data[i] = padding_len;
    }
    return 0;
}


/******************* (C) COPYRIGHT 2023 陈 **** END OF FILE ****************/




