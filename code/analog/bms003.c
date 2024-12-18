/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  bms003.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  26/10/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#if !defined(LOG_TAG)
#define LOG_TAG                "BMS003"
#endif
#undef LOG_LVL
#define LOG_LVL                ELOG_LVL_DEBUG

#include "afe.h"
#ifdef AFE_USE_BMS003
#include "bms003.h"
#include "spidrv.h"
#include "sl_udelay.h"
#include "sl_spidrv_instances.h"
#include "cgms_prm.h"
#include "cmsis_gcc.h"
#include "pin_config.h"
#include <elog.h>
#include "gpiointerrupt.h"
#include "app_global.h"
#include "fifo.h"
#include "string.h"
#include "sl_spidrv_usart_AfeSpiInst_config.h"
#include "bms003_bist_if.h"
/* Private variables ---------------------------------------------------------*/

// 反馈
#define CH1_WE1_RFB_SEL                             0x39
// 增益
#define CH1_WE1_VGAIN_SEL                           0x00
// 偏置
#define CH1_DINWE_L8                                0x96
#define CH1_DINWE_H2                                0x00

// 修改ICLK&PCLK&CIC
#define CLK                                         0x1E
#define CIC                                         0x31


#define SLEEP_TIMER_INTERVAL                        (10*1000)
#define SHOT_SLEEP_TIMER_INTERVAL                   (2970)
static uint8_t g_ucBms003NewDataFlag = 0;                           // BMS003有新数据标志位
static uint32_t g_Bms003IrqInterrupt;                               // BMS003中断引脚的中断号
static bms003_irq_callback g_Bms003IrqCallbackFun = NULL;           // 中断回调函数
static fifo_t g_NewDataFifo;                                        // 新数据fifo
static double g_fNewDataFifoBuffer[48];                             // 新数据fifo所使用的buffer
static uint32_t g_uiChipEnTime = 0;                                 // 芯片使能时间
static bool g_bWakeupFlag = false;                                  // 唤醒标志位
static uint16_t ucIrqCnt = 0;                                       // 中断计数
static uint16_t ucOnePeriodSampCnt = 0;                             // 单周期采样次数计数
static uint16_t g_BaseWeVol = 0;                                    // WE1校准电压
static uint8_t g_ucSampleingCnt = 0;                                // 采样次数
static afe_run_mode_t g_AfeRunMode = AFE_RUN_MODE_CONTINUOUS;       // AFE运行模式
static double g_fRawCurrent = 0.0;                                  // AFE中直接读出的原始电流
static bool g_bFirstDataFlag = 0;                                   // 第一个数据标志位
static double g_fAfeTemp = 30.0;                                    // AFE测量的温度
static bool g_bAfeTempMeasFlag = true;                              // AFE是否触发测量温度标志位
sl_sleeptimer_timer_handle_t g_Bms003WakeupTimer;
sl_sleeptimer_timer_handle_t g_Bms003MeasureTimer;
sl_sleeptimer_timer_handle_t g_Bms003InitTimer;
/* Private function prototypes -----------------------------------------------*/
uint8_t bms003_read_cycle(uint8_t ucRegAddr);
void bms003_read_burst(uint8_t ucRegAddr, uint8_t* pData, uint8_t ucLen);
void bms003_write_burst(uint8_t ucRegAddr, uint8_t* pData,uint8_t ucLen);
void bms003_write_cycle(uint8_t ucRegAddr, uint8_t ucData);
void bms003_int_irq_callback(uint8_t intNo, void* ctx);
void bms003_measure_timer_handler(void);
void bms003_wakeup_timer_handler(void);
void bms003_init_timer_handler(void);
void bms003_start(afe_run_mode_t RunMode);
void bms003_stop(void);
void bms003_wakeup(void);
void bms003_sleep(void);
void bms003_int_irq_handler(void);
void bms003_booting_config(void);
void bms003_wakeup_config(void);
void bms003_booting_temp_config(void);
/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           陈苏阳@2023-11-15
* Function Name  :  bms003_get_new_data
* Description    :  获取新数据
* Input          :  double * pNewData
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool bms003_get_new_data(double* pNewData)
{
    double fData;
    if (pNewData == NULL)return false;
    if (fifo_out(&g_NewDataFifo, &fData, 1, 1))
    {
        *pNewData = fData;
        return true;
    }
    return false;
}


