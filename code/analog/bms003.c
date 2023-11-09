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
#define LOG_TAG                "BMS0003"
#endif
#undef LOG_LVL
#define LOG_LVL                ELOG_LVL_WARN

#include "spidrv.h"
#include "sl_udelay.h"
#include "sl_spidrv_instances.h"
#include "bms003.h"
#include "cmsis_gcc.h"
#include "pin_config.h"
#include <elog.h>
#include "gpiointerrupt.h"
#include "app_global.h"
/* Private variables ---------------------------------------------------------*/

// 反馈
uint8_t CH1_WE1_RFB_SEL = 0x39;
// 增益
uint8_t CH1_WE1_VGAIN_SEL = 0x0;
// 偏置
uint8_t CH1_DINWE_L8 =0x94;
uint8_t CH1_DINWE_H2 =0x0;

// 修改ICLK&PCLK&CIC
uint8_t CLK = 0x26;
uint8_t CIC = 0x61;

uint16_t buff_we1 = 0;

float g_fBms003CurrData = 0.0f;                     // BMS003当前数据
uint8_t g_ucBms003NewDataFlag = 0;                  // BMS003有新数据标志位
uint32_t g_Bms003IrqInterrupt;                      // BMS003中断引脚的中断号

sl_sleeptimer_timer_handle_t g_Bms003WakeupTimer;
sl_sleeptimer_timer_handle_t g_Bms003MeasureTimer;
#define SLEEP_TIMER_INTERVAL    (20*1000)



/* Private function prototypes -----------------------------------------------*/
uint8_t bms003_read_cycle(uint8_t ucRegAddr, uint32_t uiStartDelayUs, uint32_t uiStopDelayUs);
void bms003_write_burst(uint8_t ucRegAddr, uint8_t* pData,uint8_t ucLen, uint32_t uiStartDelayUs, uint32_t uiStopDelayUs);
void bms003_write_cycle(uint8_t ucRegAddr, uint8_t ucData, uint32_t uiStartDelayUs, uint32_t uiStopDelayUs);
void bms003_int_irq_callback(uint8_t intNo, void* ctx);
void bms003_read_adc_data(void);

/* Private functions ---------------------------------------------------------*/

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
    SPIDRV_MTransferB(sl_spidrv_usart_AfeSpiInst_handle, pTxBuffer, pRxBuffer, ucLen);
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
    SPIDRV_MTransmitB(sl_spidrv_usart_AfeSpiInst_handle, pTxBuffer, ucLen);
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
    // 使能bms0003
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
    log_d("bms003_wakeup_timer_callback\n");
    // 发送事件
    event_push(MAIN_LOOP_EVENT_AFE_WAKEUP_TIMER);
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
    log_d("bms003_measure_timer_callback\n");
    // 发送事件
    event_push(MAIN_LOOP_EVENT_AFE_MEASURE_TIMER);
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
    log_d("bms003_init\n");
    // 设置CS引脚为推挽输出
    GPIO_PinModeSet(SPI_CS_PORT, SPI_CS_PIN, gpioModePushPull, 1);

    // 设置AFE_CHIP_EN引脚为推挽输出
    GPIO_PinModeSet(AFE_CHIP_EN_PORT, AFE_CHIP_EN_PIN, gpioModePushPull, 1);

    // 设置AFE_WAKEUP引脚为推挽输出
    GPIO_PinModeSet(AFE_WAKE_UP_PORT, AFE_WAKE_UP_PIN, gpioModePushPull, 1);

    // 设置AFE的INT引脚下拉输入
    GPIO_PinModeSet(AFE_INT_PORT, AFE_INT_PIN, gpioModeInputPull, 0);

    // 配置中断处理函数
    g_Bms003IrqInterrupt = GPIOINT_CallbackRegisterExt(AFE_INT_PIN, bms003_int_irq_callback, NULL);
    log_d("g_Bms003IrqInterrupt:%d\n", g_Bms003IrqInterrupt);
    // 添加事件
    event_add(MAIN_LOOP_EVENT_AFE_MEASURE_TIMER, bms003_measure_timer_handler);
    event_add(MAIN_LOOP_EVENT_AFE_WAKEUP_TIMER, bms003_wakeup_timer_handler);
    event_add(MAIN_LOOP_EVENT_AFE_IRQ, bms003_int_irq_handler);

    // 清空新数据标志位
    g_ucBms003NewDataFlag = 0;

}

