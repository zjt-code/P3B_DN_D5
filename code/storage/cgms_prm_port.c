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
#include "cgms_prm_port.h"
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  cgms_prm_port_init
* Description    :  初始化(HAL)
* Input          :  void
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
__attribute__((weak)) uint8_t cgms_prm_port_init(void)
{
    return 0;
}


/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  cgms_prm_port_erase_sector
* Description    :  擦除接口(HAL)
* Input          :  uint32_t uiAddr
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
__attribute__((weak)) uint8_t cgms_prm_port_erase_sector(uint32_t uiAddr)
{
    return 1;
}



/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_prm_port_write
* Description    :  写入接口(HAL)
* Input          :  uint32_t uiAddr
* Input          :  uint8_t * pWriteData
* Input          :  uint16_t usWriteLen(单位:字节)
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
__attribute__((weak)) uint8_t cgms_prm_port_write(uint32_t uiAddr, uint8_t* pWriteData, uint16_t usWriteLen)
{
    return 1;
}

/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_prm_port_read
* Description    :  读取接口(HAL)
* Input          :  uint32_t uiAddr
* Input          :  uint8_t * pReadData
* Input          :  uint16_t usReadLen
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
__attribute__((weak)) uint8_t cgms_prm_read(uint32_t uiAddr, uint8_t* pReadData, uint16_t usReadLen)
{
    return 1;
}



/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  cgms_prm_flash_init
* Description    :  初始化
* Input          :  void
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_prm_flash_init(void)
{
    return cgms_prm_port_init();
}


/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  cgms_prm_flash_erase_sector
* Description    :  擦除接口
* Input          :  uint32_t uiAddr
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_prm_flash_erase_sector(uint32_t uiAddr)
{
    return cgms_prm_port_erase_sector(uiAddr);
}


/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_prm_flash_write
* Description    :  写入接口
* Input          :  uint32_t uiAddr
* Input          :  uint8_t * pWriteData
* Input          :  uint16_t usWriteLen(单位:字节)
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_prm_flash_write(uint32_t uiAddr, uint8_t* pWriteData, uint16_t usWriteLen)
{
    return cgms_prm_port_write(uiAddr, pWriteData, usWriteLen);
}

/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_prm_flash_read
* Description    :  读取接口
* Input          :  uint32_t uiAddr
* Input          :  uint8_t * pReadData
* Input          :  uint16_t usReadLen
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_prm_flash_read(uint32_t uiAddr, uint8_t* pReadData, uint16_t usReadLen)
{
    return cgms_prm_port_read(uiAddr,(uint8_t*)pReadData,usReadLen);
}

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




