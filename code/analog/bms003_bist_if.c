#if !defined(LOG_TAG)
    #define LOG_TAG                    "bms003_bist_if"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_INFO
#include <elog.h>

#include "em_gpio.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "app.h"
#include "sl_udelay.h"
#include "sl_bt_api.h"
#include "sl_iostream_usart_vcom_config.h"
#include "sl_iostream_init_instances.h"
#include "sl_spidrv_instances.h"
#include "bms003_bist_if.h"
#include "em_timer.h"
#include "pin_config.h"
uint8_t g_ucFixCh1DinWeL8;
uint8_t g_ucFixCh1DinWeH2;

uint8_t g_ucFixCh1DinRceL8;
uint8_t g_ucFixCh1DinRceH2;

void TIMER0_IRQHandler(void)
{
    uint32_t flags = TIMER_IntGet(TIMER0);
    TIMER_IntClear(TIMER0, flags);
    BIST_ST(&bist_att);
}

void bms003_get_fix_ch1_din_we(uint8_t* pL8Data, uint8_t* pH2Data)
{
    if (pL8Data)*pL8Data = g_ucFixCh1DinWeL8;
    if (pH2Data)*pH2Data = g_ucFixCh1DinWeH2;
}

void bms003_get_fix_ch1_dinRce(uint8_t* pL8Data, uint8_t* pH2Data)
{
    if (pL8Data)*pL8Data = g_ucFixCh1DinRceL8;
    if (pH2Data)*pH2Data = g_ucFixCh1DinRceH2;
}

void bms003_bist_init(void)
{
    GPIO_PinModeSet(AFE_INT_PORT, AFE_INT_PIN, gpioModeInput, 1);

    GPIO_PinModeSet(AFE_WAKE_UP_PORT, AFE_WAKE_UP_PIN, gpioModePushPull, 1);

    GPIO_PinModeSet(AFE_CHIP_EN_PORT, AFE_CHIP_EN_PIN, gpioModePushPull, 1);

    GPIO_PinModeSet(TMODE1_PORT, TMODE1_PIN, gpioModePushPull, 1);

    GPIO_PinModeSet(SPI_CLK_PORT, SPI_CLK_PIN, gpioModePushPull, 1);

    GPIO_PinModeSet(SPI_MISO_PORT, SPI_MISO_PIN, gpioModePushPull, 1);
    // Enable chip_en, wake_up

    BIST_CLR_CHIP_EN;
    BIST_CLR_WAKE_UP;
  
    sl_sleeptimer_delay_millisecond(50);
    //  sl_sleeptimer_delay_millisecond(500);
    BIST_SET_CHIP_EN;
    BIST_SET_WAKE_UP;
    sl_sleeptimer_delay_millisecond(50);
    //  sl_sleeptimer_delay_millisecond(500);

    // Set TEST_MODE = 1; Enable BIST test mode
    BIST_SET_TEST_MODE;

    CMU_ClockEnable(cmuClock_TIMER0, true);
    TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
    TIMER_Init(TIMER0, &timerInit);
    TIMER_TopSet(TIMER0, 4000);
    TIMER_IntEnable(TIMER0, TIMER_IEN_OF);
    NVIC_EnableIRQ(TIMER0_IRQn);
    TIMER_Enable(TIMER0, true);

    // 初始化状态机全局变量
    BIST_INIT(&bist_att);

    // 读取FLAH中的校准参数
    // WE1偏置电压(DAC1)校准值,RE偏置电压(DAC2)校准值,
    // 温度参数
    read_param_value();
    // DISABLE TIM0
    TIMER_Enable(TIMER0, false);
    NVIC_DisableIRQ(TIMER0_IRQn);
    TIMER_Reset(TIMER0);
    // DISABLE  clock
    CMU_ClockEnable(cmuClock_TIMER0, 0);

    //    delay_ms_type(50);
    //   // 禁用BIST测试模式
    // Set TEST_MODE = 0; Disable BIST test mode
     //置低TESTMODE1脚
    GPIO_PinModeSet(TMODE1_PORT, TMODE1_PIN, gpioModeInputPull, 0);
    //调用DAC校准数值库，用于后续偏置电压赋值
    //将FLASH内的偏压值分成寄存器格式写入
    g_ucFixCh1DinWeL8 = (uint8_t)dac1WE1;
    g_ucFixCh1DinWeH2 = (uint8_t)(dac1WE1 >> 8);

    g_ucFixCh1DinRceL8 = (uint8_t)dac2RE;
    g_ucFixCh1DinRceH2 = (uint8_t)(dac2RE >> 8);

    log_i("CH1_DINWE_L8:%x", g_ucFixCh1DinWeL8);
    log_i("CH1_DINWE_H2:%x", g_ucFixCh1DinWeH2);
    log_i("CH1_DINRCE_L8:%x", g_ucFixCh1DinRceL8);
    log_i("CH1_DINRCE_H2:%x", g_ucFixCh1DinRceH2);
}



