#include <stdint.h>
#include "em_gpio.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "app.h"
#include "sl_udelay.h"
#include "sl_bt_api.h"
#include "sl_spidrv_instances.h"
#include "pin_config.h"
//Modify these definitions according to your MCU///////////////////////////////////////////////////////

#define BIST_SET_TCK  GPIO_PinOutSet(SPI_CLK_PORT, SPI_CLK_PIN)                     // set tck 1
#define BIST_CLR_TCK GPIO_PinOutClear(SPI_CLK_PORT, SPI_CLK_PIN)                     // set tck 0

#define BIST_SET_TDI GPIO_PinOutSet(SPI_MISO_PORT, SPI_MISO_PIN)                      // set tdi 1
#define BIST_CLR_TDI GPIO_PinOutClear(SPI_MISO_PORT, SPI_MISO_PIN)                      // set tdi 0

#define BIST_TDO_STATE GPIO_PinInGet(AFE_INT_PORT, AFE_INT_PIN) //this logic will check TDO bit value is set or not

#define BIST_SET_CHIP_EN GPIO_PinOutSet(AFE_CHIP_EN_PORT, AFE_CHIP_EN_PIN)            // set chip_en 1
#define BIST_CLR_CHIP_EN GPIO_PinOutClear(AFE_CHIP_EN_PORT, AFE_CHIP_EN_PIN)            // set chip_en 0

#define BIST_SET_WAKE_UP GPIO_PinOutSet(AFE_WAKE_UP_PORT, AFE_WAKE_UP_PIN)             // set wanke up 1
#define BIST_CLR_WAKE_UP GPIO_PinOutClear(AFE_WAKE_UP_PORT, AFE_WAKE_UP_PIN)           // set wanke up 0

#define BIST_SET_TEST_MODE GPIO_PinOutSet(TMODE1_PORT, TMODE1_PIN)      // set test_mode1 1
#define BIST_CLR_TEST_MODE GPIO_PinOutClear(TMODE1_PORT, TMODE1_PIN)     // set test_mode1 0
////////////////////////////////////////////////////////////////////////////////////////////////////////

//#define BIST_READ_MODE(bist) do {bist.BIST_MODE = READ_MODE;} while(0)

#define BIST_READ_MODE(bist)  {(bist)->BIST_MODE = READ_MODE;}
#define BIST_WRITE_MODE(bist) {(bist)->BIST_MODE = WRITE_MODE;}
#define BIST_ERASE_MODE(bist) {(bist)->BIST_MODE = ERASE_MODE;}

#define BIST_START(bist)      {(bist)->BIST_START = 1;}
#define BIST_EN(bist)         {(bist)->ENABLE = 1;}
#define BIST_BUSY_SET(bist)   {(bist)->IS_BUSY = 1;}
#define BIST_NVR_SET(bist)    {(bist)->BIST_NVR = 1;}

#define CMD_LENGTH  67

#define STROBE_BITS         ((uint32_t)0x054D9)  //0b000101010011011001
#define CONFIG_BITS_NVR     ((uint32_t)0x027F)   //0b00000001000111111
#define CONFIG_BITS_NO_NVR  ((uint32_t)0x003F)   //0b00000000000111111

#define CMD_MODE_SEL_PROGRAM_WORD ((uint32_t)0x0C) //0b1100
#define CMD_MODE_SEL_READ_WORD    ((uint32_t)0x0D) //0b1101
#define CMD_MODE_SEL_SECTOR_ERASE ((uint32_t)0x06) //0b0110

#define CMD_FREQ_SEL_DYNAMIC      ((uint32_t)0x00) //0b00
#define CMD_FREQ_SEL_1MHz         ((uint32_t)0x01) //0b01
#define CMD_FREQ_SEL_10MHz        ((uint32_t)0x02) //0b10
#define CMD_FREQ_SEL_20MHz        ((uint32_t)0x03) //0b11

#define CMD_TPROG_CONF_5P4_US     ((uint32_t) 0x00)
#define CMD_TPROG_CONF_6P0_US     ((uint32_t) 0x01)
#define CMD_TPROG_CONF_7P5_US     ((uint32_t) 0x02)
#define CMD_TPROG_CONF_8P25_US    ((uint32_t) 0x03)

