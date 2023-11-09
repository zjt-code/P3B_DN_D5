/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  temp_sensor.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  6/11/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#if !defined(LOG_TAG)
#define LOG_TAG                   "Temp sensor"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_INFO


#include "em_device.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_chip.h"
#include "em_gpio.h"
#include "em_timer.h"
#include "temp_sensor.h"
#include "pin_config.h"
#include <elog.h>
#include "app_global.h"
#include "sl_power_manager.h"
/* Private variables ---------------------------------------------------------*/
int16_t g_sCurTemp = 0;                             // 当前温度 
bool g_bNewTempDataFlag = false;                    // 当前是否有新温度数据
bool g_bTempSensorIsBusyFlag = false;               // 温湿度传感器是否正忙(不能休眠)
sl_sleeptimer_timer_handle_t g_TempSensorReadTimer;
sl_sleeptimer_timer_handle_t g_TempSensorReadEndTimer;


#define TEMP_SENSOR_MEAS_TIME                       (20)               // 温度传感器转换时间
#define TEMP_SENSOR_READ_TIME                       (42+16)            // 温度传感器读取时间
/* Private function prototypes -----------------------------------------------*/
void temp_sensor_read_timer_handler(void);
void temp_sensor_read_end_timer_handler(void);
void temp_sensor_read_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data);
void temp_sensor_read_end_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data);
/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  temp_sensor_init
* Description    :  初始化温度传感器
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void temp_sensor_init(void)
{
    log_d("temp_sensor_init\n");
    g_bTempSensorIsBusyFlag = false;
    g_bNewTempDataFlag = false;
    CMU_ClockEnable(cmuClock_TIMER0, true);
    GPIO_PinModeSet(TEMP_SENSOR_PORT, TEMP_SENSOR_PIN,gpioModeInputPullFilter, 0);
    GPIO_PinModeSet(TEMP_SENSOR_EN_PORT, TEMP_SENSOR_EN_PIN, gpioModePushPull, 0);

    // 路由引脚到TIMER0捕获/比较通道1并使能
    GPIO->TIMERROUTE[0].CC1ROUTE = (TEMP_SENSOR_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)  | (TEMP_SENSOR_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
    GPIO->TIMERROUTE[0].ROUTEEN = GPIO_TIMER_ROUTEEN_CC1PEN;

    // 添加事件
    event_add(MAIN_lOOP_EVENT_TEMP_SENSOR_READ_START_TIMER, temp_sensor_read_timer_handler);
    event_add(MAIN_lOOP_EVENT_TEMP_SENSOR_READ_END_TIMER, temp_sensor_read_end_timer_handler);

    // 初始化TIMER0
    TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;

    // 选择定时器的时钟源为CC1
    timerInit.clkSel = timerClkSelCC1;

    // 时钟不分频
    timerInit.prescale = timerPrescale1;
    timerInit.enable = false;

    TIMER_Init(TIMER0, &timerInit);

}

/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  temp_sensor_read_timer_callback
* Description    :  温度传感器触发读取定时器回调
* Input          :  sl_sleeptimer_timer_handle_t * handle
* Input          :  void * data
* Output         :  None
* Return         :  void
*******************************************************************************/
void temp_sensor_read_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data)
{
    log_d("temp_sensor_read_timer_callback\n");
    // 事件推送
    event_push(MAIN_lOOP_EVENT_TEMP_SENSOR_READ_START_TIMER);
}


/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  temp_sensor_read_end_timer_callback
* Description    :  温度传感器读取结束定时器回调
* Input          :  sl_sleeptimer_timer_handle_t * handle
* Input          :  void * data
* Output         :  None
* Return         :  void
*******************************************************************************/
void temp_sensor_read_end_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data)
{   
    log_d("temp_sensor_read_end_timer_callback\n");
    // 事件推送
    event_push(MAIN_lOOP_EVENT_TEMP_SENSOR_READ_END_TIMER);
}


/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  temp_sensor_read_timer_handler
* Description    :  温度传感器触发读取定时器处理
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void temp_sensor_read_timer_handler(void)
{
    log_d("temp_sensor_read_timer_handler\n");
    // 清空计数
    TIMER_CounterSet(TIMER0, 0);
    
    // 清除中断
    TIMER_IntClear(TIMER0, TIMER_IntGet(TIMER0));

    // 使能定时器
    TIMER_Enable(TIMER0, true);

    // 温度传感器正忙标志位置位
    g_bTempSensorIsBusyFlag = true;

    // 添加EM规则,只允许休眠到EM1
    sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);

    // 等待读取时间结束后做后处理
    sl_sleeptimer_start_timer(&g_TempSensorReadEndTimer, sl_sleeptimer_ms_to_tick(TEMP_SENSOR_READ_TIME), temp_sensor_read_end_timer_callback, (void*)NULL, 0, 0);

}

/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  temp_sensor_read_end_timer_handler
* Description    :  
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void temp_sensor_read_end_timer_handler(void)
{
    log_d("temp_sensor_read_end_timer_handler\n");
    uint16_t usCounter = TIMER_CounterGet(TIMER0);

    // 计算温度
    g_sCurTemp = (int16_t)((usCounter*0.0625 - 50.0625)*100);

    // 失能温度传感器
    GPIO_PinOutClear(TEMP_SENSOR_EN_PORT, TEMP_SENSOR_EN_PIN);

    // 移除EM规则,允许休眠到EM2
    sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);

    log_i("temp:%.3f C\n", g_sCurTemp/100.0f);

    // 新数据标志位
    g_bNewTempDataFlag = true;

    // 温度传感器正忙标志位解除
    g_bTempSensorIsBusyFlag = false;
}


/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  temp_sensor_start_meas
* Description    :  温度传感器开始转换
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void temp_sensor_start_meas(void)
{
    log_d("temp_sensor_start_meas\n");
    // 使能温度传感器
    GPIO_PinOutSet(TEMP_SENSOR_EN_PORT, TEMP_SENSOR_EN_PIN);

    // 转换时间过去后触发读取
    sl_sleeptimer_start_timer(&g_TempSensorReadTimer, sl_sleeptimer_ms_to_tick(TEMP_SENSOR_MEAS_TIME), temp_sensor_read_timer_callback, (void*)NULL, 0, 0);
    
}


/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  temp_sensor_get_temp
* Description    :  获取温度
* Input          :  int16_t * pTemp
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool temp_sensor_get_temp(int16_t* pTemp)
{
    if (pTemp)*pTemp = g_sCurTemp;

    if (g_bNewTempDataFlag)
    {
        g_bNewTempDataFlag = false;
        return true;
    }
    return false;
}



/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  temp_sensor_is_busy
* Description    :  温度传感器是否正忙
* Input          :  void
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool temp_sensor_is_busy(void)
{
    return g_bTempSensorIsBusyFlag;
}


/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




