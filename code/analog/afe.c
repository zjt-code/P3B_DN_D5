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
#include "sl_sleeptimer.h"
#include <elog.h>
/* Private variables ---------------------------------------------------------*/
double g_fCurrElectricCurrent = 0.00;               // 当前电流
bool g_bAfeisWorking = false;                       // AFE工作状态
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/


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
#if 0
    // 初始化bms003
    bms003_init();

#else
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
#if 0
    // bms003停止工作
    bms003_stop();
#else
    // LTCGM1272停止工作
    ltcgm1272_stop();
#endif
    g_bAfeisWorking = false;
}

/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  afe_start
* Description    :  AFE开始测量
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void afe_start(void)
{
    log_d("afe_start");
#if 0
    // bms003开始工作
    bms003_start();

#else
    // LTCGM1272开始工作
    ltcgm1272_start();
#endif

    g_bAfeisWorking = true;
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
#if 0
    return bms003_new_data_is_ready();
#else
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
    // 返回当前电流
    return g_fCurrElectricCurrent;
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
#if 0
    bool Ret =  bms003_get_new_data(&g_fCurrElectricCurrent);
    if (pNewData && Ret)*pNewData = g_fCurrElectricCurrent;
#else
    bool Ret = ltcgm1272_get_new_data(&g_fCurrElectricCurrent);
    if (pNewData && Ret)*pNewData = g_fCurrElectricCurrent;
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
#if 0
    bms003_register_irq_callback(callback);
#else
    ltcgm1272_register_irq_callback(callback);
#endif
}


/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




