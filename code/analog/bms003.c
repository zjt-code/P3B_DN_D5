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
/* Private variables ---------------------------------------------------------*/

// 反馈
#define CH1_WE1_RFB_SEL                             0x39
// 增益
#define CH1_WE1_VGAIN_SEL                           0x00
// 偏置
#define CH1_DINWE_L8                                0x96
#define CH1_DINWE_H2                                0x00

// 修改ICLK&PCLK&CIC
#define CLK                                         0x16
#define CIC                                         0x41


#define SLEEP_TIMER_INTERVAL                        (10*1000)
#define SHOT_SLEEP_TIMER_INTERVAL                   (3*1000)

static uint16_t usWe1Vol = 0;                                       // WE1电压
static uint8_t g_ucBms003NewDataFlag = 0;                           // BMS003有新数据标志位
static uint32_t g_Bms003IrqInterrupt;                               // BMS003中断引脚的中断号
static bms003_irq_callback g_Bms003IrqCallbackFun = NULL;           // 中断回调函数
static fifo_t g_NewDataFifo;                                        // 新数据fifo
static double g_fNewDataFifoBuffer[17];                             // 新数据fifo所使用的buffer
static bool g_bWakeupFlag = false;                                  // 唤醒标志位
static uint16_t ucIrqCnt = 0;                                       // 中断计数
static uint16_t ucOnePeriodSampCnt = 0;                             // 单周期采样次数计数
static uint16_t g_BaseWeVol = 0;                                    // WE1校准电压
static uint8_t g_ucSampleingCnt = 0;                                // 采样次数
static afe_run_mode_t g_AfeRunMode = AFE_RUN_MODE_CONTINUOUS;       // AFE运行模式
static bool g_bFirstDataFlag = 0;                                   // 第一个数据标志位
static bms003_start_fsm_t g_Bms003StartFsm = BMS003_START_FSM_IDLE; // BMS003启动过程状态机
sl_sleeptimer_timer_handle_t g_Bms003WakeupTimer;
sl_sleeptimer_timer_handle_t g_Bms003MeasureTimer;
sl_sleeptimer_timer_handle_t g_Bms003StartTimer;
/* Private function prototypes -----------------------------------------------*/
uint8_t bms003_read_cycle(uint8_t ucRegAddr, uint32_t uiStartDelayUs, uint32_t uiStopDelayUs);
void bms003_read_burst(uint8_t ucRegAddr, uint8_t* pData, uint8_t ucLen, uint32_t uiStartDelayUs, uint32_t uiStopDelayUs);
void bms003_write_burst(uint8_t ucRegAddr, uint8_t* pData,uint8_t ucLen, uint32_t uiStartDelayUs, uint32_t uiStopDelayUs);
void bms003_write_cycle(uint8_t ucRegAddr, uint8_t ucData, uint32_t uiStartDelayUs, uint32_t uiStopDelayUs);
void bms003_int_irq_callback(uint8_t intNo, void* ctx);
void bms003_imeas_irq_config_and_reading(void);
void bms003_measure_timer_handler(void);
void bms003_wakeup_timer_handler(void);
void bms003_start(afe_run_mode_t RunMode);
void bms003_stop(void);
void bms003_wakeup(void);
void bms003_sleep(void);
void bms003_int_irq_handler(void);
void bms003_booting_config(void);
void bms003_wakeup_config(void);
void bms003_wakeup_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data);
void bms003_start_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data);
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
    // 使能bms003
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
    // 失能bms003
    GPIO_PinOutClear(AFE_CHIP_EN_PORT, AFE_CHIP_EN_PIN);

}


