/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  main.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  25/10/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#if !defined(LOG_TAG)
    #define LOG_TAG                    "main"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_INFO

#include "sl_component_catalog.h"
#include "sl_system_init.h"
#include "app.h"
#include "sl_power_manager.h"
#include "sl_system_process_action.h"
#include "app_global.h"
#include "SEGGER_RTT.h"
#include <elog.h>
#include "em_cmu.h"

#include "pin_config.h"
#include "app_glucose_meas.h"
#include "cgms_prm.h"
/* Private variables ---------------------------------------------------------*/
sl_sleeptimer_timer_handle_t g_TestTimer;

        
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/




/*******************************************************************************
*                           陈苏阳@2023-10-25
* Function Name  :  main
* Description    :  
* Input          :  void
* Output         :  None
* Return         :  int
*******************************************************************************/
int main(void)
{
    // 系统初始化
    sl_system_init();

    // 初始化GPIO的时钟
    CMU_ClockEnable(cmuClock_GPIO, true);


    GPIO_PinModeSet(gpioPortC, 1, gpioModePushPull, 0);
    GPIO_PinModeSet(gpioPortC, 2, gpioModePushPull, 0);
    GPIO_PinModeSet(gpioPortC, 3, gpioModePushPull, 0);
    GPIO_PinModeSet(gpioPortC, 4, gpioModePushPull, 0);
    GPIO_PinModeSet(gpioPortC, 5, gpioModePushPull, 0);
    GPIO_PinModeSet(gpioPortC, 6, gpioModePushPull, 0);
    GPIO_PinModeSet(gpioPortC, 7, gpioModePushPull, 0);

    GPIO_PinModeSet(gpioPortD, 2, gpioModePushPull, 0);
    GPIO_PinModeSet(gpioPortD, 3, gpioModePushPull, 0);

    GPIO_PinModeSet(gpioPortA, 0, gpioModePushPull, 0);

    GPIO_PinModeSet(gpioPortB, 4, gpioModePushPull, 0);

    // 初始化log
    SEGGER_RTT_Init();
    SEGGER_RTT_SetTerminal(0);

    elog_init();
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_TIME | ELOG_FMT_TAG | ELOG_FMT_LVL | ELOG_FMT_FUNC | ELOG_FMT_LINE);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_TIME | ELOG_FMT_TAG | ELOG_FMT_LVL | ELOG_FMT_FUNC | ELOG_FMT_LINE);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_TIME | ELOG_FMT_TAG | ELOG_FMT_LVL | ELOG_LVL_INFO);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_TIME | ELOG_FMT_TAG | ELOG_FMT_LVL | ELOG_LVL_INFO);


    elog_start();
    log_i("sys init");

    // 参数存储上电初始化
    cgms_prm_db_power_on_init();

    while (1)
    {
        sl_system_process_action();
        sl_power_manager_sleep();
    }
}




/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/