BIST_IF_TypeDef bist_att;

uint16_t dac1WE1, dac2RE;            // WE1偏置电压(DAC1)校准值, RE偏置电压(DAC2)校准值
static float tempK, tempB;                  // 温度K,温度B,
static double tempWE1, currentWE1;           // WE1温漂系数, WE1电流值 电流源方式  25度
static double currentValue, tempValue;       // 校正后的电流值，芯片摄氏温度值
static float tempValueIR25C;                // 热电偶25度温度值

static uint16_t stm_cmd_count = 0;        // state machine command counter
static uint16_t wait_for_rd_count = 0;
static uint16_t wait_for_wr_count = 0;
static uint16_t wait_for_erase_count = 0;
static uint16_t read_data_count = 0;
static uint8_t shift_data = 0;

void BIST_ST(BIST_IF_TypeDef* bist)
{
    uint8_t pinState;

    if (bist->CLK_PHASE == NEG_PHASE)
    {
        BIST_CLR_TCK;
    }
    else
    {
        BIST_SET_TCK;
    }

    if (bist->ENABLE == 1 && bist->CLK_PHASE == NEG_PHASE)
    {
        switch (bist->BIST_ST)
        {
        case IDLE_ST:
            if (bist->BIST_START == 1)
            {
                bist->BIST_ST = CMD_SENT_ST;
                bist->IS_BUSY = 1;
                bist->READ_DATA_REG = 0;
                //          printf("IDLE_ST IS_BUSY %d\n",bist->IS_BUSY);
            }
            else
            {
                stm_cmd_count = 0;
                wait_for_rd_count = 0;
                wait_for_wr_count = 0;
                read_data_count = 0;
                wait_for_erase_count = 0;
                bist->IS_BUSY = 0;
                //          printf("IDLE_ST IS_BUSY %d\n",bist->IS_BUSY);
            }
            break;

        case CMD_SENT_ST:
            if (stm_cmd_count < CMD_LENGTH)
            {
                //shift out the TDI bit based on shift data. To get the program optimised, shift data
                //has been calculated during POS_PHASE so it is ready for shifting here

                if (shift_data)
                {
                    BIST_SET_TDI;
                }
                else
                {
                    BIST_CLR_TDI;
                }
                stm_cmd_count++;
            }
            else
            {
                stm_cmd_count = 0;
                if (bist->BIST_MODE == WRITE_MODE)
                {
                    bist->BIST_ST = WAIT_FOR_WRITE_ST;
                }
                else if (bist->BIST_MODE == READ_MODE)
                {
                    bist->BIST_ST = WAIT_FOR_READ_ST;
                }
                else if (bist->BIST_MODE == ERASE_MODE)
                {
                    bist->BIST_ST = WAIT_FOR_ERASE_ST;
                }
            }
            break;

        case WAIT_FOR_READ_ST:
            if (++wait_for_rd_count == 6)
            {
                wait_for_rd_count = 0;
                bist->BIST_ST = READ_ST;
            }
            break;

        case READ_ST:
            if (read_data_count < 8)
            {
                //read data from TDO
                pinState = (BIST_TDO_STATE) ? 1 : 0;
                bist->READ_DATA_REG = bist->READ_DATA_REG | (pinState << read_data_count);
                read_data_count++;
            }
            else
            {
                bist->ENABLE = 0;
                bist->BIST_ST = IDLE_ST;
                bist->BIST_START = 0; //Reset bist start signal after the read is done
                read_data_count = 0;
                bist->IS_BUSY = 0;
                //          printf("READ_ST IS_BUSY %d\n",bist->IS_BUSY);
            }
            break;
        case WAIT_FOR_WRITE_ST:
            if (++wait_for_wr_count == 2)
            {
                bist->ENABLE = 0;
                bist->BIST_ST = IDLE_ST;
                bist->BIST_START = 0;       //Reset bist start signal after the read is done
                wait_for_wr_count = 0;
                bist->IS_BUSY = 0;
                //          printf("WAIT_FOR_WRITE_ST IS_BUSY %d\n",bist->IS_BUSY);
            }
            break;
        case WAIT_FOR_ERASE_ST:
            if (++wait_for_erase_count == 1000)
            {
                bist->ENABLE = 0;
                bist->BIST_ST = IDLE_ST;
                bist->BIST_START = 0;       //Reset bist start signal after the read is done
                wait_for_erase_count = 0;
                bist->IS_BUSY = 0;
                //          printf("WAIT_FOR_ERASE_ST IS_BUSY %d\n",bist->IS_BUSY);

            }
        default:
            break;
        }
    }
    else if (bist->ENABLE == 1 && bist->CLK_PHASE == POS_PHASE)
    {
        if (bist->BIST_ST == CMD_SENT_ST)
        {
            if (stm_cmd_count < 32)
            {
                shift_data = bist->COMMAND_0 & 0x01;
                bist->COMMAND_0 = (bist->COMMAND_0 >> 1);
            }
            else if (stm_cmd_count < 64)
            {
                shift_data = bist->COMMAND_1 & 0x01;
                bist->COMMAND_1 = (bist->COMMAND_1 >> 1);
            }
            else
            {
                shift_data = bist->COMMAND_2 & 0x01;
                bist->COMMAND_2 = (bist->COMMAND_2 >> 1);
            }
        }
    }

    if (bist->CLK_PHASE == NEG_PHASE)
    {
        bist->CLK_PHASE = POS_PHASE;
    }
    else
    {
        bist->CLK_PHASE = NEG_PHASE;
    }

}


