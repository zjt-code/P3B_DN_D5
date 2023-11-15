/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  app_battery.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  15/11/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "app_battery.h"

/* Private variables ---------------------------------------------------------*/

static uint8_t g_ucAppBatteryFsmState = 0;          // 电量测量状态机
static uint32_t g_uiBattaryVol = 0;                 // 当前电池电压
static uint32_t g_uiBatteryLifeTimeCnt;             // 电池生命周期的秒数
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
*                           陈苏阳@2022-12-23
* Function Name  :  app_battery_get_run_time
* Description    :  获取电池的运行时间(单位:0.1Hour)
* Input          :  void
* Output         :  None
* Return         :  uint32_t
*******************************************************************************/
uint32_t app_battery_get_run_time(void)
{
    return g_uiBatteryLifeTimeCnt / 6;
}


/*******************************************************************************
*                           陈苏阳@2022-12-23
* Function Name  :  app_battery_timer_handler
* Description    :  电量采集定时器回调函数
* Input          :  uint16_t usInterval
* Output         :  None
* Return         :  None
*******************************************************************************/
void app_battery_timer_handler(uint16_t usInterval)
{
    g_uiBatteryLifeTimeCnt += usInterval;

    if (g_ucAppBatteryFsmState == 0)
    {
        g_ucAppBatteryFsmState = 1;
        // 运行一次电量采集
        //app_battery_adc_start();
    }
    else
    {
        g_ucAppBatteryFsmState = 0;

        // 运行一次电量更新
        //battery_level_update();
    }
}







/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




