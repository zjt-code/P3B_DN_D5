/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_db_port_efr32bg27.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  23/10/2023
* Description        :  
*******************************************************************************/
#if !defined(LOG_TAG)
#define LOG_TAG                "CGMS_DB_PORT"
#endif
#undef LOG_LVL
#define LOG_LVL                ELOG_LVL_WARN

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "string.h"
#include "cgms_db_port.h"
#include "em_msc.h"
#include "elog.h"
/* Private variables ---------------------------------------------------------*/


        
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_db_port_init
* Description    :  初始化接口_具体实现
* Input          :  cgms_db_port_info_t* pInfo
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_db_port_init(cgms_db_port_info_t* pInfo)
{
    pInfo->ucAlignAtNByte = 4;
    pInfo->uiAddroffset = 0x8090000;
    pInfo->usSectorByteSize = 1024*8;
    return 0;
}




/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_db_port_erase_sector
* Description    :  擦除接口_具体实现
* Input          :  uint32_t uiAddr
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_db_port_erase_sector(uint32_t uiAddr)
{
    //log_d("cgms_db_port_erase_sector:0x%x, MSC STATUS:0x%x", uiAddr,MSC->STATUS);
    MSC_Status_TypeDef Ret = MSC_ErasePage((uint32_t*)uiAddr);
    if (Ret == mscReturnOk)return 0;
    return 1;
}

/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_db_port_write
* Description    :  写入接口(具体实现)
* Input          :  uint32_t uiAddr
* Input          :  uint8_t * pWriteData
* Input          :  uint16_t usWriteLen(单位:字节)
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_db_port_write(uint32_t uiAddr, uint8_t* pWriteData, uint16_t usWriteLen)
{
    //log_d("cgms_db_port_write:0x%x, MSC STATUS:0x%x", uiAddr,MSC->STATUS);
    //elog_hexdump("data", 16, pWriteData, 16);
    if (MSC_WriteWord((uint32_t*)uiAddr, (uint32_t*)pWriteData, usWriteLen) == mscReturnOk)
    {
        return 0;
    }
    return 1;
}

/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_db_port_read
* Description    :  读取接口(具体实现)
* Input          :  uint32_t uiAddr
* Input          :  uint8_t * pReadData
* Input          :  uint16_t usReadLen
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t cgms_db_port_read(uint32_t uiAddr, uint8_t* pReadData, uint16_t usReadLen)
{
    //log_d("cgms_db_port_read:0x%x, MSC STATUS:0x%x", uiAddr,MSC->STATUS);
    memcpy(pReadData, (uint32_t*)uiAddr, usReadLen);
    //elog_hexdump("data", 16, pReadData, 16);
    return 0;
}


/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