void BIST_INIT(BIST_IF_TypeDef* bist)
{
    bist->CLK_PHASE = NEG_PHASE;
    bist->BIST_ST = IDLE_ST;
    bist->BIST_START = 0;
    bist->ENABLE = 0;
    bist->BIST_MODE = NO_ACT;
    bist->COMMAND_0 = 0;
    bist->COMMAND_1 = 0;
    bist->READ_DATA_REG = 0;
    bist->WRITE_DATA_REG = 0;
    bist->IS_BUSY = 0;
}

void BIST_COMMAND_FORMING(BIST_IF_TypeDef* bist)
{
    uint32_t config_bits = 0;

    if (bist->BIST_NVR == 1)
    {
        config_bits = CONFIG_BITS_NVR; // set NVR bit
    }
    else
    {
        config_bits = CONFIG_BITS_NO_NVR;
    }
    if (bist->BIST_MODE == ERASE_MODE)
    {
        config_bits |= 0x01 << 6;
    }

    if (bist->BIST_MODE == READ_MODE)
    {
        bist->COMMAND_0 = (STROBE_BITS | config_bits << 17); //17 bit strobe + 15 first CONFIG bits
        // 2 last bits of CONFIG + 15 bit ADDRESS + 8 bit data + 4 bits mode + 2 bits freq + 1 bit TPROG
        bist->COMMAND_1 = (uint32_t)(config_bits >> 15 & 0x03) | ((uint32_t)bist->BIST_ADDRESS) << 2 | ((uint32_t)CMD_MODE_SEL_READ_WORD) << 25 | ((uint32_t)CMD_FREQ_SEL_1MHz) << 29 | (CMD_TPROG_CONF_6P0_US & 0x01) << 31;
        //1 last TPROG bit + 2 erase bits
        bist->COMMAND_2 = (CMD_TPROG_CONF_6P0_US >> 1 & 0x01) | CMD_TERASE_CONF_4P0_US << 1; //erase time 4ms
    }
    else if (bist->BIST_MODE == WRITE_MODE)
    {
        bist->COMMAND_0 = (STROBE_BITS | config_bits << 17); //17 bit strobe + 15 first CONFIG bits
        // 2 last bits of CONFIG + 15 bit ADDRESS + 8 bit data + 4 bits mode + 2 bits freq + 1 bit TPROG
        bist->COMMAND_1 = (uint32_t)(config_bits >> 15 & 0x03) | ((uint32_t)bist->BIST_ADDRESS) << 2 | (uint32_t)bist->WRITE_DATA_REG << 17 | ((uint32_t)CMD_MODE_SEL_PROGRAM_WORD) << 25 | ((uint32_t)CMD_FREQ_SEL_1MHz) << 29 | (CMD_TPROG_CONF_6P0_US & 0x01) << 31;
        //1 last TPROG bit + 2 erase bits
        bist->COMMAND_2 = (CMD_TPROG_CONF_6P0_US >> 1 & 0x01) | CMD_TERASE_CONF_4P0_US << 1; //erase time 4ms
    }
    else if (bist->BIST_MODE == ERASE_MODE)
    {
        bist->COMMAND_0 = (STROBE_BITS | config_bits << 17); //17 bit strobe + 15 first CONFIG bits
        // 2 last bits of CONFIG + 15 bit ADDRESS + 8 bit data + 4 bits mode + 2 bits freq + 1 bit TPROG
        bist->COMMAND_1 = (uint32_t)(config_bits >> 15 & 0x03) | ((uint32_t)bist->BIST_ADDRESS) << 2 | (uint32_t)bist->WRITE_DATA_REG << 17 | ((uint32_t)CMD_MODE_SEL_SECTOR_ERASE) << 25 | ((uint32_t)CMD_FREQ_SEL_1MHz) << 29 | (CMD_TPROG_CONF_6P0_US & 0x01) << 31;
        //1 last TPROG bit + 2 erase bits
        bist->COMMAND_2 = (CMD_TPROG_CONF_6P0_US >> 1 & 0x01) | CMD_TERASE_CONF_4P0_US << 1; //erase time 4ms
    }

}


