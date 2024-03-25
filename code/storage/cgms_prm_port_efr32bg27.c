/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_prm_port_efr32bg27.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  23/10/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "string.h"
#include <stdio.h>
#include <string.h>
#include "nvm3_default.h"
#include "nvm3_default_config.h"
/* Private variables ---------------------------------------------------------*/

uint32_t g_uiPrmFlashAddrOffset = 0xFE00000;
        
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  cgms_prm_port_init
* Description    :  初始化
* Input          :  void
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_prm_port_init(void)
{
  if(nvm3_initDefault()==ECODE_NVM3_OK)return 0;
  return 1;
}


/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_prm_port_erase_sector
* Description    :  擦除接口_具体实现
* Input          :  uint32_t uiAddr
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_prm_port_erase_sector(uint32_t uiAddr)
{
  while (nvm3_repackNeeded(nvm3_defaultHandle))
  {
    nvm3_repack(nvm3_defaultHandle);
  }
  return 0;
}

/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_prm_port_write
* Description    :  写入接口(具体实现)
* Input          :  uint32_t uiAddr
* Input          :  uint8_t * pWriteData
* Input          :  uint16_t usWriteLen(单位:字节)
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_prm_port_write(uint32_t uiAddr, uint8_t* pWriteData, uint16_t usWriteLen)
{

  if (ECODE_NVM3_OK != nvm3_writeData(nvm3_defaultHandle, uiAddr, (unsigned char *)pWriteData, usWriteLen))return 1;
  return 0;
}

/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_prm_port_read
* Description    :  读取接口(具体实现)
* Input          :  uint32_t uiAddr
* Input          :  uint8_t * pReadData
* Input          :  uint16_t usReadLen
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_prm_port_read(uint32_t uiAddr, uint8_t* pReadData, uint16_t usReadLen)
{
    if(nvm3_readData(nvm3_defaultHandle, uiAddr, pReadData, usReadLen)!=ECODE_NVM3_OK)return 1;
    return 0;
}


/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




