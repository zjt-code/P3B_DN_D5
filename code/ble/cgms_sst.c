/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_sst.c
* Author             :  陈苏阳
* CPU Type         	 :  RSL15
* IDE                :  Onsemi IDE
* Version            :  V1.0
* Date               :  28/4/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#if !defined(LOG_TAG)
#define LOG_TAG                   "CGMS_SST"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_INFO

#include <elog.h>
#include "cgms_sst.h"
#include "cgms_crc.h"
#include "app_util.h"
#include "ble_customss.h"
#include "string.h"
/* Private variables ---------------------------------------------------------*/
ble_cgms_sst_t g_mSST;

        
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/



/*******************************************************************************
*                           陈苏阳@2023-02-16
* Function Name  :  cgms_update_sst_and_time_zone
* Description    :  更新启动时间
* Input          :  uint32_t uiStartTime
* Input          :  uint8_t ucTimeZone
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_update_sst_and_time_zone(uint32_t uiStartTime, uint8_t ucTimeZone)
{
    // 更新读取启动时间的char的数据
    att_get_start_time()->uiStartTime = uiStartTime;
    att_get_start_time()->ucTimeZone = ucTimeZone;
    att_update_start_time_char_data_crc();

    memset(&g_mSST, 0, sizeof(ble_cgms_sst_t));

    // 将时间戳转换为年月日
    get_date_time_from_second(uiStartTime, &(g_mSST.date_time.time_info));

    g_mSST.time_zone = ucTimeZone;

    log_i("cgm_update_sst %04d/%02d/%02d  %02d:%02d:%02d\r\n", g_mSST.date_time.time_info.year, g_mSST.date_time.time_info.month, g_mSST.date_time.time_info.day, g_mSST.date_time.time_info.hour, g_mSST.date_time.time_info.minute, g_mSST.date_time.time_info.sec);
}

/*******************************************************************************
*                           陈苏阳@2023-02-16
* Function Name  :  cgms_sst_init
* Description    :  初始化启动时间
* Input          :  void
* Output         :  None
* Return         :  uint32_t
*******************************************************************************/
uint32_t cgms_sst_init(void)
{
    memset(&g_mSST, 0, sizeof(ble_cgms_sst_t));
    //2000.01.01.00
    g_mSST.date_time.time_info.sec = 0x00;
    g_mSST.date_time.time_info.minute = 0x00;
    g_mSST.date_time.time_info.hour = 0x00;
    g_mSST.date_time.time_info.day = 0x00;
    g_mSST.date_time.time_info.month = 0x00;
    g_mSST.date_time.time_info.year = 0x07D0;
    g_mSST.time_zone = 0xEC;
    g_mSST.dst = 0x00;

    att_get_start_time()->uiStartTime = get_second_time(&(g_mSST.date_time.time_info));
    att_get_start_time()->ucTimeZone = g_mSST.time_zone;
    att_update_start_time_char_data_crc();

    return 0;
}



/*******************************************************************************
*                           陈苏阳@2023-04-28
* Function Name  :  cgms_sst_recover
* Description    :  根据提供的ble_cgms_sst_t结构体变量来恢复传感器的SST
* Input          :  ble_cgms_sst_t SST
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_sst_recover(ble_cgms_sst_t SST)
{
    g_mSST = SST;
    att_get_start_time()->uiStartTime = get_second_time(&(g_mSST.date_time.time_info));
    att_get_start_time()->ucTimeZone = g_mSST.time_zone;
    att_update_start_time_char_data_crc();
}

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/