void BIST_READ_1BYTE(BIST_IF_TypeDef* bist, uint16_t address, uint8_t nvr_bit)
{
    //BIST READ
    BIST_READ_MODE(bist);
    bist->BIST_NVR = nvr_bit;
    bist->BIST_ADDRESS = address;
    BIST_COMMAND_FORMING(bist);
    BIST_EN(bist);
    BIST_START(bist);
    BIST_BUSY_SET(bist); // must set IS_BUSY = 1 right after START signal is asserted
    //  printf("BIST_READ_1BYTE %d\n",bist->IS_BUSY);
    while (bist->IS_BUSY)WDOGn_Feed(WDOG0);
    //  printf("BIST_READ_1BYTE_1 %d\n",bist->IS_BUSY);

}

void BIST_WRITE_1BYTE(BIST_IF_TypeDef* bist, uint16_t address, uint8_t write_data, uint8_t nvr_bit)
{
    //BIST READ
    BIST_WRITE_MODE(bist);
    bist->BIST_NVR = nvr_bit;
    bist->BIST_ADDRESS = address;
    bist->WRITE_DATA_REG = write_data;
    BIST_COMMAND_FORMING(bist);
    BIST_EN(bist);
    BIST_START(bist);
    BIST_BUSY_SET(bist); // must set IS_BUSY = 1 right after START signal is asserted
    while (bist->IS_BUSY)WDOGn_Feed(WDOG0);
}

void BIST_ERASE_SECTOR(BIST_IF_TypeDef* bist, uint8_t nvr_bit) {
    BIST_ERASE_MODE(bist);
    bist->BIST_NVR = nvr_bit;
    BIST_COMMAND_FORMING(bist);
    BIST_EN(bist);
    BIST_START(bist);
    BIST_BUSY_SET(bist); // must set IS_BUSY = 1 right after START signal is asserted
    while (bist->IS_BUSY)WDOGn_Feed(WDOG0);
}

