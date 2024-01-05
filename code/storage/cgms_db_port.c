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
#if !defined(LOG_TAG)
#define LOG_TAG                "CGMS_DB_PORT"
#endif
#undef LOG_LVL
#define LOG_LVL                ELOG_LVL_INFO


#include "cgms_db_port.h"
#include <stdbool.h>
#include <stdint.h>
#include "string.h"
#include "elog.h"
#include "app_global.h"
/* Private variables ---------------------------------------------------------*/
cgms_db_port_info_t g_CgmsDbPortInfo;

        
/* Private function prototypes -----------------------------------------------*/


extern uint8_t cgms_db_port_read(uint32_t uiAddr, uint8_t* pReadData, uint16_t usReadLen);
extern uint8_t cgms_db_port_write(uint32_t uiAddr, uint8_t* pWriteData, uint16_t usWriteLen);
extern uint8_t cgms_db_port_erase_sector(uint32_t uiAddr);
extern uint8_t cgms_db_port_init(cgms_db_port_info_t* pInfo);

/* Private functions ---------------------------------------------------------*/

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
	ret_code_t Ret = 0;

    if (uiEraseSize / g_CgmsDbPortInfo.usSectorByteSize < 1)
    {
        log_w("cgms_db_flash_erase uiEraseSize<usSectorByteSize");
    }

    // 计算要擦除多少个扇区
    for (uint32_t i = 0; i < uiEraseSize / g_CgmsDbPortInfo.usSectorByteSize; i++)
    {
        // 擦除
    	Ret = (ret_code_t)cgms_db_port_erase_sector(g_CgmsDbPortInfo.uiAddroffset + uiAddr + (i * g_CgmsDbPortInfo.usSectorByteSize));

        if (Ret != RET_CODE_SUCCESS)
        {
            log_e("cgms_db_port_erase_sector fail (0x%x)  :%d", g_CgmsDbPortInfo.uiAddroffset + uiAddr + (i * g_CgmsDbPortInfo.usSectorByteSize), Ret);
        }
        if (Ret)return Ret;
    }
    return Ret;
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
    if (usWriteLen % 4)
    {
        log_w("cgms_db_port_write Write Byte len is %d bytes", usWriteLen);
    }
    ret_code_t Ret = (ret_code_t)cgms_db_port_write(g_CgmsDbPortInfo.uiAddroffset + uiAddr, pWriteData, usWriteLen);

    if (Ret != RET_CODE_SUCCESS)
    {
        log_e("cgms_db_port_write fail (0x%x,%d)  :%d", g_CgmsDbPortInfo.uiAddroffset + uiAddr, usWriteLen,Ret);
    }
    return Ret;
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
    ret_code_t Ret = cgms_db_port_read(g_CgmsDbPortInfo.uiAddroffset + uiAddr, pReadData, usReadLen);
    if (Ret != RET_CODE_SUCCESS)
    {
        log_e("cgms_db_flash_read fail (0x%x,%d)  :%d", g_CgmsDbPortInfo.uiAddroffset + uiAddr, usReadLen, Ret);
    }
    return Ret;
}


/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




