/******************** (C) COPYRIGHT 2023 ������ ********************************
* File Name          :  app_battery.c
* Author             :  ������
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  15/11/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "app_battery.h"

/* Private variables ---------------------------------------------------------*/

static uint8_t g_ucAppBatteryFsmState = 0;          // ��������״̬��
static uint32_t g_uiBattaryVol = 0;                 // ��ǰ��ص�ѹ
static uint32_t g_uiBatteryLifeTimeCnt;             // ����������ڵ�����
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
*                           ������@2022-12-23
* Function Name  :  app_battery_get_run_time
* Description    :  ��ȡ��ص�����ʱ��(��λ:0.1Hour)
* Input          :  void
* Output         :  None
* Return         :  uint32_t
*******************************************************************************/
uint32_t app_battery_get_run_time(void)
{
    return g_uiBatteryLifeTimeCnt / 6;
}


/*******************************************************************************
*                           ������@2022-12-23
* Function Name  :  app_battery_timer_handler
* Description    :  �����ɼ���ʱ���ص�����
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
        // ����һ�ε����ɼ�
        //app_battery_adc_start();
    }
    else
    {
        g_ucAppBatteryFsmState = 0;

        // ����һ�ε�������
        //battery_level_update();
    }
}







/******************* (C) COPYRIGHT 2023 ������ **** END OF FILE ****************/




