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

#include "pin_config.h"
#include "app_glucose_meas.h"
#include "cgms_prm.h"
#include "cgms_debug_db.h"
#include <cm_backtrace.h>
#include "em_rmu.h"
#include "em_msc.h"
/* Private variables ---------------------------------------------------------*/
uint32_t rest_dig_status;
uint32_t acs_reset_status;
uint8_t g_ucWatchdogTriggerFlag = 0;
/* Private function prototypes -----------------------------------------------*/
void wdog_init(void);
extern uint8_t cgms_db_flash_init(void);

/* Private functions ---------------------------------------------------------*/

void fault_test_by_div0(void) {
    volatile int * SCB_CCR = (volatile int *) 0xE000ED14; // SCB->CCR
    int x, y, z;

    *SCB_CCR |= (1 << 4); /* bit4: DIV_0_TRP. */

    x = 10;
    y = 0;
    z = x / y;
    printf("z:%d\n", z);
}

/*******************************************************************************
*                           陈苏阳@2024-04-17
* Function Name  :  print_reset_cause
* Description    :  打印复位原因
* Input          :  uint32_t uiResetCause
* Output         :  None
* Return         :  void
*******************************************************************************/
void print_reset_cause(uint32_t uiResetCause)
{
 if(uiResetCause & EMU_RSTCAUSE_POR)
 {
     log_i("ResetCause:Power On Reset");
 }
 if (uiResetCause & EMU_RSTCAUSE_PIN)
 {
     log_i("ResetCause:Pin Reset");
 }
 if (uiResetCause & EMU_RSTCAUSE_EM4)
 {
     log_i("ResetCause:EM4 Wakeup Reset");
 }
 if (uiResetCause & EMU_RSTCAUSE_WDOG0)
 {
     log_i("ResetCause:Watchdog 0 Reset");
 }
 if (uiResetCause & EMU_RSTCAUSE_LOCKUP)
 {
     log_i("ResetCause:M33 Core Lockup Reset");
 }
 if (uiResetCause & EMU_RSTCAUSE_SYSREQ)
 {
     log_i("ResetCause:M33 Core Sys Reset");
 }
 if (uiResetCause & EMU_RSTCAUSE_DVDDBOD)
 {
     log_i("ResetCause:Shift value for EMU_DVDDBOD");
 }
 if (uiResetCause & EMU_RSTCAUSE_DVDDLEBOD)
 {
     log_i("ResetCause:LEBOD Reset");
 }
 if (uiResetCause & EMU_RSTCAUSE_DECBOD)
 {
     log_i("ResetCause:LVBOD Reset");
 }
 if (uiResetCause & EMU_RSTCAUSE_AVDDBOD)
 {
     log_i("ResetCause:LEBOD1 Reset");
 }
 if (uiResetCause & EMU_RSTCAUSE_IOVDD0BOD)
 {
     log_i("ResetCause:LEBOD2 Reset");
 }
 if (uiResetCause & EMU_RSTCAUSE_DCI)
 {
     log_i("ResetCause:DCI reset");
 }
 if (uiResetCause & EMU_RSTCAUSE_BOOSTON)
 {
     log_i("ResetCause:BOOST_EN pin reset");
 }
 if (uiResetCause & EMU_RSTCAUSE_VREGIN)
 {
     log_i("ResetCause:DCDC VREGIN comparator");
 }
}

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
    rest_dig_status = RMU_ResetCauseGet();
    RMU_ResetCauseClear();

    // 系统初始化
    sl_system_init();

    MSC_Init();

    sl_sleeptimer_delay_millisecond(1000);


    // 初始化GPIO的时钟
    CMU_ClockEnable(cmuClock_GPIO, true);
    GPIO_PinModeSet(gpioPortC, 0, gpioModePushPull, 0);
    GPIO_PinModeSet(gpioPortC, 1, gpioModePushPull, 0);
    GPIO_PinModeSet(gpioPortC, 2, gpioModePushPull, 0);
    GPIO_PinModeSet(gpioPortC, 5, gpioModePushPull, 0);

    GPIO_PinModeSet(gpioPortA, 0, gpioModePushPull, 0);
    GPIO_PinModeSet(gpioPortA, 4, gpioModePushPull, 0);
    GPIO_PinModeSet(gpioPortA, 6, gpioModePushPull, 0);

    GPIO_PinModeSet(gpioPortB, 0, gpioModePushPull, 0);


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
    log_i("sys init  ver:%s", SOFT_VER);
    log_i("ResetCause:0x%x", rest_dig_status);
    print_reset_cause(rest_dig_status);

    wdog_init();
    cm_backtrace_init("P3A", "0.0.3", "0.0.3");
    // 初始化历史数据的flash接口
    cgms_db_flash_init();

    // 打印debug信息
    cgms_debug_db_print();

    // 清空现有的键值对
    cgms_debug_clear_all_kv();

    // 参数存储上电初始化
    cgms_prm_db_power_on_init();

    WDOGn_Feed(WDOG0);

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
    uint32_t call_stack[14] = { 0 };
    size_t i, depth = 0;
    /* 获取当前环境下的函数调用栈，每个元素将会以 32 位地址形式存储， depth 为函数调用栈实际深度 */
    depth = cm_backtrace_call_stack(call_stack, sizeof(call_stack), cmb_get_sp());
    log_i("WDOG0_IRQHandler");
    //WDOGn_IntClear(WDOG0, WDOG_IEN_WARN);
    if (g_ucWatchdogTriggerFlag == 0)
    {
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
    }
    g_ucWatchdogTriggerFlag = 1;
}

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/