#define CMD_TERASE_CONF_0P5_US      ((uint32_t) 0x00)
#define CMD_TERASE_CONF_1P0_US      ((uint32_t) 0x01)
#define CMD_TERASE_CONF_4P0_US      ((uint32_t) 0x02)
#define CMD_TERASE_CONF_5P0_US      ((uint32_t) 0x03)

///////////////////////////////FLASH ADD////////////////////////////////////////////////////////
#define test              0x0004            //


#define WE_DAC_STEP_REG   0x00A0
#define WE_DAC_ZERO_REG   0x00A4
#define RE_DAC_STEP_REG   0x00A8
#define RE_DAC_ZERO_REG   0x00AC

#define DAC1_WE1          0x0018            // WE1偏置电压(DAC1)校准值 10bit
#define DAC2_RE           0x001A            // RE偏置电压(DAC2)校准值  10bit
#define WE1_CURRENT_25C   0x0030            // WE1电流采样 电流源方式  25度
#define TEMP_VALUE_25C    0x0074            // 25度热电偶值
#define TEMP_K            0x0084            // 温度K
#define TEMP_B            0x0088            // 温度B
#define TEMP_WE1          0x008C            // WE1温漂系数

/////////////////////////////////////////////////////////////////////////////////////////////////


typedef enum 
{
  NO_ACT = 0,     //no activity
  READ_MODE = 1,
  WRITE_MODE = 2,
  ERASE_MODE = 3
} BIST_OPERATE_MODE_TypeDef;

typedef enum 
{
  IDLE_ST,
  CMD_SENT_ST,
  WAIT_FOR_READ_ST,
  WAIT_FOR_WRITE_ST,
  WAIT_FOR_ERASE_ST,
  READ_ST,
  WRITE_ST
} BIST_IF_STATE_TypeDef;


typedef enum
{
  NEG_PHASE = 0,
  POS_PHASE = 1
} CLK_PHASE_TypeDef;


typedef struct
{
  CLK_PHASE_TypeDef CLK_PHASE;
  BIST_IF_STATE_TypeDef BIST_ST;
  uint8_t BIST_START;
  uint8_t BIST_NVR;
  uint8_t ENABLE;
  BIST_OPERATE_MODE_TypeDef BIST_MODE;
  uint32_t COMMAND_0;
  uint32_t COMMAND_1;
  uint32_t COMMAND_2;
  uint16_t BIST_ADDRESS;
  uint8_t  READ_DATA_REG;
  uint8_t  WRITE_DATA_REG;
  uint8_t  IS_BUSY;
} BIST_IF_TypeDef;

void bms003_bist_init(void);
void bms003_get_fix_ch1_din_we(uint8_t* pL8Data, uint8_t* pH2Data);
void bms003_get_fix_ch1_dinRce(uint8_t* pL8Data, uint8_t* pH2Data);
extern BIST_IF_TypeDef bist_att;
extern uint16_t dac1WE1, dac2RE;                    // WE1,RE,

void BIST_INIT(BIST_IF_TypeDef* bist);
void BIST_ST(BIST_IF_TypeDef* bist);

void BIST_COMMAND_FORMING(BIST_IF_TypeDef* bist);
void BIST_READ_1BYTE(BIST_IF_TypeDef* bist, uint16_t address, uint8_t nvr_bit);
void BIST_WRITE_1BYTE(BIST_IF_TypeDef* bist, uint16_t address,uint8_t write_data, uint8_t nvr_bit);
void BIST_ERASE_SECTOR(BIST_IF_TypeDef* bist, uint8_t nvr_bit);

void flash_write_float(uint16_t add, float data);

void read_param_value(void);
uint16_t WE1_Read(void);
uint16_t RE_Read(void);
float WE1_current_25c_Read(void);
float TEMP_K_Read(void);
float TEMP_B_Read(void);
float TEMP_WE1_Read(void);
double temp_value(uint16_t temp);
double current_cal(double current, uint16_t temp);
uint16_t test_Read(void);