/*******************************************************************************
*                           陈苏阳@2023-11-15
* Function Name  :  bms003_register_irq_callback
* Description    :  注册中断回调
* Input          :  bms003_irq_callback callback
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_register_irq_callback(bms003_irq_callback callback)
{
    g_Bms003IrqCallbackFun = callback;
}



/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  bms003_new_data_is_ready
* Description    :  是否有新数据
* Input          :  void
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool bms003_new_data_is_ready(void)
{
    return fifo_len(&g_NewDataFifo) ? true : false;
}

/*******************************************************************************
*                           陈苏阳@2023-10-26
* Function Name  :  bms003_delay_us
* Description    :  延时
* Input          :  uint32_t_t uiUs
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_delay_us(uint32_t uiUs)
{
    if (uiUs >= 1000)
    {
        sl_sleeptimer_delay_millisecond(uiUs/1000);
    }
    else
    {
        sl_udelay_wait(uiUs);
    }
}
/*******************************************************************************
*                           陈苏阳@2023-10-26
* Function Name  :  bms003_spi_transfer
* Description    :  BMS003 SPI读写
* Input          :  uint8_t * pTxBuffer
* Input          :  uint8_t * pRxBuffer
* Input          :  uint8_t ucLen
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_spi_transfer(uint8_t* pTxBuffer,uint8_t* pRxBuffer,uint8_t ucLen)
{
    SPIDRV_MTransferB(sl_spidrv_eusart_AfeSpiInst_handle, pTxBuffer, pRxBuffer, ucLen);
}

/*******************************************************************************
*                           陈苏阳@2023-10-26
* Function Name  :  bms003_spi_write_data
* Description    :  BMS003 SPI写
* Input          :  uint8_t * pTxBuffer
* Input          :  uint8_t ucLen
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_spi_write_data(uint8_t* pTxBuffer, uint8_t ucLen)
{
    SPIDRV_MTransmitB(sl_spidrv_eusart_AfeSpiInst_handle, pTxBuffer, ucLen);
}

/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  bms003_enable
* Description    :  BMS003使能
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_enable(void)
{
    log_d("bms003_enable");
    // 使能bms0003
    GPIO_PinOutSet(AFE_CHIP_EN_PORT, AFE_CHIP_EN_PIN);

}


/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  bms003_disable
* Description    :  BMS003失能
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_disable(void)
{
    log_d("bms003_disable");
    // 失能bms0003
    GPIO_PinOutClear(AFE_CHIP_EN_PORT, AFE_CHIP_EN_PIN);

}


/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  bms003_timer_callback
* Description    :  bms003定时器回调
* Input          :  sl_sleeptimer_timer_handle_t * handle
* Input          :  void * data
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_wakeup_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data)
{
    // 如果是猝发模式,则管理采样次数
    if (g_AfeRunMode == AFE_RUN_MODE_SHOT)
    {
        if (g_ucSampleingCnt)
        {
            g_ucSampleingCnt--;
        }
        else
        {
            sl_sleeptimer_stop_timer(&g_Bms003WakeupTimer);
            return;
        }
    }

    log_d("bms003_wakeup_timer_callback");
    // 发送事件
    event_push(MAIN_LOOP_EVENT_AFE_WAKEUP_TIMER, NULL);
}

/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  bms003_measure_timer_callback
* Description    :  bms003定时器回调
* Input          :  sl_sleeptimer_timer_handle_t * handle
* Input          :  void * data
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_measure_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data)
{
    log_d("bms003_measure_timer_callback");
    // 发送事件
    event_push(MAIN_LOOP_EVENT_AFE_MEASURE_TIMER, NULL);
}


/*******************************************************************************
*                           陈苏阳@2024-11-29
* Function Name  :  bms003_init_timer_callback
* Description    :  bms003初始化定时器回调
* Input          :  sl_sleeptimer_timer_handle_t * handle
* Input          :  void * data
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_init_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data)
{
    log_d("bms003_init_timer_callback");
    // 发送事件
    event_push(MAIN_LOOP_EVENT_AFE_INIT_DELAY_TIMER, NULL);
}

/*******************************************************************************
*                           陈苏阳@2024-11-29
* Function Name  :  bms003_init_timer_handler
* Description    :  BMS003初始化软件定时器
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_init_timer_handler(void)
{
    // 使能BMS003
    bms003_enable();
    g_uiChipEnTime = rtc_get_curr_time();
}


/*******************************************************************************
*                           陈苏阳@2023-10-26
* Function Name  :  bms003_init
* Description    :  BMS003初始化
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_init(void)
{
    log_d("bms003_init");
    // 设置CS引脚为推挽输出
    GPIO_PinModeSet(SPI_CS_PORT, SPI_CS_PIN, gpioModePushPull, 1);

    // 设置AFE_CHIP_EN引脚为推挽输出
    GPIO_PinModeSet(AFE_CHIP_EN_PORT, AFE_CHIP_EN_PIN, gpioModePushPull, 0);

    // 设置AFE_WAKEUP引脚为推挽输出
    GPIO_PinModeSet(AFE_WAKE_UP_PORT, AFE_WAKE_UP_PIN, gpioModePushPull, 1);

    // 设置AFE的INT引脚下拉输入
    GPIO_PinModeSet(AFE_INT_PORT, AFE_INT_PIN, gpioModeInput, 0);

    // CS拉高
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    // 配置中断处理函数
    g_Bms003IrqInterrupt = GPIOINT_CallbackRegisterExt(AFE_INT_PIN, bms003_int_irq_callback, NULL);
    log_d("g_Bms003IrqInterrupt:%d", g_Bms003IrqInterrupt);
    // 添加事件
    event_add(MAIN_LOOP_EVENT_AFE_MEASURE_TIMER, bms003_measure_timer_handler);
    event_add(MAIN_LOOP_EVENT_AFE_WAKEUP_TIMER, bms003_wakeup_timer_handler);
    event_add(MAIN_LOOP_EVENT_AFE_INIT_DELAY_TIMER, bms003_init_timer_handler);
    event_add(MAIN_LOOP_EVENT_AFE_IRQ, bms003_int_irq_handler);
    // 创建fifo
    fifo_create(&g_NewDataFifo, g_fNewDataFifoBuffer, sizeof(g_fNewDataFifoBuffer[0]), sizeof(g_fNewDataFifoBuffer) / sizeof(g_fNewDataFifoBuffer[0]));

    // 失能BMS003
    bms003_disable();
    bms003_sleep();
    sl_status_t status;
    // 启动一个200ms后的单次定时器
    status = sl_sleeptimer_start_timer(&g_Bms003InitTimer, sl_sleeptimer_ms_to_tick(200), bms003_init_timer_callback, (void*)NULL, 0, 0);
    if (status != SL_STATUS_OK)
    {
        log_e("sl_sleeptimer_start_timer failed");
        return;
    }
}       

/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  bms003_measure_timer_handler
* Description    :  BMS003触发测量定时器
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_measure_timer_handler(void)
{
    log_d("bms003_measure_timer_handler");
    // BMS003唤醒配置
    bms003_wakeup_config();
    // 清除GPIO中断
    GPIO_IntClear(g_Bms003IrqInterrupt);

    // 设置AFE的INT引脚中断
    GPIO_ExtIntConfig(AFE_INT_PORT, AFE_INT_PIN, g_Bms003IrqInterrupt, true, false, true);
}

/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  bms003_wakeup_timer_handler
* Description    :  BMS003休眠定时器唤醒回调
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_wakeup_timer_handler(void)
{
    sl_status_t status;
    log_d("bms003_wakeup_timer_handler");
    // 唤醒BMS003
    bms003_wakeup();

    // 启动一个10ms后的单次定时器
    status = sl_sleeptimer_start_timer(&g_Bms003MeasureTimer, sl_sleeptimer_ms_to_tick(10), bms003_measure_timer_callback, (void*)NULL, 0, 0);
    if (status != SL_STATUS_OK)
    {
        log_e("sl_sleeptimer_start_timer failed");
        return;
    }
}

/******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  bms003_start
* Description    :  BMS003开始工作
* Input          :  afe_run_mode_t RunMode
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_start(afe_run_mode_t RunMode)
{
    log_d("bms003_start");
    sl_status_t status;

    // 唤醒bms003
    bms003_wakeup();
    bms003_delay_us(10 * 1000);

    // 获取当前时间与启动时间的时间差
    uint32_t uiTimeDiff = rtc_get_curr_time() - g_uiChipEnTime;

    // 如果BMS003启动时间少于1S,则等待BMS003启动
    if (uiTimeDiff < 1000)
    {
        log_d("uiTimeDiff:%d", uiTimeDiff);
        bms003_delay_us((1000 - uiTimeDiff) * 1000);
    }

    // 设置AFE的INT引脚中断
    GPIO_ExtIntConfig(AFE_INT_PORT, AFE_INT_PIN, g_Bms003IrqInterrupt, true, false, true);

    // 如果是连续采样,则直接开定时器
    if (RunMode == AFE_RUN_MODE_CONTINUOUS)
    {
        status = sl_sleeptimer_start_periodic_timer(&g_Bms003WakeupTimer, sl_sleeptimer_ms_to_tick(SLEEP_TIMER_INTERVAL), bms003_wakeup_timer_callback, (void*)NULL, 0, 0);
        if (status != SL_STATUS_OK)
        {
            log_e("sl_sleeptimer_start_periodic_timer failed");
            return;
        }
    }
    g_AfeRunMode = RunMode;

    // 按初次配置来进行
    g_bWakeupFlag = false;

    // 设置第一次数据标志位
    g_bFirstDataFlag = true;

    // 允许温度采样一次
    g_bAfeTempMeasFlag = true;

    // bms003配置
    bms003_booting_config();
}

/*******************************************************************************
*                           陈苏阳@2024-06-25
* Function Name  :  bms003_shot
* Description    :  BMS003猝发采样
* Input          :  uint8_t ucSampleingCnt
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_shot(uint8_t ucSampleingCnt)
{
    if (g_AfeRunMode == AFE_RUN_MODE_SHOT)
    {
        log_d("bms003_shot(%d)", ucSampleingCnt);
        sl_status_t status;

        // 允许温度采样一次
        g_bAfeTempMeasFlag = true;

        g_ucSampleingCnt = ucSampleingCnt;
        status = sl_sleeptimer_start_periodic_timer(&g_Bms003WakeupTimer, sl_sleeptimer_ms_to_tick(SHOT_SLEEP_TIMER_INTERVAL), bms003_wakeup_timer_callback, (void*)NULL, 0, 0);
        if (status != SL_STATUS_OK)
        {
            log_e("sl_sleeptimer_start_periodic_timer failed");
            return;
        }
    }
}

/*******************************************************************************
*                           陈苏阳@2024-06-26
* Function Name  :  bms003_get_residue_samplingCnt
* Description    :  获取剩余的采样次数
* Input          :  void
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t bms003_get_residue_samplingCnt(void)
{
    return g_ucSampleingCnt;
}

/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  bms003_stop
* Description    :  BMS003停止工作
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_stop(void)
{
    log_d("bms003_stop");
    // 停止定时器
    sl_sleeptimer_stop_timer(&g_Bms003WakeupTimer);

    sl_sleeptimer_stop_timer(&g_Bms003MeasureTimer);

    // 关闭AFE的INT引脚中断
    GPIO_ExtIntConfig(AFE_INT_PORT, AFE_INT_PIN, g_Bms003IrqInterrupt, false, false, false);

    // 清除GPIO中断
    GPIO_IntClear(g_Bms003IrqInterrupt);

    // 清空标志位
    g_ucBms003NewDataFlag = 0;

    // 失能BMS003
    bms003_disable();

    // 休眠bms003
    bms003_sleep();
}

/*******************************************************************************
*                           陈苏阳@2023-10-27
* Function Name  :  bms003_wakeup
* Description    :  唤醒bms003
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_wakeup(void)
{
    log_d("bms003_wakeup");
    GPIO_PinOutSet(AFE_WAKE_UP_PORT, AFE_WAKE_UP_PIN);
}

/*******************************************************************************
*                           陈苏阳@2023-10-27
* Function Name  :  bms003_sleep
* Description    :  休眠bms003
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_sleep(void)
{
    log_d("bms003_sleep");
    GPIO_PinOutClear(AFE_WAKE_UP_PORT, AFE_WAKE_UP_PIN);
}


/*******************************************************************************
*                           陈苏阳@2024-10-18
* Function Name  :  bms003_calc_current_and_send_fifo
* Description    :  计算电流并发送到FIFO
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_calc_current_and_send_fifo(void)
{
    // 校准库校准电流
    double fTmpCail = current_cal(g_fRawCurrent, g_fAfeTemp);

    log_d("fTmpCail:%f fTemp:%f", fTmpCail, g_fAfeTemp);
    if (g_bFirstDataFlag == false)
    {
        log_d("send_fifo");
        fifo_in(&g_NewDataFifo, &fTmpCail, 1);
    }
    else
    {
        g_bFirstDataFlag = false;
    }

    bms003_delay_us(1);

    bms003_write_cycle(0x3A, CLK);

    bms003_delay_us(1);

    bms003_write_cycle(0x50, 0x02);

    bms003_delay_us(1);

    bms003_write_cycle(0x61, 0x00);

    bms003_delay_us(1);

    bms003_write_cycle(0x3A, CLK | 0x80);

    bms003_delay_us(1);
    
    bms003_sleep();
}

/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  bms003_int_irq_handler
* Description    :  BMS003触发引脚中断处理
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_int_irq_handler(void)
{
    log_d("bms003_int_irq_handler");

    // 清除GPIO中断，进入sleep期间BMS3可能会产生一个IO中断
    GPIO_IntClear(g_Bms003IrqInterrupt);

    uint16_t usBuff = 0;      //AD中转存储值
    bms003_delay_us(1);
    bms003_write_cycle(IMEAS_REG_SEQ, 0x01);
    bms003_delay_us(1);

    uint8_t ucCh0Data[2];
    bms003_read_burst(IMEAS_CH0DATA_0, ucCh0Data, 2);
    usBuff = (ucCh0Data[1] << 8) + ucCh0Data[0];
    // 累计中断次数
    ucIrqCnt++;

    log_d("ucIrqCnt:%d  ucOnePeriodSampCnt:%d", ucIrqCnt, ucOnePeriodSampCnt);

    // 在第14次周期时记录WE1
    if (ucIrqCnt == 1)
    {
        g_BaseWeVol = usBuff;
        log_d("g_BaseWeVol:%d", g_BaseWeVol);
        bms003_write_cycle(0x3A, CLK);
        bms003_write_cycle(0x3B, 0x0B);
        bms003_write_cycle(0x61, 0x0d);
        bms003_write_cycle(0x3A, CLK | 0x80);
        bms003_write_cycle(0x61, 0x00);
    }
    // 开始正常采集
    else if (ucIrqCnt >= 2)
    {
        ucIrqCnt = 2;

        // 累计本周期的采样次数
        ucOnePeriodSampCnt++;

        // 如果是本周期的第三次采样,则作为电流的正式数据
        if (ucOnePeriodSampCnt == 3)
        {

            // 获取原始的电流数据
            g_fRawCurrent = ((double)(usBuff - g_BaseWeVol) / 32768.0 * 1.2 * 100);
            log_d("g_fRawCurrent:%f", g_fRawCurrent);

            // 如果需要采集温度
            if (g_bAfeTempMeasFlag)
            {
                g_bAfeTempMeasFlag = false;
                // BMS003重新唤醒并写温度配置
                bms003_sleep();
                bms003_delay_us(10000);
                bms003_wakeup();
                bms003_delay_us(10000);
                bms003_booting_temp_config();
            }
            else
            {
                // 计算电流并发送到FIFO
                bms003_calc_current_and_send_fifo();

                ucOnePeriodSampCnt = 0;
            }
        }
        else if (ucOnePeriodSampCnt >= 4)
        {
            //计算温度值
            g_fAfeTemp = temp_value(usBuff);

            log_i("g_fAfeTemp:%f", g_fAfeTemp);

            // 计算电流并发送到FIFO
            bms003_calc_current_and_send_fifo();

            ucOnePeriodSampCnt = 0;

        }
    }
    //addr:0x4  清除中断状态
    bms003_delay_us(1);
    bms003_write_cycle(IMEAS_INT, 0x01);

}

/*******************************************************************************
*                           陈苏阳@2023-10-30
* Function Name  :  bms003_int_irq_callback
* Description    :  BMS003触发引脚中断处理
* Input          :  uint8_t intNo
* Input          :  void * data
* Output         :  void* ctx
* Return         :  void
*******************************************************************************/
void bms003_int_irq_callback(uint8_t intNo, void* ctx)
{
    // 发送事件
    event_push(MAIN_LOOP_EVENT_AFE_IRQ,NULL);
}


