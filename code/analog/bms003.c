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
#include "spidrv.h"
#include "sl_spidrv_instances.h"
#include "bms003.h"
#include "cmsis_gcc.h"
#include "pin_config.h"
#include "app_log.h"
#include "gpiointerrupt.h"
/* Private variables ---------------------------------------------------------*/

// 反馈
uint8_t CH1_WE1_RFB_SEL = 0x39;
// 增益
uint8_t CH1_WE1_VGAIN_SEL = 0x0;
// 偏置
uint8_t CH1_DINWE_L8 =0x94;
uint8_t CH1_DINWE_H2 =0x0;

// 修改ICLK&PCLK&CIC
uint8_t CLK = 0x3e;
uint8_t CIC = 0x61;

uint16_t buff_we1 = 0;

//sl_sleeptimer_timer_handle_t g_Bms003IntIrqTimer;

/* Private function prototypes -----------------------------------------------*/
uint8_t bms003_read_cycle(uint8_t ucRegAddr);
void bms003_write_burst(uint8_t ucRegAddr, uint8_t* pData,uint8_t ucLen);
void bms003_write_cycle(uint8_t ucRegAddr, uint8_t ucData);
void bms003_int_irq_callback(uint8_t intNo, void* ctx);


/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
*                           陈苏阳@2023-10-26
* Function Name  :  bms003_delay_us
* Description    :  延时
* Input          :  uint16_t usUs
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_delay_us(uint16_t usUs)
{
    for (uint16_t i = 0; i < usUs; i++)
    {
        for (uint8_t k = 0; k < 75; k++)
        {
            __NOP();
        }
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
*                           陈苏阳@2023-10-26
* Function Name  :  bms003_init
* Description    :  BMS003初始化
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void bms003_init(void)
{
    // 设置CS引脚为推挽输出
    GPIO_PinModeSet(SPI_CS_PORT, SPI_CS_PIN, gpioModePushPull, 1);

    // 设置AFE_CHIP_EN引脚为推挽输出
    GPIO_PinModeSet(AFE_CHIP_EN_PORT, AFE_CHIP_EN_PIN, gpioModePushPull, 1);

    // 设置AFE_WAKEUP引脚为推挽输出
    GPIO_PinModeSet(AFE_WAKE_UP_PORT, AFE_WAKE_UP_PIN, gpioModePushPull, 1);

    // 使能bms0003
    GPIO_PinOutSet(AFE_CHIP_EN_PORT, AFE_CHIP_EN_PIN);

    // 唤醒bms003
    bms003_wakeup();

    bms003_delay_us(1000);

    // 设置AFE的INT引脚下拉输入
    GPIO_PinModeSet(AFE_INT_PORT, AFE_INT_PIN, gpioModeInputPull, 0);

    // 配置中断处理函数
    unsigned int interrupt = GPIOINT_CallbackRegisterExt(AFE_INT_PIN, bms003_int_irq_callback, NULL);

    // 设置AFE的INT引脚中断
    GPIO_ExtIntConfig(AFE_INT_PORT, AFE_INT_PIN, interrupt, true, false, true);

    // 使能中断
    GPIO_IntEnable(AFE_INT_PIN);

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
    static uint16_t i = 0;
    uint16_t buff2 = 0;
    uint16_t buff3 = 0;
    int32_t buff = 0;
    float ad = 0.0;
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    //////8  addr 0x05

    bms003_write_cycle(IMEAS_REG_SEQ, 0x1);
    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(200);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    buff = bms003_read_cycle(IMEAS_CH0DATA_0);
    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(200);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    buff2 = bms003_read_cycle(IMEAS_CH0DATA_1);
    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    i++;
    buff = (buff2 << 8) + buff;
    if (i < 14)
    {
        app_log_info("*\n");
    }
    if (i == 14)
    {
        buff_we1 = buff;
        bms003_delay_us(200);
        GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);

        bms003_write_cycle(0x3A, CLK);
        bms003_delay_us(30);
        GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

        bms003_delay_us(200);
        GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
        bms003_write_cycle(0x3B, 0x0B);
        bms003_delay_us(31);
        GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);


        bms003_delay_us(200);
        GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
        bms003_write_cycle(0x61, 0x0D);
        bms003_delay_us(31);
        GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

        bms003_delay_us(200);
        GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
        bms003_write_cycle(0x3A, (CLK | 0x80));
        bms003_delay_us(30);
        GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

        bms003_delay_us(300);
        GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
        buff3 = bms003_read_cycle(0x61);
        app_log_info("addr:%x\n", 0x61);
        app_log_info("0x61:%x\n", buff3);
        bms003_delay_us(1);
        GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    }
    if (i >= 15)
    {
        i = 15;
        ad = (buff - buff_we1) / 32768.0 * 1.165 * 100;
        app_log_info("%.5f\n", ad);
    }

    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
    bms003_delay_us(200);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    bms003_write_cycle(IMEAS_INT, 0x01);
    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
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
    bms003_spi_write_data(ucWriteBuffer, ucBufferIndex);
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

    bms003_spi_write_data(ucWriteBuffer, ucBufferIndex);
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
    bms003_spi_transfer(ucWriteBuffer, ucReadBuffer, 3);
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
    //CLK_CTRL_REG
    //address:0x3A
    //cmd:0x80
    //data:0x1
    //16   1E    26   2E   36    3E   
    //32K  16K   8k   4K   2K    1K

    // 配置ICLK[3:5]  PCLK[1:2]  分频 FLCK使能[0]
    // 0x3B 写B & 0x3A[0]置0

    bms003_delay_us(300);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    bms003_delay_us(15);

    uint8_t ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CLK;
    ucWriteBuffer[ucBufferIndex++] = 0xb;
    ucWriteBuffer[ucBufferIndex++] = 0x7;
    bms003_write_burst(0x3A, ucWriteBuffer, ucBufferIndex);
    bms003_delay_us(30);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(300);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    bms003_delay_us(15);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x03;          // addr:0x50 使能BG,DAC[0:1]   // 模拟保持状态关闭BG,DCDC分频
    ucWriteBuffer[ucBufferIndex++] = 0x00;          // addr:0x51 默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x00;          // addr:0x52 默认为0
    ucWriteBuffer[ucBufferIndex++] = CH1_WE1_RFB_SEL;// addr:0x53 配置反馈电阻[2:5]  使能位WE1&DDA [0:1]
    ucWriteBuffer[ucBufferIndex++] = CH1_WE1_VGAIN_SEL;// addr:0x54 配置DDA增益倍数
    ucWriteBuffer[ucBufferIndex++] = CH1_WE1_RFB_SEL;// addr:0x55 默认为0,WE12
    ucWriteBuffer[ucBufferIndex++] = 0x00;// addr:0x56 默认为0
    ucWriteBuffer[ucBufferIndex++] = 0x1;// addr:0x57 参比电极以及辅助电极使能
    ucWriteBuffer[ucBufferIndex++] = 0x1;// addr:0x58 工作电极的偏置电压由第一个DAC生成使能
    ucWriteBuffer[ucBufferIndex++] = CH1_DINWE_L8;// addr:0x59 设置偏置电压[0:7]
    ucWriteBuffer[ucBufferIndex++] = CH1_DINWE_H2;// addr:0x5A 设置偏置电压[8:9]
    ucWriteBuffer[ucBufferIndex++] = 0x1;// addr:0x5B 参比电极的偏置电压由第二个DAC生成使能[0:7]
    ucWriteBuffer[ucBufferIndex++] = 0x7f;// addr:0x5C 配置CE[0:7]
    ucWriteBuffer[ucBufferIndex++] = 0x00;// addr:0x5D 配置CE[8:9]
    bms003_write_burst(0x50, ucWriteBuffer, ucBufferIndex);
    bms003_delay_us(30);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(10 * 1000);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = ELE_BUF;
    ucWriteBuffer[ucBufferIndex++] = 0x1;
    bms003_write_burst(0x61, ucWriteBuffer, ucBufferIndex);
    bms003_delay_us(30);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(300);


    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    bms003_write_cycle(0x3A, (CLK | 0x80));
    bms003_delay_us(30);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);


    app_log_info("CFG_BURST_ANA FINISH!\r\n");

    bms003_delay_us(300);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(15);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = CIC;
    ucWriteBuffer[ucBufferIndex++] = 0x00;
    ucWriteBuffer[ucBufferIndex++] = CHA_NUM;
    bms003_write_burst(0x01, ucWriteBuffer, ucBufferIndex);
    bms003_delay_us(30);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    ////////////////////////
    //SD16RST
    //address:0x05
    //cmd:0x80
    //data:0x0
    ////////////////////////
    bms003_delay_us(300);


    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    bms003_write_cycle(IMEAS_REG_SEQ, 0x00);
    bms003_delay_us(31);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(300);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    bms003_delay_us(17);

    ucBufferIndex = 0;
    ucWriteBuffer[ucBufferIndex++] = 0x1;
    ucWriteBuffer[ucBufferIndex++] = 0x1;
    bms003_write_burst(0x17, ucWriteBuffer, ucBufferIndex);
    bms003_delay_us(30);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    app_log_info("CFG BURST IMEAS FINISH!\r\n");
    // 读寄存器
    bms003_delay_us(300);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);

    uint8_t ucReadData = bms003_read_cycle(0x73);
    app_log_info("addr:%x\n", 0x73);
    app_log_info("check data write:%x\n", ucReadData);

    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(300);


    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    ucReadData = bms003_read_cycle(0x53);
    app_log_info("addr:%x\n", 0x53);
    app_log_info("FB res:%x\n", ucReadData);
    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(300);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    ucReadData = bms003_read_cycle(0x54);
    app_log_info("addr:%x\n", 0x54);
    app_log_info("DDA:%x\n", ucReadData);
    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(300);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    ucReadData = bms003_read_cycle(0x59);
    app_log_info("addr:%x\n", 0x59);
    app_log_info("diff vol low 8bit:%x\n", ucReadData);
    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(300);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);

    ucReadData = bms003_read_cycle(0x5A);
    app_log_info("addr:%x\n", 0x5A);
    app_log_info("diff vol high 2bit:%x\n", ucReadData);
    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(300);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);
    ucReadData = bms003_read_cycle(0x3A);
    app_log_info("addr:%x\n", 0x3A);
    app_log_info("PCLK&ICK:%x\n", ucReadData);
    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(300);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);

    ucReadData = bms003_read_cycle(0x01);
    app_log_info("addr:%x\n", 0x01);
    app_log_info("CIC enable state:%x\n", ucReadData);
    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(300);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);

    ucReadData = bms003_read_cycle(0x50);
    app_log_info("addr:%x\n", 0x50);
    app_log_info("BG,DAC enable state:%x\n", ucReadData);
    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(300);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);

    ucReadData = bms003_read_cycle(0x3B);
    app_log_info("addr:%x\n", 0x3B);
    app_log_info("0X3B:%x\n", ucReadData);
    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);

    bms003_delay_us(300);
    GPIO_PinOutClear(SPI_CS_PORT, SPI_CS_PIN);

    ucReadData = bms003_read_cycle(0x55);
    app_log_info("addr:%x\n", 0x55);
    app_log_info("0X55:%x\n", ucReadData);

    bms003_delay_us(1);
    GPIO_PinOutSet(SPI_CS_PORT, SPI_CS_PIN);
}

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




