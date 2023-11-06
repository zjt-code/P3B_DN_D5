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
#include "em_msc.h"
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
  if(MSC_ErasePage(g_uiPrmFlashAddrOffset+uiAddr)==mscReturnOk)return 0;
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
    if (MSC_WriteWord(g_uiPrmFlashAddrOffset+uiAddr, (uint32_t*)pWriteData, usWriteLen) == mscReturnOk)
    {
        return 10;
    }
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
    memcpy(pReadData, (uint32_t*)(g_uiPrmFlashAddrOffset+uiAddr), usReadLen);
    return 0;
}


/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




