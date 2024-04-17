/******************** (C) COPYRIGHT 2024 陈苏阳 ********************************
* File Name          :  cgms_debug_db.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52840
* IDE                :  IAR 8.11
* Version            :  V1.0
* Date               :  5/3/2024
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#if !defined(LOG_TAG)
#define LOG_TAG                "CGMS_DEBUG_DB"
#endif
#undef LOG_LVL
#define LOG_LVL                ELOG_LVL_DEBUG


#include <stdbool.h>
#include <stdint.h>
#include "stdlib.h"
#include "cgms_crc.h"
#include <elog.h>
#include "cgms_db_port.h"
#include "cgms_debug_db.h"
#include "string.h"
/* Private variables ---------------------------------------------------------*/
__attribute__((aligned(4))) cgms_debug_info_t	g_mDebugInfo;
#define CGMS_DEBUG_INFO_FLASH_ADDR              (0x2000+0x1C000)   // 存储Debug信息的Flash地址

/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           陈苏阳@2024-04-15
* Function Name  :  cgms_debug_write_kv
* Description    :  写入键值对
* Input          :  char * pKey
* Input          :  uint8_t * pData
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool cgms_debug_write_kv(char* pKey, uint8_t* pData)
{
    for (uint8_t i = 0; i < CGMS_DEBUG_MAX_KV_NUM; i++)
    {
        if (g_mDebugInfo.KvArray[i].cKey[0] ==0)
        {
            strncpy(g_mDebugInfo.KvArray[i].cKey, pKey, 12);
            memcpy(g_mDebugInfo.KvArray[i].ucData, pData, 4);
            return true;
        }
    }
    return false;
}


/*******************************************************************************
*                           陈苏阳@2024-04-15
* Function Name  :  cgms_debug_read_kv
* Description    :  读取键值对
* Input          :  char * pKey
* Input          :  uint8_t * pData
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool cgms_debug_read_kv(char* pKey, uint8_t* pData)
{
    if (pKey)
    {
        for (uint8_t i = 0; i < CGMS_DEBUG_MAX_KV_NUM; i++)
        {
            if (g_mDebugInfo.KvArray[i].cKey[0] != 0 && strcmp(pKey,g_mDebugInfo.KvArray[i].cKey)==0)
            {
                memcpy(pData,g_mDebugInfo.KvArray[i].ucData, 4);
                return true;
            }
        }
    }
    return false;
}

/*******************************************************************************
*                           陈苏阳@2024-04-15
* Function Name  :  debug_get_kv
* Description    :  获取键值对
* Input          :  uint8_t ucIndex
* Input          :  char * pKey
* Input          :  uint8_t * pData
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool debug_get_kv(uint8_t ucIndex, char* pKey, uint8_t* pData)
{
    if (ucIndex < CGMS_DEBUG_MAX_KV_NUM)
    {
        if (g_mDebugInfo.KvArray[ucIndex].cKey[0] != 0)
        {
            strncpy(pKey, g_mDebugInfo.KvArray[ucIndex].cKey, 12);
            memcpy(pData, g_mDebugInfo.KvArray[ucIndex].ucData, 4);
            return true;
        }
    }
    return false;
}

/*******************************************************************************
*                           陈苏阳@2024-04-15
* Function Name  :  cgms_debug_clear_all_kv
* Description    :  清除所有的键值对
* Input          :  None
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_debug_clear_all_kv(void)
{
    memset(&g_mDebugInfo, 0x00, sizeof(g_mDebugInfo));
}

/*******************************************************************************
*                           陈苏阳@2024-03-05
* Function Name  :  cgms_debug_db_write
* Description    :  写debug信息
* Input          :  None
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_debug_db_write(void)
{
    g_mDebugInfo.usCrc = 0;
    g_mDebugInfo.usCrc = do_crc(&g_mDebugInfo, sizeof(g_mDebugInfo));
    // 擦除并写入
    cgms_db_flash_erase(CGMS_DEBUG_INFO_FLASH_ADDR, cgm_db_flash_get_info()->usSectorByteSize);
    cgms_db_flash_write(CGMS_DEBUG_INFO_FLASH_ADDR,(uint8_t*)&g_mDebugInfo, sizeof(g_mDebugInfo));
    cgms_debug_db_print();
}

/*******************************************************************************
*                           陈苏阳@2024-03-05
* Function Name  :  cgms_debug_db_read
* Description    :  读debug信息
* Input          :  None
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool cgms_debug_db_read(void)
{
    cgms_debug_clear_all_kv();
    cgms_db_flash_read(CGMS_DEBUG_INFO_FLASH_ADDR, (uint8_t*)&g_mDebugInfo, sizeof(g_mDebugInfo));
    uint16_t usCrc = g_mDebugInfo.usCrc;
    g_mDebugInfo.usCrc = 0;
    g_mDebugInfo.usCrc = do_crc(&g_mDebugInfo, sizeof(g_mDebugInfo));
    if (usCrc == g_mDebugInfo.usCrc)
    {
        return true;
    }
    else
    {
        log_w("cgms_debug_db_read fail,addr:0x%x", cgm_db_flash_get_info()->uiAddroffset + CGMS_DEBUG_INFO_FLASH_ADDR);
    }
    return false;
}

/*******************************************************************************
*                           陈苏阳@2024-03-05
* Function Name  :  cgms_debug_db_print
* Description    :  打印debug信息
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_debug_db_print(void)
{
    if (cgms_debug_db_read())
    {
        log_i("debug info:");
        for (uint8_t i = 0; i < CGMS_DEBUG_MAX_KV_NUM; i++)
        {
            if (g_mDebugInfo.KvArray[i].cKey[0] != 0)
            {
                elog_hexdump(g_mDebugInfo.KvArray[i].cKey, 4, g_mDebugInfo.KvArray[i].ucData, 4);
            }
        }
    }
    else
    {
        log_w("debug info is invalid");
        elog_hexdump("g_mDebugInfo", 16, &g_mDebugInfo, sizeof(g_mDebugInfo));
    }
}

/*******************************************************************************
*                           陈苏阳@2024-03-05
* Function Name  :  cgms_debug_db_clear
* Description    :  擦除debug信息
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_debug_db_clear(void)
{
    cgms_db_flash_erase(CGMS_DEBUG_INFO_FLASH_ADDR, cgm_db_flash_get_info()->usSectorByteSize);
}


/******************* (C) COPYRIGHT 2024 陈苏阳 **** END OF FILE ****************/




