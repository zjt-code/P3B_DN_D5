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
#include "em_wdog.h"
#include "stdio.h"
#include "pin_config.h"
#include "app_glucose_meas.h"
#include "cgms_prm.h"
#include "cgms_debug_db.h"
#include <cm_backtrace.h>
#include "em_rmu.h"
#include "em_msc.h"
#include "em_emu.h"
/* Private variables ---------------------------------------------------------*/
uint32_t g_uiRstCause;
uint8_t g_ucWatchdogTriggerFlag = 0;
/* Private function prototypes -----------------------------------------------*/
void wdog_init(void);

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
    // 记录复位原因并清空复位原因
    g_uiRstCause  = RMU_ResetCauseGet();
    RMU_ResetCauseClear();

    // 系统初始化
    sl_system_init();
    MSC_Init();

    // 初始化GPIO的时钟
    CMU_ClockEnable(cmuClock_GPIO, true);
    // 设置MCU_POWER_LOCK引脚为推挽输出
    GPIO_PinModeSet(MCU_POWER_LOCK_PORT, MCU_POWER_LOCK_PIN, gpioModePushPull, 1);
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

    wdog_init();

    // 这边只做了基本的初始化,固件的入口在:app_global.c里的app_init()函数

    while (1)
    {
        WDOGn_Feed(WDOG0);
        sl_system_process_action();
        sl_power_manager_sleep();
    }
}


/*******************************************************************************
*                           陈苏阳@2024-04-15
* Function Name  :  wdog_init
* Description    :  初始化看门狗
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void wdog_init(void)
{
    // Enable clock for the WDOG module; has no effect on xG21
    CMU_ClockEnable(cmuClock_WDOG0, true);

    WDOG_Init_TypeDef wdogInit = WDOG_INIT_DEFAULT;
    wdogInit.debugRun = true;
    wdogInit.em3Run = true;
    wdogInit.perSel = wdogPeriod_16k; //~ 16 seconds period
    wdogInit.warnSel = wdogWarnTime75pct; // Interrupt at 25% of the timeout period
    /* Initializing watchdog with chosen settings */
    WDOGn_Init(WDOG0, &wdogInit);
    WDOGn_IntEnable(WDOG0, WDOG_IEN_WARN);
    /* Clear and enable interrupts */
    NVIC_ClearPendingIRQ(WDOG0_IRQn);
    NVIC_EnableIRQ(WDOG0_IRQn);
}
/*******************************************************************************
*                           陈苏阳@2024-04-15
* Function Name  :  WDOG0_IRQHandler
* Description    :  看门狗中断
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void WDOG0_IRQHandler(void)
{
    //WDOGn_IntClear(WDOG0, WDOG_IEN_WARN);
    if (g_ucWatchdogTriggerFlag == 0)
    {
        uint32_t call_stack[14] = { 0 };
        size_t i, depth = 0;
        /* 获取当前环境下的函数调用栈，每个元素将会以 32 位地址形式存储， depth 为函数调用栈实际深度 */
        depth = cm_backtrace_call_stack(call_stack, sizeof(call_stack), cmb_get_sp());
        /* 输出当前函数调用栈信息
         * 注意：查看函数名称及具体行号时，需要使用 addr2line 工具转换
         */
        log_i("backtrace_call_stack(%d):", depth);
        // 清除所有键值对
        cgms_debug_clear_all_kv();
        for (i = 0; i < depth; i++) 
        {
            elog_raw_output("%08x ", call_stack[i]);
            char TmpKey[12];
            sprintf(TmpKey,"BCS%02d",i);
            // 先将键值对写到内存
            cgms_debug_write_kv(TmpKey, (uint8_t*)&call_stack[i]);
        }

        // 写出到flash
        cgms_debug_db_write();
        sl_sleeptimer_delay_millisecond(1000);
        NVIC_SystemReset();

    }
    g_ucWatchdogTriggerFlag = 1;
}

void NMI_Handler(void)
{
    uint32_t call_stack[14] = { 0 };
    size_t i, depth = 0;
    /* 获取当前环境下的函数调用栈，每个元素将会以 32 位地址形式存储， depth 为函数调用栈实际深度 */
    depth = cm_backtrace_call_stack(call_stack, sizeof(call_stack), cmb_get_sp());
    log_i("NMI_Handler");
    log_i("backtrace_call_stack(%d):", depth);
    // 清除所有键值对
    cgms_debug_clear_all_kv();
    for (i = 0; i < depth; i++)
    {
        elog_raw_output("%08x ", call_stack[i]);
        char TmpKey[12];
        sprintf(TmpKey, "BCS%02d", i);
        // 先将键值对写到内存
        cgms_debug_write_kv(TmpKey, (uint8_t*)&call_stack[i]);
    }

    // 写出到flash
    cgms_debug_db_write();

    while (1);
}


void BusFault_Handler(void)
{
    uint32_t call_stack[14] = { 0 };
    size_t i, depth = 0;
    /* 获取当前环境下的函数调用栈，每个元素将会以 32 位地址形式存储， depth 为函数调用栈实际深度 */
    depth = cm_backtrace_call_stack(call_stack, sizeof(call_stack), cmb_get_sp());
    log_i("BusFault_Handler");
    log_i("backtrace_call_stack(%d):", depth);
    // 清除所有键值对
    cgms_debug_clear_all_kv();
    for (i = 0; i < depth; i++)
    {
        elog_raw_output("%08x ", call_stack[i]);
        char TmpKey[12];
        sprintf(TmpKey, "BCS%02d", i);
        // 先将键值对写到内存
        cgms_debug_write_kv(TmpKey, (uint8_t*)&call_stack[i]);
    }

    // 写出到flash
    cgms_debug_db_write();

    while (1);
}

void UsageFault_Handler(void)
{
    uint32_t call_stack[14] = { 0 };
    size_t i, depth = 0;
    /* 获取当前环境下的函数调用栈，每个元素将会以 32 位地址形式存储， depth 为函数调用栈实际深度 */
    depth = cm_backtrace_call_stack(call_stack, sizeof(call_stack), cmb_get_sp());
    log_i("UsageFault_Handler");
    log_i("backtrace_call_stack(%d):", depth);
    // 清除所有键值对
    cgms_debug_clear_all_kv();
    for (i = 0; i < depth; i++)
    {
        elog_raw_output("%08x ", call_stack[i]);
        char TmpKey[12];
        sprintf(TmpKey, "BCS%02d", i);
        // 先将键值对写到内存
        cgms_debug_write_kv(TmpKey, (uint8_t*)&call_stack[i]);
    }

    // 写出到flash
    cgms_debug_db_write();

    while (1);
}

void ecureFault_Handler(void)
{
    uint32_t call_stack[14] = { 0 };
    size_t i, depth = 0;
    /* 获取当前环境下的函数调用栈，每个元素将会以 32 位地址形式存储， depth 为函数调用栈实际深度 */
    depth = cm_backtrace_call_stack(call_stack, sizeof(call_stack), cmb_get_sp());
    log_i("ecureFault_Handler");
    log_i("backtrace_call_stack(%d):", depth);
    // 清除所有键值对
    cgms_debug_clear_all_kv();
    for (i = 0; i < depth; i++)
    {
        elog_raw_output("%08x ", call_stack[i]);
        char TmpKey[12];
        sprintf(TmpKey, "BCS%02d", i);
        // 先将键值对写到内存
        cgms_debug_write_kv(TmpKey, (uint8_t*)&call_stack[i]);
    }

    // 写出到flash
    cgms_debug_db_write();

    while (1);
}
/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/





