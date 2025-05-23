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
#if !defined(LOG_TAG)
    #define LOG_TAG                "LTCGM1272"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_DEBUG

#include "afe.h"
#ifdef AFE_USE_LTCGM1272
#include "ltcgm1272.h"
#include "spidrv.h"
#include "sl_udelay.h"
#include "sl_spidrv_instances.h"
#include "pin_config.h"
#include <elog.h>
#include "gpiointerrupt.h"
#include "app_global.h"
#include "fifo.h"
/* Private variables ---------------------------------------------------------*/


float g_fLtcgm1272CurrData = 0.0f;                                     // LTCGM1272当前数据
uint32_t g_ucLtcgm1272IrqInterrupt;                                    // LTCGM1272中断引脚的中断号
ltcgm1272_irq_callback g_Ltcgm1272IrqCallbackFun = NULL;               // 中断回调函数
fifo_t g_NewDataFifo;                                                  // 新数据fifo
uint16_t g_usNewDataFifoBuffer[9];                                     // 新数据fifo所使用的buffer

sl_sleeptimer_timer_handle_t g_Ltcgm1272WakeupTimer;
sl_sleeptimer_timer_handle_t g_Ltcgm1272MeasureTimer;

#define SLEEP_TIMER_INTERVAL                        (20*1000)
        
/* Private function prototypes -----------------------------------------------*/
void ltcgm1272_int_irq_handler(void);
void ltcgm1272_int_irq_callback(uint8_t intNo, void* ctx);
void ltcgm1272_write_reg(uint8_t ucAddr, uint8_t ucData);
uint8_t ltcgm1272_read_reg(uint8_t ucAddr,uint8_t ucUseCsPin);


/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           陈苏阳@2023-11-15
* Function Name  :  ltcgm1272_register_irq_callback
* Description    :  注册中断回调
* Input          :  ltcgm1272_irq_callback callback
* Output         :  None
* Return         :  void
*******************************************************************************/
void ltcgm1272_register_irq_callback(ltcgm1272_irq_callback callback)
{
    g_Ltcgm1272IrqCallbackFun = callback;
}

/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  ltcgm1272_new_data_is_ready
* Description    :  是否有新数据
* Input          :  void
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool ltcgm1272_new_data_is_ready(void)
{
    return fifo_len(&g_NewDataFifo) ? true : false;
}