/*
* 从0x0018地址，读取WE1偏置电压(DAC1)校准值
* 返回 WE1偏置电压(DAC1)校准值 10bit
*/
uint16_t WE1_Read(void)
{
    uint16_t dac1_ce1_data;

    //    BIST_ERASE_SECTOR(&bist_att,1);
    //    delay_ms_type(5);
    //    BIST_WRITE_1BYTE(&bist_att,DAC1_WE1,0xf0,1);
    //    delay_ms_type(5);


    BIST_READ_1BYTE(&bist_att, DAC1_WE1 + 1, 1);                       // DAC1_WE1 MSB
    dac1_ce1_data = bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, DAC1_WE1, 1);
    dac1_ce1_data = (dac1_ce1_data << 8) + bist_att.READ_DATA_REG;    // DAC1_WE1 LSB

    return (dac1_ce1_data & 0x3ff);                                   // DAC1_WE1 10bit
}

uint16_t test_Read(void)
{
    uint16_t dac1_ce1_data;

    //    BIST_ERASE_SECTOR(&bist_att,1);
    //    delay_ms_type(5);
    BIST_WRITE_1BYTE(&bist_att, test, 0xf0, 1);
    sl_sleeptimer_delay_millisecond(5);


    BIST_READ_1BYTE(&bist_att, test, 1);                       // DAC1_WE1 MSB
    dac1_ce1_data = bist_att.READ_DATA_REG;
    log_i("test:%d", dac1_ce1_data);

}
/*
* 从0x001A地址，读取RE偏置电压(DAC2)校准值
* 返回 RE偏置电压(DAC2)校准值 10bit
*/
uint16_t RE_Read(void)
{
    uint16_t dac2_re_data;

    BIST_READ_1BYTE(&bist_att, DAC2_RE + 1, 1);                      // DAC2_RE MSB
    dac2_re_data = bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, DAC2_RE, 1);
    dac2_re_data = (dac2_re_data << 8) + bist_att.READ_DATA_REG;    // DAC2_RE LSB

    return (dac2_re_data & 0x3ff);                                  // DAC2_RE 10bit
}

/*
* 从0x0030地址，读WE1 电流值
* 返回float类型 WE1值
*/
float WE1_current_25c_Read(void)                                  // WE1电流值 电流源方式 25度
{
    uint32_t we1_current;

    BIST_READ_1BYTE(&bist_att, WE1_CURRENT_25C + 3, 1);
    we1_current = bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, WE1_CURRENT_25C + 2, 1);
    we1_current = (we1_current << 8) + bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, WE1_CURRENT_25C + 1, 1);
    we1_current = (we1_current << 8) + bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, WE1_CURRENT_25C, 1);
    we1_current = (we1_current << 8) + bist_att.READ_DATA_REG;

    return *((float*)(&we1_current));
}

/*
* 从0x0074地址，读热电偶温度值
* 返回float类型 热电偶温度值
*/
float temp_value_25c_Read(void)
{
    uint32_t temp_value;

    BIST_READ_1BYTE(&bist_att, TEMP_VALUE_25C + 3, 1);
    temp_value = bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, TEMP_VALUE_25C + 2, 1);
    temp_value = (temp_value << 8) + bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, TEMP_VALUE_25C + 1, 1);
    temp_value = (temp_value << 8) + bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, TEMP_VALUE_25C, 1);
    temp_value = (temp_value << 8) + bist_att.READ_DATA_REG;

    return *((float*)(&temp_value));
}

/*
* 从0x0084地址，读温度K值
* 返回float类型 温度K值
*/
float TEMP_K_Read(void)
{
    uint32_t temp_k_data;

    BIST_READ_1BYTE(&bist_att, TEMP_K + 3, 1);
    temp_k_data = bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, TEMP_K + 2, 1);
    temp_k_data = (temp_k_data << 8) + bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, TEMP_K + 1, 1);
    temp_k_data = (temp_k_data << 8) + bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, TEMP_K, 1);
    temp_k_data = (temp_k_data << 8) + bist_att.READ_DATA_REG;

    return *((float*)(&temp_k_data));
}