/*******************************************************************************
*                           陈苏阳@2023-10-30
* Function Name  :  bms003_write_cycle
* Description    :  写单个数据
* Input          :  uint8_t ucRegAddr
* Input          :  uint8_t ucData
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_write_cycle(uint8_t ucRegAddr, uint8_t ucData)
{
    uint8_t ucWriteBuffer[4];
    uint8_t ucBufferIndex = 0;
    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ucRegAddr;
    ucWriteBuffer[ucBufferIndex++] = WR_SINGLE_CMD;
    ucWriteBuffer[ucBufferIndex++] = ucData;
    ucWriteBuffer[ucBufferIndex++] = PAD;
    bms003_delay_us(5);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    bms003_delay_us(20);
    bms003_spi_write_data(ucWriteBuffer, ucBufferIndex);
    bms003_delay_us(10);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
    bms003_delay_us(5);
}

/*******************************************************************************
*                           陈苏阳@2023-10-30
* Function Name  :  bms003_write_burst
* Description    :  写多个字节
* Input          :  uint8_t ucRegAddr
* Input          :  uint8_t * pData
* Input          :  uint8_t ucLen
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_write_burst(uint8_t ucRegAddr, uint8_t* pData,uint8_t ucLen)
{
    uint8_t ucWriteBuffer[64];
    uint8_t ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ucRegAddr;
    ucWriteBuffer[ucBufferIndex++] = WR_BURST_CMD;
    for (uint8_t i = 0; i < ucLen; i++)
    {
        ucWriteBuffer[ucBufferIndex++] = pData[i];
    }
    ucWriteBuffer[ucBufferIndex++] = PAD;
    bms003_delay_us(5);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    bms003_delay_us(20);
    bms003_spi_write_data(ucWriteBuffer, ucBufferIndex);
    bms003_delay_us(10);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
    bms003_delay_us(5);
}

/*******************************************************************************
*                           陈苏阳@2023-10-30
* Function Name  :  bms003_read_cycle
* Description    :  读单个数据
* Input          :  uint8_t ucRegAddr
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t bms003_read_cycle(uint8_t ucRegAddr)
{
    uint8_t ucWriteBuffer[3];
    uint8_t ucReadBuffer[3] = {0, 0, 0};
    uint8_t ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ucRegAddr;
    ucWriteBuffer[ucBufferIndex++] = RD_SINGLE_CMD;
    ucWriteBuffer[ucBufferIndex++] = PAD;
    bms003_delay_us(5);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    bms003_delay_us(20);
    bms003_spi_transfer(ucWriteBuffer, ucReadBuffer, 3);
    bms003_delay_us(10);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
    bms003_delay_us(5);
    return ucReadBuffer[2];
}

/*******************************************************************************
*                           陈苏阳@2024-03-18
* Function Name  :  bms003_read_burst
* Description    :  读多个字节
* Input          :  uint8_t ucRegAddr
* Input          :  uint8_t * pData
* Input          :  uint8_t ucLen
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_read_burst(uint8_t ucRegAddr, uint8_t* pData, uint8_t ucLen)
{
    uint8_t ucWriteBuffer[32 + 3];
    uint8_t ucReadBuffer[32 + 3];
    uint8_t ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ucRegAddr;
    ucWriteBuffer[ucBufferIndex++] = RD_BURST_REG_CMD;
    bms003_delay_us(5);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    bms003_delay_us(20);
    bms003_spi_transfer(ucWriteBuffer, ucReadBuffer, ucLen + 3);
    bms003_delay_us(10);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
    bms003_delay_us(5);
    memcpy(pData,&ucReadBuffer[2], ucLen);
}


/*******************************************************************************
*                           陈苏阳@2023-10-27
* Function Name  :  bms003_config
* Description    :  BMS003启动配置
* Input          :  None
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_booting_config(void)
{
    uint8_t ucReadBuffer[32];
    memset(ucReadBuffer, 0x00, sizeof(ucReadBuffer));

    uint8_t ucWriteBuffer[32];
    uint8_t ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CLK;
    ucWriteBuffer[ucBufferIndex++] = 0xb;
    ucWriteBuffer[ucBufferIndex++] = 0x02;
    bms003_write_burst(0x3A, ucWriteBuffer, ucBufferIndex);

    bms003_delay_us(1);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x03;          // addr:0x50 使能BG,DAC[0:1]   // 模拟保持状态关闭BG,DCDC分频
    ucWriteBuffer[ucBufferIndex++] = 0x00;          // addr:0x51 默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x00;          // addr:0x52 默认为0
    ucWriteBuffer[ucBufferIndex++] = CH1_WE1_RFB_SEL;// addr:0x53 配置反馈电阻[2:5]  使能位WE1&DDA [0:1]
    ucWriteBuffer[ucBufferIndex++] = CH1_WE1_VGAIN_SEL;// addr:0x54 配置DDA增益倍数
    ucWriteBuffer[ucBufferIndex++] = 0x00;// addr:0x55 默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x00;// addr:0x56 默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x1;// addr:0x57 参比电极以及辅助电极使能
    ucWriteBuffer[ucBufferIndex++] = 0x1;// addr:0x58 工作电极的偏置电压由第一个DAC生成使能
    uint8_t ucCh1WeL8, ucCH1WeH2;
    bms003_get_fix_ch1_din_we(&ucCh1WeL8, &ucCH1WeH2);
    ucWriteBuffer[ucBufferIndex++] = ucCh1WeL8;// addr:0x59 设置偏置电压[0:7]
    ucWriteBuffer[ucBufferIndex++] = ucCH1WeH2;// addr:0x5A 设置偏置电压[8:9]
    ucWriteBuffer[ucBufferIndex++] = 0x01;// addr:0x5B 参比电极的偏置电压由第二个DAC生成使能[0:7]
    uint8_t ucCh1ReceL8, ucCH1ReceH2;
    bms003_get_fix_ch1_dinRce(&ucCh1ReceL8, ucCH1ReceH2);
    ucWriteBuffer[ucBufferIndex++] = ucCh1ReceL8;// addr:0x5C 配置CE[0:7]
    ucWriteBuffer[ucBufferIndex++] = ucCH1ReceH2;// addr:0x5D 配置CE[8:9]
    bms003_write_burst(0x50, ucWriteBuffer, ucBufferIndex);
    bms003_delay_us(1);
    // 回读校验
    bms003_read_burst(0x53, ucReadBuffer, 2);
    if (ucReadBuffer[0] != CH1_WE1_RFB_SEL || ucReadBuffer[1] != CH1_WE1_VGAIN_SEL)
    {
        log_e("bms003_booting_config half read back check error 0x53:0x%02X   0x54:0x%02X", ucReadBuffer[0], ucReadBuffer[1]);
    }

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ELE_BUF;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    bms003_write_burst(0x61, ucWriteBuffer, ucBufferIndex);

    bms003_write_cycle(0x3A, CLK|0x80);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CIC;
    ucWriteBuffer[ucBufferIndex++] = 0x00;
    ucWriteBuffer[ucBufferIndex++] = CHA_NUM;
    bms003_write_burst(0x01, ucWriteBuffer, ucBufferIndex);

    bms003_write_cycle(IMEAS_REG_SEQ, 0x00);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    bms003_write_burst(0x017, ucWriteBuffer, ucBufferIndex);

    memset(ucReadBuffer, 0x00, sizeof(ucReadBuffer));

    // 回读校验
    bms003_read_burst(0x53, ucReadBuffer, 2);
    if (ucReadBuffer[0] == CH1_WE1_RFB_SEL && ucReadBuffer[1] == CH1_WE1_VGAIN_SEL)
    {
        log_i("bms003_booting_config done");
}
    else
    {
        log_e("bms003_booting_config error 0x53:0x%02X   0x54:0x%02X", ucReadBuffer[0], ucReadBuffer[1]);
    }
}



/*******************************************************************************
*                           陈苏阳@2023-10-27
* Function Name  :  bms003_booting_temp_config
* Description    :  BMS003启动温度测量配置
* Input          :  None
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_booting_temp_config(void)
{
    log_i("bms003_booting_temp_config");
    uint8_t ucWriteBuffer[32];
    bms003_delay_us(1);
    uint8_t ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CLK;
    ucWriteBuffer[ucBufferIndex++] = 0xB;
    ucWriteBuffer[ucBufferIndex++] = 0x2;
    bms003_write_burst(0x3A, ucWriteBuffer, ucBufferIndex);

    bms003_delay_us(3);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x03;          // addr:0x50 使能BG,DAC[0:1]   // 模拟保持状态关闭BG,DCDC分频
#if BMS003_TEMP_DDA_EN
    ucWriteBuffer[ucBufferIndex++] = 0xAF;                      //51默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x01;                      //52默认为0
#else
    ucWriteBuffer[ucBufferIndex++] = 0xAF - 0x80;                      //51默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //52默认为0
#endif
    ucWriteBuffer[ucBufferIndex++] = CH1_WE1_RFB_SEL;// addr:0x53 配置反馈电阻[2:5]  使能位WE1&DDA [0:1]
    ucWriteBuffer[ucBufferIndex++] = CH1_WE1_VGAIN_SEL;// addr:0x54 配置DDA增益倍数
    ucWriteBuffer[ucBufferIndex++] = 0x00;// addr:0x55 默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x00;// addr:0x56 默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x1;// addr:0x57 参比电极以及辅助电极使能
    ucWriteBuffer[ucBufferIndex++] = 0x1;// addr:0x58 工作电极的偏置电压由第一个DAC生成使能
    uint8_t ucCh1WeL8, ucCH1WeH2;

    bms003_get_fix_ch1_din_we(&ucCh1WeL8, &ucCH1WeH2);
    ucWriteBuffer[ucBufferIndex++] = ucCh1WeL8;// addr:0x59 设置偏置电压[0:7]
    ucWriteBuffer[ucBufferIndex++] = ucCH1WeH2;// addr:0x5A 设置偏置电压[8:9]
    ucWriteBuffer[ucBufferIndex++] = 0x01;// addr:0x5B 参比电极的偏置电压由第二个DAC生成使能[0:7]
    uint8_t ucCh1ReceL8, ucCH1ReceH2;
    bms003_get_fix_ch1_dinRce(&ucCh1ReceL8, ucCH1ReceH2);
    ucWriteBuffer[ucBufferIndex++] = ucCh1ReceL8;// addr:0x5C 配置CE[0:7]
    ucWriteBuffer[ucBufferIndex++] = ucCH1ReceH2;// addr:0x5D 配置CE[8:9]
    ucWriteBuffer[ucBufferIndex++] = 0x03;// addr:0x5E
    ucWriteBuffer[ucBufferIndex++] = 0x09;// addr:0x5F
    bms003_write_burst(0x50, ucWriteBuffer, ucBufferIndex);


    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x00;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    bms003_write_burst(0x61, ucWriteBuffer, ucBufferIndex);

    bms003_write_cycle(0x3A, CLK | 0x80);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CIC;
    ucWriteBuffer[ucBufferIndex++] = 0x00;
#if BMS003_TEMP_DDA_EN
    ucWriteBuffer[ucBufferIndex++] = 0x44;
#else
    ucWriteBuffer[ucBufferIndex++] = 0x64;
#endif
    bms003_write_burst(0x01, ucWriteBuffer, ucBufferIndex);

    bms003_write_cycle(IMEAS_REG_SEQ, 0x00);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    bms003_write_burst(0x017, ucWriteBuffer, ucBufferIndex);
}






/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  bms003_wakeup_config
* Description    :  BMS003唤醒配置
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_wakeup_config(void)
{
    uint8_t ucBufferIndex = 0;
    uint8_t ucWriteBuffer[32];
    bms003_delay_us(1);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CLK;                           // 写ICLK为4分频,PCLK8分频,FCLK Dynamic关闭
    ucWriteBuffer[ucBufferIndex++] = 0x0B;
    ucWriteBuffer[ucBufferIndex++] = 0x02;
    bms003_write_burst(0x3A, ucWriteBuffer, ucBufferIndex);

    bms003_delay_us(1);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x03;                      //50使能BG,DAC[0:1]   //模拟保持状态关闭BG，DCDC分频
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //51默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //52默认为0
    ucWriteBuffer[ucBufferIndex++] = CH1_WE1_RFB_SEL;           //53配置反馈电阻[2:5]  使能位WE1&DDA  [0:1]  
    ucWriteBuffer[ucBufferIndex++] = CH1_WE1_VGAIN_SEL;         //54配置DDA增益倍数  0 8 10 18 20 28 30 38    1 2 3 4 8 15 22 29
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //55默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //56默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x01;                      //57参比电极以及辅助电极使能
    ucWriteBuffer[ucBufferIndex++] = 0x01;                      //58工作电极的偏置电压由第一个DAC生成使能	
    uint8_t ucCh1WeL8, ucCH1WeH2;
    bms003_get_fix_ch1_din_we(&ucCh1WeL8, &ucCH1WeH2);
    ucWriteBuffer[ucBufferIndex++] = ucCh1WeL8;                 //59设置偏置电压[0:7]
    ucWriteBuffer[ucBufferIndex++] = ucCH1WeH2;                 //5A设置偏置电压[8:9]
    ucWriteBuffer[ucBufferIndex++] = 0x01;// addr:0x5B 参比电极的偏置电压由第二个DAC生成使能[0:7]
    uint8_t ucCh1ReceL8, ucCH1ReceH2;
    bms003_get_fix_ch1_dinRce(&ucCh1ReceL8, ucCH1ReceH2);
    ucWriteBuffer[ucBufferIndex++] = ucCh1ReceL8;// addr:0x5C 配置CE[0:7]
    ucWriteBuffer[ucBufferIndex++] = ucCH1ReceH2;// addr:0x5D 配置CE[8:9]
    bms003_write_burst(0x50, ucWriteBuffer, ucBufferIndex);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x0D;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    bms003_write_burst(0x61, ucWriteBuffer, ucBufferIndex);

    bms003_write_cycle(0x3A, CLK|0x80);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CIC;
    ucWriteBuffer[ucBufferIndex++] = 0x00;
    ucWriteBuffer[ucBufferIndex++] = CHA_NUM;
    bms003_write_burst(0x01, ucWriteBuffer, ucBufferIndex);

    bms003_write_cycle(IMEAS_REG_SEQ, 0x00);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    bms003_write_burst(0x17, ucWriteBuffer, ucBufferIndex);
}
#endif

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




