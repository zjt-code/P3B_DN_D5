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

#if !defined(LOG_TAG)
#define LOG_TAG                "AFE"
#endif
#undef LOG_LVL
#define LOG_LVL                ELOG_LVL_INFO


#include "bms003.h"
#include "ltcgm1272.h"
#include "sl_sleeptimer.h"
#include <elog.h>
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/




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
    log_d("afe_init\n");
#if 1
    // 初始化bms003
    bms003_init();

    // bms003开始工作
    bms003_start();
    
    // bms003配置
    bms003_config();

#else
    // 初始化LTCGM1272
    ltcgm1272_init();

    // LTCGM1272开始工作
    ltcgm1272_start();
#endif
}




/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




