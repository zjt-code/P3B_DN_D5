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
#define LOG_LVL                ELOG_LVL_DEBUG

#include "afe.h"
#include "bms003.h"
#include "ltcgm1272.h"
#include "cgms_prm.h"
#include "sl_sleeptimer.h"
#include <elog.h>
/* Private variables ---------------------------------------------------------*/
double g_fCurrElectricCurrent = 0.00;                               // 当前电流
bool g_bAfeisWorking = false;                                       // AFE工作状态
afe_run_mode_t g_AfeRunMode = AFE_RUN_MODE_CONTINUOUS;              // AFE运行模式
uint8_t g_ucAfeShotCnt = 0;                                         // AFE猝发剩余次数
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
*                           陈苏阳@2024-08-01
* Function Name  :  update_vol_offset
* Description    :  更新电压偏移
* Input          :  int16_t sVolOffset
* Output         :  None
* Return         :  void
*******************************************************************************/
void update_vol_offset(int16_t sVolOffset)
{

}
/*******************************************************************************
*                           陈苏阳@2023-11-15
* Function Name  :  afe_is_working
* Description    :  AFE是否正在工作
* Input          :  void
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool afe_is_working(void)
{
    return g_bAfeisWorking;
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
    log_d("afe_init");

    g_bAfeisWorking = false;
#if AFE_USE_BMS003
    // 初始化bms003
    bms003_init();
#endif
#if AFE_USE_LTCGM1272
    // 初始化LTCGM1272
    ltcgm1272_init();
#endif
}


/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  afe_stop
* Description    :  AFE停止测量
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void afe_stop(void)
{
    log_d("afe_stop");
#if AFE_USE_BMS003
    // bms003停止工作
    bms003_stop();
#endif
#if AFE_USE_LTCGM1272
    // LTCGM1272停止工作
    ltcgm1272_stop();
#endif
    g_bAfeisWorking = false;
}

/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  afe_start
* Description    :  AFE开始测量
* Input          :  afe_run_mode_t RunMode
* Output         :  None
* Return         :  void
*******************************************************************************/
void afe_start(afe_run_mode_t RunMode)
{
    log_d("afe_start");
#if AFE_USE_BMS003
    // bms003开始工作
    bms003_start(RunMode);
#endif
#if AFE_USE_LTCGM1272
    // LTCGM1272开始工作
    ltcgm1272_start(RunMode);
#endif

    g_bAfeisWorking = true;
}



/*******************************************************************************
*                           陈苏阳@2024-06-25
* Function Name  :  afe_shot
* Description    :  AFE多次采集
* Input          :  uint8_t ucSampleingCnt
* Output         :  None
* Return         :  void
*******************************************************************************/
void afe_shot(uint8_t ucSampleingCnt)
{
    log_d("afe_shot");
#if AFE_USE_BMS003
    // bms003计次工作
    bms003_shot(ucSampleingCnt);
#endif
#if AFE_USE_LTCGM1272
    
#endif
}

/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  afe_new_data_is_ready
* Description    :  是否有新数据
* Input          :  void
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool afe_new_data_is_ready(void)
{
#if AFE_USE_BMS003
    return bms003_new_data_is_ready();
#endif

#if AFE_USE_LTCGM1272
    return ltcgm1272_new_data_is_ready();
#endif
}


/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  afe_get_current_electric_current
* Description    :  获取当前电流
* Input          :  void
* Output         :  None
* Return         :  double
*******************************************************************************/
double afe_get_current_electric_current(void)
{
    double fAdcK = 1.000;
    double fAdcB = 0.000;
    if (g_PrmDb.AdcK != 0 && g_PrmDb.AdcK > 800 && g_PrmDb.AdcK < 1200)
    {
        fAdcK = (double)g_PrmDb.AdcK / 1000.0;
    }
    if (g_PrmDb.AdcB != 0 && g_PrmDb.AdcB > -10000 && g_PrmDb.AdcB < 10000)
    {
        fAdcB = (double)g_PrmDb.AdcB / 1000.0;
    }
    // 返回当前电流
    return fAdcK * g_fCurrElectricCurrent + fAdcB;
}


/*******************************************************************************
*                           陈苏阳@2023-11-15
* Function Name  :  afe_get_new_data
* Description    :  获取AFE中测量到的最新电流(nA)
* Input          :  double * pNewData
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool afe_get_new_data(double* pNewData)
{
#if AFE_USE_BMS003
    bool Ret =  bms003_get_new_data(&g_fCurrElectricCurrent);
    if (pNewData && Ret)*pNewData = afe_get_current_electric_current();
#endif

#if AFE_USE_LTCGM1272
    bool Ret = ltcgm1272_get_new_data(&g_fCurrElectricCurrent);
    if (pNewData && Ret)*pNewData = afe_get_current_electric_current();
#endif
    return Ret;
}


/*******************************************************************************
*                           陈苏阳@2023-11-15
* Function Name  :  afe_register_irq_callback
* Description    :  注册中断回调
* Input          :  afe_irq_callback callback
* Output         :  None
* Return         :  void
*******************************************************************************/
void afe_register_irq_callback(afe_irq_callback callback)
{
#if AFE_USE_BMS003
    bms003_register_irq_callback(callback);
#endif

#if AFE_USE_LTCGM1272
    ltcgm1272_register_irq_callback(callback);
#endif
}


/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




