/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_db_port.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  23/10/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "cgms_db_port.h"
#include <stdbool.h>
#include <stdint.h>
#include "string.h"
/* Private variables ---------------------------------------------------------*/
cgms_db_port_info_t g_CgmsDbPortInfo;

        
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_db_port_erase_sector
* Description    :  擦除接口(HAL)
* Input          :  uint32_t uiAddr
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
__attribute__((weak)) uint8_t cgms_db_port_erase_sector(uint32_t uiAddr)
{
    return 1;
}

/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_db_port_write
* Description    :  写入接口(HAL)
* Input          :  uint32_t uiAddr
* Input          :  uint8_t * pWriteData
* Input          :  uint16_t usWriteLen(单位:字节)
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
__attribute__((weak)) uint8_t cgms_db_port_write(uint32_t uiAddr, uint8_t* pWriteData, uint16_t usWriteLen)
{
    return 1;
}

/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_db_port_read
* Description    :  读取接口(HAL)
* Input          :  uint32_t uiAddr
* Input          :  uint8_t * pReadData
* Input          :  uint16_t usReadLen
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
__attribute__((weak))  uint8_t cgms_db_port_read(uint32_t uiAddr, uint8_t* pReadData, uint16_t usReadLen)
{
    return 1;
}

/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_db_port_init
* Description    :  初始化接口(HAL)
* Input          :  (cgms_db_port_info_t* pInfo
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
__attribute__((weak)) uint8_t cgms_db_port_init(cgms_db_port_info_t* pInfo)
{
    g_CgmsDbPortInfo.ucAlignAtNByte = 4;
    g_CgmsDbPortInfo.uiAddroffset = 0;
    g_CgmsDbPortInfo.usSectorByteSize = 4096;
    return 0;
}


/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_db_flash_init
* Description    :  初始化接口
* Input          :  void
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_db_flash_init(void)
{
    return cgms_db_port_init(&g_CgmsDbPortInfo);
}

/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgm_db_flash_get_info
* Description    :  获取Flash信息
* Input          :  None
* Output         :  None
* Return         :  cgms_db_port_info_t *
*******************************************************************************/
cgms_db_port_info_t* cgm_db_flash_get_info(void)
{
	return &g_CgmsDbPortInfo;
}

/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_db_flash_erase
* Description    :  擦除
* Input          :  uint32_t uiAddr
* Input          :  uint32_t uiEraseSize
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_db_flash_erase(uint32_t uiAddr,uint32_t uiEraseSize)
{
    uint8_t ucRet = 0;
    // 计算要擦除多少个扇区
    for (uint32_t i = 0; i < uiEraseSize / g_CgmsDbPortInfo.usSectorByteSize; i++)
    {
        // 擦除
        ucRet = cgms_db_port_erase_sector(g_CgmsDbPortInfo.uiAddroffset + uiAddr + (i * g_CgmsDbPortInfo.usSectorByteSize));
        if (ucRet)return ucRet;
    }
    return ucRet;
}


/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_db_flash_write
* Description    :  写数据
* Input          :  uint32_t uiAddr
* Input          :  uint8_t * pWriteData
* Input          :  uint16_t usWriteLen
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_db_flash_write(uint32_t uiAddr, uint8_t* pWriteData, uint16_t usWriteLen)
{
    return cgms_db_port_write(g_CgmsDbPortInfo.uiAddroffset + uiAddr, pWriteData, usWriteLen);
}


/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_db_flash_read
* Description    :  读数据
* Input          :  uint32_t uiAddr
* Input          :  uint8_t * pReadData
* Input          :  uint16_t usReadLen
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_db_flash_read(uint32_t uiAddr, uint8_t* pReadData, uint16_t usReadLen)
{
    return cgms_db_port_read(uiAddr, pReadData, usReadLen);
}


/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




