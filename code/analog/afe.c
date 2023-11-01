/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  afe.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  27/10/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "bms003.h"
#include "sl_sleeptimer.h"
#include "app_log.h"
/* Private variables ---------------------------------------------------------*/

sl_sleeptimer_timer_handle_t g_AfeTimer;
        
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/


void afe_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data)
{
  app_log_info("bms003_config");
    //bms003_config();
}




/*******************************************************************************
*                           陈苏阳@2023-10-27
* Function Name  :  afe_init
* Description    :  AFE初始化
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void afe_init(void)
{
    bms003_init();
    app_log_info("bms003_config");
    bms003_config();
    app_log_info("bms003_config done");
}



/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