/*******************************************************************************
*                           陈苏阳@2023-11-15
* Function Name  :  ltcgm1272_get_new_data
* Description    :  获取新数据
* Input          :  double* pNewData
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool ltcgm1272_get_new_data(double* pNewData)
{
    uint16_t usData;
    if (pNewData == NULL)return false;
    if (fifo_out(&g_NewDataFifo, &usData, 1, 1))
    {
        double fVwe = (0x4E * 1800.0 / 255.0);
        // 按10M电阻计算电流
        *pNewData = (((double)usData)/4095.0*1800.0-fVwe)/ 10000000.0 * 1000.0 * 1000.0;
        return true;
    }
    return false;
}

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
    log_d("ltcgm1272_init");
    // 设置CS引脚为推挽输出
    GPIO_PinModeSet(SPI_CS_PORT, SPI_CS_PIN, gpioModePushPull, 1);

    // 设置AFE的INT引脚下拉输入
    GPIO_PinModeSet(AFE_INT_PORT, AFE_INT_PIN, gpioModeInputPull, 0);

    // AFE触发软件复位
    ltcgm1272_write_reg(CGM1272_SOFT_RESET_CLR,0X01);

    // 配置中断处理函数
    g_ucLtcgm1272IrqInterrupt = GPIOINT_CallbackRegisterExt(AFE_INT_PIN, ltcgm1272_int_irq_callback, NULL);

    //log_d("g_ucLtcgm1272IrqInterrupt:%d", g_ucLtcgm1272IrqInterrupt);

    // 添加事件
    event_add(MAIN_LOOP_EVENT_AFE_IRQ, ltcgm1272_int_irq_handler);

    // 创建fifo
    fifo_create(&g_NewDataFifo, g_usNewDataFifoBuffer, 2, sizeof(g_usNewDataFifoBuffer) / sizeof(g_usNewDataFifoBuffer[0]));
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
    log_d("ltcgm1272_write_reg:0x%02x,0x%02x", ucAddr, ucData);
    uint8_t ucWriteBuffer[2];
    uint8_t ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ucAddr & 0x7F;
    ucWriteBuffer[ucBufferIndex++] = ucData;

    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    sl_udelay_wait(250);
    SPIDRV_MTransmitB(sl_spidrv_usart_AfeSpiInst_handle, ucWriteBuffer, ucBufferIndex);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
    GPIO_PinOutClear(SPI_MOSI_PORT,SPI_MOSI_PIN);
    GPIO_PinModeSet(SPI_MISO_PORT, SPI_MISO_PIN, gpioModeInputPull, 0);
}

/*******************************************************************************
*                           陈苏阳@2023-11-06
* Function Name  :  ltcgm1272_read_reg
* Description    :  读寄存器
* Input          :  uint8_t ucAddr
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t ltcgm1272_read_reg(uint8_t ucAddr,uint8_t ucUseCsPin)
{
    uint8_t ucWriteBuffer[2];
    uint8_t ucReadBuffer[2] = { 0, 0 };
    uint8_t ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ucAddr | 0x80;
    ucWriteBuffer[ucBufferIndex++] = 0xFF;


    if(ucUseCsPin)GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    sl_udelay_wait(250);
    SPIDRV_MTransferB(sl_spidrv_usart_AfeSpiInst_handle, ucWriteBuffer, ucReadBuffer, ucBufferIndex);
    if(ucUseCsPin)GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
    log_d("ltcgm1272_read_reg:0x%02x return:0x%02x", ucAddr, ucReadBuffer[1]);
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
    log_d("ltcgm1272_int_irq_callback");
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
    log_d("ltcgm1272_start");

    ltcgm1272_write_reg(0x13, 0x01);	//open oprtia
    ltcgm1272_write_reg(0x14, 0x07);	//set Rtia
    ltcgm1272_write_reg(0x15, 0x4a | 0x01);//set 2-lead mode-1
    ltcgm1272_write_reg(0x16, 0x0f);	//buffer enable
    ltcgm1272_write_reg(0x17, 0x4E);	//WE=0.55V
    ltcgm1272_write_reg(0x1A, 4);//set waterline = 18
    ltcgm1272_write_reg(0X00, 0X3E);//Adc sample cnt = 31+1
    ltcgm1272_write_reg(0x11, 0x64);	//start adc, Tsample = 100/10 sec


    ltcgm1272_read_reg(0X00,1);//Adc sample cnt = 31+1
    ltcgm1272_read_reg(0x11,1);  //start adc, Tsample = 100/10 sec

    // 设置AFE的INT引脚中断
    GPIO_ExtIntConfig(AFE_INT_PORT, AFE_INT_PIN, g_ucLtcgm1272IrqInterrupt, true, false, true);
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
    log_d("ltcgm1272_stop");
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
    log_i("ltcgm1272_int_irq_handler");

    // 唤醒
    for (uint8_t i = 0; i < 18; i++)
    {
        GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
        sl_udelay_wait(250);

        // 读取数据数量
        uint8_t ucDatanNum = ltcgm1272_read_reg(0x40, 0);
        if (ucDatanNum)
        {
            // 读取FIFO数据
            uint16_t usData = (ltcgm1272_read_reg(0X42, 0) << 8) + (ltcgm1272_read_reg(0X41, 0) & 0xff);

            // 数据推入fifo
            fifo_in(&g_NewDataFifo, &usData, 1);
            GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
        }
        else
        {
            GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
            break;
        }
    }


    // 调用中断回调
    if (g_Ltcgm1272IrqCallbackFun)
    {
        g_Ltcgm1272IrqCallbackFun();
    }
}



#endif
/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