/*******************************************************************************
*                           陈苏阳@2024-08-28
* Function Name  :  bms003_start_timer_callback
* Description    :  bms003定时器回调
* Input          :  sl_sleeptimer_timer_handle_t * handle
* Input          :  void * data
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_start_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data)
{
    switch (g_Bms003StartFsm)
    {
//     case BMS003_START_FSM_CHIP_EN:
//     {
//         log_i("BMS003_START_FSM_CHIP_EN");
//         // 切换状态
//         g_Bms003StartFsm = BMS003_START_FSM_WAKEUP;
//         // 唤醒bms003
//         bms003_wakeup();
//         // 启动一个10ms后的单次定时器
//         sl_status_t status = sl_sleeptimer_start_timer(&g_Bms003StartTimer, sl_sleeptimer_ms_to_tick(10), bms003_start_timer_callback, (void*)NULL, 0, 0);
//         if (status != SL_STATUS_OK)log_e("sl_sleeptimer_start_timer failed");
//         return;
//     }
    case BMS003_START_FSM_WAKEUP:
    {
        log_i("BMS003_START_FSM_WAKEUP");
        // 设置AFE的INT引脚中断
        GPIO_ExtIntConfig(AFE_INT_PORT, AFE_INT_PIN, g_Bms003IrqInterrupt, true, false, true);

        // 如果采样次数为0,说明是连续采样,则直接开定时器
        if (g_AfeRunMode == AFE_RUN_MODE_CONTINUOUS)
        {
            sl_status_t status = sl_sleeptimer_start_periodic_timer(&g_Bms003WakeupTimer, sl_sleeptimer_ms_to_tick(SLEEP_TIMER_INTERVAL), bms003_wakeup_timer_callback, (void*)NULL, 0, 0);
            if (status != SL_STATUS_OK)
            {
                log_e("sl_sleeptimer_start_periodic_timer failed");
            }
        }

        // bms003配置
        bms003_booting_config();
        // 切换状态
        g_Bms003StartFsm = BMS003_START_FSM_IDLE;
        break;
    }
    default:
    {
        // 切换状态
        g_Bms003StartFsm = BMS003_START_FSM_IDLE;
        break;
    }
    }
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
    GPIO_PinModeSet(AFE_INT_PORT, AFE_INT_PIN, gpioModeInputPull, 0);

    // CS拉高
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    // 配置中断处理函数
    g_Bms003IrqInterrupt = GPIOINT_CallbackRegisterExt(AFE_INT_PIN, bms003_int_irq_callback, NULL);
    log_d("g_Bms003IrqInterrupt:%d", g_Bms003IrqInterrupt);
    // 添加事件
    event_add(MAIN_LOOP_EVENT_AFE_MEASURE_TIMER, bms003_measure_timer_handler);
    event_add(MAIN_LOOP_EVENT_AFE_WAKEUP_TIMER, bms003_wakeup_timer_handler);
    event_add(MAIN_LOOP_EVENT_AFE_IRQ, bms003_int_irq_handler);
    // 创建fifo
    fifo_create(&g_NewDataFifo, g_fNewDataFifoBuffer, sizeof(g_fNewDataFifoBuffer[0]), sizeof(g_fNewDataFifoBuffer) / sizeof(g_fNewDataFifoBuffer[0]));

    // 失能BMS003
    bms003_disable();
}


/*******************************************************************************
*                           陈苏阳@2024-03-27
* Function Name  :  bms003_imeas_irq_config_and_reading
* Description    :  BMS003测量中断的配置与数据读取
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_imeas_irq_config_and_reading(void)
{
    int buff = 0;      //AD中转存储值
    bms003_delay_us(1);
    bms003_write_cycle(IMEAS_REG_SEQ, 0x01, 0, 1);
    bms003_delay_us(1);

    uint8_t ucCh0Data[2];
    bms003_read_burst(IMEAS_CH0DATA_0, ucCh0Data, 2, 0, 0);
    buff = (ucCh0Data[1] << 8) + ucCh0Data[0];
    // 累计中断次数
    ucIrqCnt++;

    log_i("bms003_read_adc_data ucIrqCnt:%d  ucOnePeriodSampCnt:%d", ucIrqCnt, ucOnePeriodSampCnt);

    // 在第14次周期时记录WE1
    if (ucIrqCnt == 19)
    {
        g_BaseWeVol = buff;
    }

    // 开始正常采集
    if (ucIrqCnt >= 20)
    {
        ucIrqCnt = 20;

        // 累计本周期的采样次数
        ucOnePeriodSampCnt++;

        // 如果是本周期的第三次采样,则作为正式数据
        if (ucOnePeriodSampCnt >= 3)
        {
            ucOnePeriodSampCnt = 3;

            // 数据推入FIFO
            double fData = ((double)(buff - g_BaseWeVol) / 32768.0 * 1.2 * 100) / 1.11291;
            log_i("buff:%d   g_BaseWeVol:%d  fData:%f", buff, g_BaseWeVol, fData);
            if (g_bFirstDataFlag==false)
            {
                log_d("send_fifo");
                fifo_in(&g_NewDataFifo, &fData, 1);
            }
            else
            {
                g_bFirstDataFlag = false;
            }
            
        }
    }

    //addr:0x4  清除中断状态
    bms003_delay_us(1);
    bms003_write_cycle(IMEAS_INT, 0x01, 0, 1);
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

/*******************************************************************************
*                           陈苏阳@2024-08-01
* Function Name  :  bms003_update_vol_offset
* Description    :  BMS003更新电压偏移
* Input          :  int16_t sVolOffset
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_update_vol_offset(int16_t sVolOffset)
{
    usWe1Vol = (uint16_t)(CH1_DINWE_H2 << 8) + CH1_DINWE_L8;
    usWe1Vol += sVolOffset;
}

/*******************************************************************************
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
    usWe1Vol = (uint16_t)(CH1_DINWE_H2 << 8) + CH1_DINWE_L8;
    if (g_PrmDb.DacVolOffset != 0)
    {
        usWe1Vol += g_PrmDb.DacVolOffset;
    }

    // 使能BMS003
    bms003_enable();

    // 唤醒bms003
    bms003_wakeup();

    // 状态机切换状态
    g_Bms003StartFsm = BMS003_START_FSM_WAKEUP;

    // 启动一个1S后的单次定时器
    sl_status_t status = sl_sleeptimer_start_timer(&g_Bms003StartTimer, sl_sleeptimer_ms_to_tick(100), bms003_start_timer_callback, (void*)NULL, 0, 0);
    if (status != SL_STATUS_OK)
    {
        log_e("sl_sleeptimer_start_timer failed");
        return;
    }
    g_AfeRunMode = RunMode;

    // 按初次配置来进行
    g_bWakeupFlag = false;

    // 设置第一次数据标志位
    g_bFirstDataFlag = true;
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
    GPIO_PinOutClear(AFE_WAKE_UP_PORT, AFE_WAKE_UP_PIN);
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

    // 先关闭中断
    GPIO_ExtIntConfig(AFE_INT_PORT, AFE_INT_PIN, g_Bms003IrqInterrupt, false, false, false);

    // 清除GPIO中断，进入sleep期间BMS3可能会产生一个IO中断
    GPIO_IntClear(g_Bms003IrqInterrupt);


    // 测量中断后的配置以及读取ADC数据
    bms003_imeas_irq_config_and_reading();

    if ((ucIrqCnt == 20) && (ucOnePeriodSampCnt == 3))
    {
        ucOnePeriodSampCnt = 0;

        bms003_delay_us(1);

        bms003_write_cycle(0x3A, CLK, 0, 1);

        bms003_delay_us(1);
        //使能BG,DAC  关闭BG
        bms003_write_cycle(0x50, 0x02, 0, 1);

        bms003_delay_us(1);

        bms003_write_cycle(0x61, 0x00, 0, 1);

        bms003_delay_us(1);

        bms003_write_cycle(0x3A, CLK | 0x80, 0, 1);

        bms003_delay_us(1);

        bms003_sleep();
    }

    // 开启中断
    GPIO_ExtIntConfig(AFE_INT_PORT, AFE_INT_PIN, g_Bms003IrqInterrupt, true, false, true);

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
* Input          :  uint32_t uiStartDelayUs
* Input          :  uint32_t uiStopDelayUs
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_write_cycle(uint8_t ucRegAddr, uint8_t ucData, uint32_t uiStartDelayUs, uint32_t uiStopDelayUs)
{
    uint8_t ucWriteBuffer[4];
    uint8_t ucBufferIndex = 0;
    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ucRegAddr;
    ucWriteBuffer[ucBufferIndex++] = WR_SINGLE_CMD;
    ucWriteBuffer[ucBufferIndex++] = ucData;
    ucWriteBuffer[ucBufferIndex++] = PAD;

    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    if (uiStartDelayUs)bms003_delay_us(uiStartDelayUs);
    bms003_spi_write_data(ucWriteBuffer, ucBufferIndex);
    if (uiStopDelayUs)bms003_delay_us(uiStopDelayUs);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
}

/*******************************************************************************
*                           陈苏阳@2023-10-30
* Function Name  :  bms003_write_burst
* Description    :  写多个字节
* Input          :  uint8_t ucRegAddr
* Input          :  uint8_t * pData
* Input          :  uint8_t ucLen
* Input          :  uint32_t uiStartDelayUs
* Input          :  uint32_t uiStopDelayUs
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_write_burst(uint8_t ucRegAddr, uint8_t* pData,uint8_t ucLen,uint32_t uiStartDelayUs,uint32_t uiStopDelayUs)
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

    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    if(uiStartDelayUs)bms003_delay_us(uiStartDelayUs);
    bms003_spi_write_data(ucWriteBuffer, ucBufferIndex);
    if(uiStopDelayUs)bms003_delay_us(uiStopDelayUs);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
}

/*******************************************************************************
*                           陈苏阳@2023-10-30
* Function Name  :  bms003_read_cycle
* Description    :  读单个数据
* Input          :  uint8_t ucRegAddr
* Input          :  uint32_t uiStartDelayUs
* Input          :  uint32_t uiStopDelayUs
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t bms003_read_cycle(uint8_t ucRegAddr, uint32_t uiStartDelayUs, uint32_t uiStopDelayUs)
{
    uint8_t ucWriteBuffer[3];
    uint8_t ucReadBuffer[3] = {0, 0, 0};
    uint8_t ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ucRegAddr;
    ucWriteBuffer[ucBufferIndex++] = RD_SINGLE_CMD;
    ucWriteBuffer[ucBufferIndex++] = PAD;

    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    if (uiStartDelayUs)bms003_delay_us(uiStartDelayUs);
    bms003_spi_transfer(ucWriteBuffer, ucReadBuffer, 3);
    if (uiStopDelayUs)bms003_delay_us(uiStopDelayUs);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
    return ucReadBuffer[2];
}

/*******************************************************************************
*                           陈苏阳@2024-03-18
* Function Name  :  bms003_read_burst
* Description    :  读多个字节
* Input          :  uint8_t ucRegAddr
* Input          :  uint8_t * pData
* Input          :  uint8_t ucLen
* Input          :  uint32_t uiStartDelayUs
* Input          :  uint32_t uiStopDelayUs
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_read_burst(uint8_t ucRegAddr, uint8_t* pData, uint8_t ucLen, uint32_t uiStartDelayUs, uint32_t uiStopDelayUs)
{
    uint8_t ucWriteBuffer[32 + 3];
    uint8_t ucReadBuffer[32 + 3];
    uint8_t ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ucRegAddr;
    ucWriteBuffer[ucBufferIndex++] = RD_BURST_REG_CMD;

    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    if (uiStartDelayUs)bms003_delay_us(uiStartDelayUs);
    bms003_spi_transfer(ucWriteBuffer, ucReadBuffer, ucLen + 3);
    if (uiStopDelayUs)bms003_delay_us(uiStopDelayUs);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
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
    uint8_t ucWriteBuffer[32];
    bms003_delay_us(1);
    uint8_t ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CLK;
    ucWriteBuffer[ucBufferIndex++] = 0xb;
    ucWriteBuffer[ucBufferIndex++] = 0x7;
    bms003_write_burst(0x3A, ucWriteBuffer, ucBufferIndex, 1, 1);

    bms003_delay_us(1);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x03;          // addr:0x50 使能BG,DAC[0:1]   // 模拟保持状态关闭BG,DCDC分频
    ucWriteBuffer[ucBufferIndex++] = 0x00;          // addr:0x51 默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x00;          // addr:0x52 默认为0
    ucWriteBuffer[ucBufferIndex++] = CH1_WE1_RFB_SEL;// addr:0x53 配置反馈电阻[2:5]  使能位WE1&DDA [0:1]
    ucWriteBuffer[ucBufferIndex++] = CH1_WE1_VGAIN_SEL;// addr:0x54 配置DDA增益倍数
    ucWriteBuffer[ucBufferIndex++] = 0x00;// addr:0x55 默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x00;// addr:0x56 默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x0;// addr:0x57 参比电极以及辅助电极使能
    ucWriteBuffer[ucBufferIndex++] = 0x1;// addr:0x58 工作电极的偏置电压由第一个DAC生成使能
    ucWriteBuffer[ucBufferIndex++] = usWe1Vol & 0xFF;// addr:0x59 设置偏置电压[0:7]
    ucWriteBuffer[ucBufferIndex++] = (usWe1Vol >> 8) & 0x3;// addr:0x5A 设置偏置电压[8:9]
    ucWriteBuffer[ucBufferIndex++] = 0x00;// addr:0x5B 参比电极的偏置电压由第二个DAC生成使能[0:7]
    ucWriteBuffer[ucBufferIndex++] = 0x00;// addr:0x5C 配置CE[0:7]
    ucWriteBuffer[ucBufferIndex++] = 0x00;// addr:0x5D 配置CE[8:9]
    bms003_write_burst(0x50, ucWriteBuffer, ucBufferIndex, 1, 1);


    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ELE_BUF;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    bms003_write_burst(0x61, ucWriteBuffer, ucBufferIndex, 1, 1);

    bms003_write_cycle(0x3A, CLK|0x80, 1, 1);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CIC;
    ucWriteBuffer[ucBufferIndex++] = 0x00;
    ucWriteBuffer[ucBufferIndex++] = CHA_NUM;
    bms003_write_burst(0x01, ucWriteBuffer, ucBufferIndex, 1, 1);

    bms003_write_cycle(IMEAS_REG_SEQ, 0x00, 1, 2);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    bms003_write_burst(0x017, ucWriteBuffer, ucBufferIndex, 1, 1);
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
    ucWriteBuffer[ucBufferIndex++] = CLK;
    ucWriteBuffer[ucBufferIndex++] = 0x0B;
    ucWriteBuffer[ucBufferIndex++] = 0x07;
    bms003_write_burst(0x3A, ucWriteBuffer, ucBufferIndex, 1, 1);

    bms003_delay_us(1);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x03;                      //50使能BG,DAC[0:1]   //模拟保持状态关闭BG，DCDC分频
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //51默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //52默认为0
    ucWriteBuffer[ucBufferIndex++] = CH1_WE1_RFB_SEL;           //53配置反馈电阻[2:5]  使能位WE1&DDA  [0:1]  
    ucWriteBuffer[ucBufferIndex++] = CH1_WE1_VGAIN_SEL;         //54配置DDA增益倍数  0 8 10 18 20 28 30 38    1 2 3 4 8 15 22 29
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //55默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //56默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //57参比电极以及辅助电极使能
    ucWriteBuffer[ucBufferIndex++] = 0x01;                      //58工作电极的偏置电压由第一个DAC生成使能	
    ucWriteBuffer[ucBufferIndex++] = usWe1Vol&0xFF;             //59设置偏置电压[0:7]
    ucWriteBuffer[ucBufferIndex++] = (usWe1Vol >> 8) & 0x3;     //5A设置偏置电压[8:9]
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //5B参比电极的偏置电压由第二个DAC生成使能[0:7]
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //5C配置CE[0:7]
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //5C配置CE[8:9]
    bms003_write_burst(0x50, ucWriteBuffer, ucBufferIndex, 1, 1);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x0D;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    bms003_write_burst(0x61, ucWriteBuffer, ucBufferIndex, 1, 1);

    bms003_write_cycle(0x3A, CLK|0x80, 1, 1);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CIC;
    ucWriteBuffer[ucBufferIndex++] = 0x00;
    ucWriteBuffer[ucBufferIndex++] = CHA_NUM;
    bms003_write_burst(0x01, ucWriteBuffer, ucBufferIndex, 1, 1);

    bms003_write_cycle(IMEAS_REG_SEQ, 0x00, 1, 1);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    bms003_write_burst(0x017, ucWriteBuffer, ucBufferIndex, 1, 1);
}
#endif

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