/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  bms003_read_adc_data
* Description    :  bms003读取ADC数据
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_read_adc_data(void)
{

    static uint16_t i = 0;
    static uint16_t i_i = 0;
    static uint16_t a = 0;
    static uint16_t buff_we1 = 0;
    uint16_t buff1 = 0;     //地址值
    uint16_t buff2 = 0;     //AD值
    uint16_t buff3 = 0;     //AD值
    int buff = 0;      //AD中转存储值
    float ad = 0.0;    //AD转换后的电流测量值?



    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(2);
    //////8  addr 0x05
    bms003_write_cycle(IMEAS_REG_SEQ, 0x01, 0, 1);

    //////9   0x7 数据低8位
    bms003_delay_us(2);

    buff2 = bms003_read_cycle(IMEAS_CH0DATA_0, 0, 1);
    buff = buff2;

    //////9   0x8 数据高8位
    bms003_delay_us(2);
    buff2 = bms003_read_cycle(IMEAS_CH0DATA_1, 0, 1);
    buff = (buff2 << 8) + buff;

    i++;

    if (i == 14)
    {
        buff_we1 = buff;
    }
    if (i >= 15)
    {
        i = 15;
        i_i++;
        if (i_i >= 3)
        {
            i_i = 3;
            ad = (buff - buff_we1) / 32768.0 * 1.165 * 100;
            log_i("buff:%d,buff_we1:%d,%.5f\n",buff,buff_we1, ad);

            // 记录新数据,更新标志位
            g_fBms003CurrData = ad;
            g_ucBms003NewDataFlag = 1;
            GPIO_ExtIntConfig(AFE_INT_PORT, AFE_INT_PIN, g_Bms003IrqInterrupt, false, false, false);
        }
    }

    //addr:0x4  清除中断状态
    bms003_delay_us(3);
    bms003_write_cycle(IMEAS_INT, 0x01, 0, 6);

    if ((i == 15) && (i_i == 3))
    {
        i_i = 0;
        bms003_delay_us(3);

        bms003_write_burst(0x3A, &CLK, 1, 15, 30);


        //使能BG,DAC  关闭BG
        bms003_delay_us(2);
        bms003_write_cycle(0x50, 0x03, 0, 30);

        bms003_delay_us(2);

        bms003_write_cycle(0x3A, CLK | 0x80, 0, 30);

        // 采集结束
        bms003_sleep();

        // 清除GPIO中断，计算时间比较长，期间BMS3可能会产生一个IO中断
        GPIO_IntClear(g_Bms003IrqInterrupt);
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
    log_d("bms003_measure_timer_handler\n");
    // 设置AFE的INT引脚中断
    GPIO_ExtIntConfig(AFE_INT_PORT, AFE_INT_PIN, g_Bms003IrqInterrupt, true, false, true);

    // BMS003唤醒配置
    bms003_wakeup_config();
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
   log_d("bms003_wakeup_timer_handler\n");
    // 唤醒BMS003
    bms003_wakeup();

    // 启动一个1S后的单次定时器
    status = sl_sleeptimer_start_timer(&g_Bms003MeasureTimer, sl_sleeptimer_ms_to_tick(1000), bms003_measure_timer_callback, (void*)NULL, 0, 0);
    if (status != SL_STATUS_OK)
    {
        log_e("sl_sleeptimer_start_timer failed\n");
        return;
    }
}

/*******************************************************************************
*                           陈苏阳@2023-11-02
* Function Name  :  bms003_start
* Description    :  BMS003开始工作
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_start(void)
{
    sl_status_t status;

    // 使能BMS003
    bms003_enable();

    // 唤醒bms003
    bms003_wakeup();

    // 等待BMS003启动
    bms003_delay_us(1000 * 1000);

    // 设置AFE的INT引脚中断
    GPIO_ExtIntConfig(AFE_INT_PORT, AFE_INT_PIN, g_Bms003IrqInterrupt, true, false, true);

    status = sl_sleeptimer_start_periodic_timer(&g_Bms003WakeupTimer, sl_sleeptimer_ms_to_tick(SLEEP_TIMER_INTERVAL), bms003_wakeup_timer_callback, (void*)NULL, 0, 0);
    if (status != SL_STATUS_OK)
    {
        log_e("sl_sleeptimer_start_periodic_timer failed\n");
        return;
    }
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
    log_d("bms003_int_irq_handler\n");
    // 读取数据
    bms003_read_adc_data();
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
    event_push(MAIN_LOOP_EVENT_AFE_IRQ);
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
*                           陈苏阳@2023-10-27
* Function Name  :  bms003_config
* Description    :  BMS003配置
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_config(void)
{
    uint8_t ucWriteBuffer[32];
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
    bms003_delay_us(300);
    uint8_t ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CLK;
    ucWriteBuffer[ucBufferIndex++] = 0xb;
    ucWriteBuffer[ucBufferIndex++] = 0x7;
    bms003_write_burst(0x3A, ucWriteBuffer, ucBufferIndex, 15, 30);

    bms003_delay_us(300);

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
    ucWriteBuffer[ucBufferIndex++] = CH1_DINWE_L8;// addr:0x59 设置偏置电压[0:7]
    ucWriteBuffer[ucBufferIndex++] = CH1_DINWE_H2;// addr:0x5A 设置偏置电压[8:9]
    ucWriteBuffer[ucBufferIndex++] = 0x1;// addr:0x5B 参比电极的偏置电压由第二个DAC生成使能[0:7]
    ucWriteBuffer[ucBufferIndex++] = 0x7f;// addr:0x5C 配置CE[0:7]
    ucWriteBuffer[ucBufferIndex++] = 0x00;// addr:0x5D 配置CE[8:9]
    bms003_write_burst(0x50, ucWriteBuffer, ucBufferIndex, 15, 30);

    bms003_delay_us(10 * 1000);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ELE_BUF;
    ucWriteBuffer[ucBufferIndex++] = 0x1;
    bms003_write_burst(0x61, ucWriteBuffer, ucBufferIndex, 0, 30);

    bms003_delay_us(300);

    bms003_write_cycle(0x3A, (CLK | 0x80), 0, 30);


    log_d("CFG_BURST_ANA FINISH!\r\n");

    bms003_delay_us(300);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CIC;
    ucWriteBuffer[ucBufferIndex++] = 0x00;
    ucWriteBuffer[ucBufferIndex++] = CHA_NUM;
    bms003_write_burst(0x01, ucWriteBuffer, ucBufferIndex, 15, 30);

    ////////////////////////
    //SD16RST
    //address:0x05
    //cmd:0x80
    //data:0x0
    ////////////////////////
    bms003_delay_us(300);

    bms003_write_cycle(IMEAS_REG_SEQ, 0x00, 0, 31);

    bms003_delay_us(300);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x1;
    ucWriteBuffer[ucBufferIndex++] = 0x1;
    bms003_write_burst(0x17, ucWriteBuffer, ucBufferIndex, 17, 30);


    log_d("CFG BURST IMEAS FINISH!\r\n");
    // 读寄存器
    bms003_delay_us(300);

    uint8_t ucReadData = bms003_read_cycle(0x73, 0, 1);
    log_d("addr:%x\n", 0x73);
    log_d("check data write:%x\n", ucReadData);

    bms003_delay_us(300);

    ucReadData = bms003_read_cycle(0x53, 0, 1);
    log_d("addr:%x\n", 0x53);
    log_d("FB res:%x\n", ucReadData);

    bms003_delay_us(300);

    ucReadData = bms003_read_cycle(0x54, 0, 1);
    log_d("addr:%x\n", 0x54);
    log_d("DDA:%x\n", ucReadData);

    bms003_delay_us(300);

    ucReadData = bms003_read_cycle(0x59, 0, 1);
    log_d("addr:%x\n", 0x59);
    log_d("diff vol low 8bit:%x\n", ucReadData);

    bms003_delay_us(300);

    ucReadData = bms003_read_cycle(0x5A, 0, 1);
    log_d("addr:%x\n", 0x5A);
    log_d("diff vol high 2bit:%x\n", ucReadData);

    bms003_delay_us(300);

    ucReadData = bms003_read_cycle(0x3A, 0, 1);
    log_d("addr:%x\n", 0x3A);
    log_d("PCLK&ICK:%x\n", ucReadData);

    bms003_delay_us(300);

    ucReadData = bms003_read_cycle(0x01, 0, 1);
    log_d("addr:%x\n", 0x01);
    log_d("CIC enable state:%x\n", ucReadData);

    bms003_delay_us(300);

    ucReadData = bms003_read_cycle(0x50, 0, 1);
    log_d("addr:%x\n", 0x50);
    log_d("BG,DAC enable state:%x\n", ucReadData);

    bms003_delay_us(300);

    ucReadData = bms003_read_cycle(0x3B, 0, 1);
    log_d("addr:%x\n", 0x3B);
    log_d("0X3B:%x\n", ucReadData);

    bms003_delay_us(300);

    ucReadData = bms003_read_cycle(0x55, 0, 1);
    log_d("addr:%x\n", 0x55);
    log_d("0X55:%x\n", ucReadData);
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
    int buf1 = 0;
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
    //CLK_CTRL_REG
    //address:0x3A
    //cmd:0x80
    //data:0x1
    //16   1E    26   2E   36    3E   
    //32K  16K   8k   4K   2K    1K
    ////////////////////////配置ICLK[3:5] PCLK[1:2] 分频 FCLK使能[0]
    //0x3B写B & 0x3A[0]置0
    bms003_delay_us(3);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CLK;
    ucWriteBuffer[ucBufferIndex++] = 0x0B;
    ucWriteBuffer[ucBufferIndex++] = 0x07;
    bms003_write_burst(0x3A, ucWriteBuffer, ucBufferIndex, 15, 30);

    bms003_delay_us(3);

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
    ucWriteBuffer[ucBufferIndex++] = CH1_DINWE_L8;              //59设置偏置电压[0:7]
    ucWriteBuffer[ucBufferIndex++] = CH1_DINWE_H2;              //5A设置偏置电压[8:9]
    ucWriteBuffer[ucBufferIndex++] = 0x01;                      //5B参比电极的偏置电压由第二个DAC生成使能[0:7]
    ucWriteBuffer[ucBufferIndex++] = 0x7F;                      //5C配置CE[0:7]
    ucWriteBuffer[ucBufferIndex++] = 0x00;                      //5C配置CE[8:9]
    bms003_write_burst(0x50, ucWriteBuffer, ucBufferIndex, 15, 30);

    bms003_delay_us(10 * 1000);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x0D;
    ucWriteBuffer[ucBufferIndex++] = 0x01;
    bms003_write_burst(0x61, ucWriteBuffer, ucBufferIndex, 0, 30);

    //第7位置1  3E|80
    bms003_delay_us(3);

    bms003_write_cycle(0x3A, CLK | 0x80, 0, 30);

    bms003_delay_us(3);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CIC;       //1CIC2048 
    ucWriteBuffer[ucBufferIndex++] = 0x00;      //2默认0
    ucWriteBuffer[ucBufferIndex++] = CHA_NUM;   //3通道加模式配置
    bms003_write_burst(0x01, ucWriteBuffer, ucBufferIndex, 15, 30);

    ////////////////////////
    //SD16RST
    //address:0x05
    //cmd:0x80
    //data:0x0
    ////////////////////////
    bms003_delay_us(3);

    bms003_write_cycle(IMEAS_REG_SEQ, 0x0, 0, 31);

    bms003_delay_us(3);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x1;      //17INPUT_FORMAT
    ucWriteBuffer[ucBufferIndex++] = 0x1;      //18使能IMEAS_EN
    bms003_write_burst(0x17, ucWriteBuffer, ucBufferIndex, 17, 30);

    bms003_delay_us(1);
}

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