/*
* 从0x0088地址，读温度B值
* 返回float类型 温度B值
*/
float TEMP_B_Read(void)
{
    uint32_t temp_b_data;

    BIST_READ_1BYTE(&bist_att, TEMP_B + 3, 1);
    temp_b_data = bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, TEMP_B + 2, 1);
    temp_b_data = (temp_b_data << 8) + bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, TEMP_B + 1, 1);
    temp_b_data = (temp_b_data << 8) + bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, TEMP_B, 1);
    temp_b_data = (temp_b_data << 8) + bist_att.READ_DATA_REG;

    return *((float*)(&temp_b_data));
}

/*
* 从0x008C地址，读WE1温漂系数
* 返回float类型 WE1温漂系数
*/
float TEMP_WE1_Read(void)
{
    uint32_t temp_we1_data;

    BIST_READ_1BYTE(&bist_att, TEMP_WE1 + 3, 1);
    temp_we1_data = bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, TEMP_WE1 + 2, 1);
    temp_we1_data = (temp_we1_data << 8) + bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, TEMP_WE1 + 1, 1);
    temp_we1_data = (temp_we1_data << 8) + bist_att.READ_DATA_REG;

    BIST_READ_1BYTE(&bist_att, TEMP_WE1, 1);
    temp_we1_data = (temp_we1_data << 8) + bist_att.READ_DATA_REG;

    return *((float*)(&temp_we1_data));
}

/*
* 写一个float数据到FLASH
* add:FLASH 地址
* data：浮点数
*/
void flash_write_float(uint16_t add, float data)      // 写一个float数据
{
    uint8_t  i;
    uint8_t  var1 = 0;
    uint32_t var2 = 0;
    float var3 = data;

    var2 = *((uint32_t*)(&var3));
    for (i = 0; i < 4; i++)
    {
        var1 = (uint8_t)(var2 >> i * 8);
        BIST_WRITE_1BYTE(&bist_att, add + i, var1, 1);
        sl_sleeptimer_delay_millisecond(5);
    }
}

/*
* 读取FLAH中的校准参数
*/
void read_param_value(void)
{
    test_Read();
    dac1WE1 = WE1_Read();                   // WE1偏置电压(DAC1)校准值 10bit
    dac1WE1 = dac1WE1 - (400 / 1.66f);
    dac2RE = RE_Read();                     // RE偏置电压(DAC2)校准值 10bit
    //dac2RE = dac2RE + (500 / 1.66f);
    tempK = TEMP_K_Read();                  // 温度K
    tempB = TEMP_B_Read();                  // 温度B
    tempWE1 = TEMP_WE1_Read();              // WE1温漂系数
    currentWE1 = WE1_current_25c_Read();    // WE1电流值 电流源方式  25度
    tempValueIR25C = temp_value_25c_Read(); // 热电偶25度温度值
    log_i("dac1WE1:%d", dac1WE1);
    log_i("dac2RE:%d", dac2RE);
    log_i("tempK:%f", tempK);
    log_i("tempB:%f", tempB);
    log_i("tempWE1:%f", tempWE1);
    log_i("currentWE1:%f", currentWE1);
    log_i("tempValueIR25C:%f", tempValueIR25C);
}

/*
* 温度转换函数
* temp：温度AD值
* 返回float类型 摄氏温度值
*/
double temp_value(uint16_t temp)
{
    double tmp = ((double)(temp)-tempB) / tempK;
    tempValue = tmp;
    //log_i("temp_value %d,%f,%f,%f", temp, tempB, tempK, tmp);
    return tmp;
}

/*
* 电流温度补偿函数
* current:采集到的WE1电流值，
* temp：温度AD值
* 返回float类型 补偿后的电流值
*/
double current_cal(double current, uint16_t temp)
{
    currentValue = current / (currentWE1 - (tempWE1 * (tempValueIR25C - tempValue)));

    return currentValue;
}





