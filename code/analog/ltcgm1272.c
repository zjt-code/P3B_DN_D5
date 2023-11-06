/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  ltcgm1272.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  6/11/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "ltcgm1272.h"
#include "spidrv.h"
#include "sl_udelay.h"
#include "sl_spidrv_instances.h"
#include "pin_config.h"
#include "app_log.h"
#include "gpiointerrupt.h"
#include "app_global.h"
/* Private variables ---------------------------------------------------------*/


float g_fLtcgm1272CurrData = 0.0f;                  // LTCGM1272当前数据
uint8_t g_ucLtcgm1272NewDataFlag = 0;               // LTCGM1272有新数据标志位
uint32_t g_ucLtcgm1272IrqInterrupt;                 // LTCGM1272中断引脚的中断号

sl_sleeptimer_timer_handle_t g_Ltcgm1272WakeupTimer;
sl_sleeptimer_timer_handle_t g_Ltcgm1272MeasureTimer;
#define SLEEP_TIMER_INTERVAL                        (20*1000)
        
/* Private function prototypes -----------------------------------------------*/
void ltcgm1272_int_irq_handler(void);
void ltcgm1272_int_irq_callback(uint8_t intNo, void* ctx);

/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  ltcgm1272_init
* Description    :  初始化LTCGM1272
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void ltcgm1272_init(void)
{
    // 设置CS引脚为推挽输出
    GPIO_PinModeSet(SPI_CS_PORT, SPI_CS_PIN, gpioModePushPull, 1);

    // 设置AFE_CHIP_EN引脚为推挽输出
    GPIO_PinModeSet(AFE_CHIP_EN_PORT, AFE_CHIP_EN_PIN, gpioModePushPull, 1);

    // 设置AFE_WAKEUP引脚为推挽输出
    GPIO_PinModeSet(AFE_WAKE_UP_PORT, AFE_WAKE_UP_PIN, gpioModePushPull, 1);

    // 设置AFE的INT引脚下拉输入
    GPIO_PinModeSet(AFE_INT_PORT, AFE_INT_PIN, gpioModeInputPull, 0);

    // 配置中断处理函数
    g_ucLtcgm1272IrqInterrupt = GPIOINT_CallbackRegisterExt(AFE_INT_PIN, ltcgm1272_int_irq_callback, NULL);

    // 添加事件
    event_add(MAIN_LOOP_EVENT_AFE_IRQ, ltcgm1272_int_irq_handler);

    // 清空新数据标志位
    g_ucLtcgm1272NewDataFlag = 0;
}

/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  ltcgm1272_write_reg
* Description    :  写寄存器
* Input          :  uint8_t ucAddr
* Input          :  uint8_t ucData
* Output         :  None
* Return         :  void
*******************************************************************************/
void ltcgm1272_write_reg(uint8_t ucAddr, uint8_t ucData)
{
    uint8_t ucWriteBuffer[2];
    uint8_t ucBufferIndex = 0;
    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ucAddr & 0x7F;
    ucWriteBuffer[ucBufferIndex++] = ucData;

    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    SPIDRV_MTransmitB(sl_spidrv_usart_AfeSpiInst_handle, ucWriteBuffer, ucBufferIndex);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
}

/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  ltcgm1272_read_reg
* Description    :  读寄存器
* Input          :  uint8_t ucAddr
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t ltcgm1272_read_reg(uint8_t ucAddr)
{
    uint8_t ucWriteBuffer[2];
    uint8_t ucReadBuffer[2] = { 0, 0, 0 };
    uint8_t ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ucAddr | 0x80;
    ucWriteBuffer[ucBufferIndex++] = 0xFF;

    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    SPIDRV_MTransferB(sl_spidrv_usart_AfeSpiInst_handle, ucWriteBuffer, ucReadBuffer, ucBufferIndex);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
    return ucReadBuffer[1];
}



/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  ltcgm1272_int_irq_callback
* Description    :  LTCGM1272中断引脚标志位
* Input          :  uint8_t intNo
* Input          :  void * ctx
* Output         :  None
* Return         :  void
*******************************************************************************/
void ltcgm1272_int_irq_callback(uint8_t intNo, void* ctx)
{
    // 发送事件
    event_push(MAIN_LOOP_EVENT_AFE_IRQ);
}


/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  ltcgm1272_start
* Description    :  LTCGM1272启动
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void ltcgm1272_start(void)
{
    ltcgm1272_write_reg(0X00, 0x3E);    //采样次数31+1=32次
    ltcgm1272_write_reg(0x04, 0x03);    //ADC时钟4分频
    ltcgm1272_write_reg(0x05, 0x0F);    //采样时间间隔
    ltcgm1272_write_reg(0X1A, 0x0A);    //FIFO水线=18
    ltcgm1272_write_reg(0X12, 0x01);    //open oprce
    ltcgm1272_write_reg(0X13, 0x01);    //open oprtia
    ltcgm1272_write_reg(0X14, 0x35);    //set Rtia
    ltcgm1272_write_reg(0X15, 0x4a);    //set 2-lead mode-1   
    ltcgm1272_write_reg(0X16, 0x0f);    //buffer bypass
    ltcgm1272_write_reg(0X17, 0x78);    //WE=0.55V
    ltcgm1272_write_reg(0X11, 0x64);    //10s采一个点
}

/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  ltcgm1272_stop
* Description    :  停止采样
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void ltcgm1272_stop(void)
{
    // 设置ADC采样频率为0
    ltcgm1272_write_reg(0X11, 0x00);
}


/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  ltcgm1272_int_irq_handler
* Description    :  LTCGM1272中断处理
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void ltcgm1272_int_irq_handler(void)
{
    uint16_t usDataArray[20];
    for (uint8_t i = 0; i < 20; i++)
    {
        // 读取数据数量
        uint8_t ucDatanNum = ltcgm1272_read_reg(0x40);
        if (ucDatanNum)
        {
            // 读取FIFO数据
            usDataArray[i] = (ltcgm1272_read_reg(0X42) << 8) + (ltcgm1272_read_reg(0X41) & 0xff);
        }
        else
        {
            break;
        }
    }
    for(uint8_t i=0;i<18;i++)app_log_info("No.%d Data:%d\n", usDataArray[i]);

    app_log_info("\r\n\r\n");
}




/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




