/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  bms003.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  26/10/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BMS003_H
#define __BMS003_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "stdbool.h"
/* Private define ------------------------------------------------------------*/


#define IMEAS_REG_CTRL_0                            0x01
#define IMEAS_REG_CTRL_1                            0x02
#define IMEAS_REG_CH                                0x03
#define IMEAS_INT                                   0x04
#define IMEAS_REG_SEQ                               0x05
#define IMEAS_REG_RSTVAL                            0x06
#define IMEAS_CH0DATA_0                             0x07
#define IMEAS_CH0DATA_1                             0x08
#define IMEAS_CH1DATA_0                             0x09
#define IMEAS_CH1DATA_1                             0x0A
#define IMEAS_CH2DATA_0                             0x0B
#define IMEAS_CH2DATA_1                             0x0C
#define IMEAS_GRP_CTL                               0x0D
#define IMEAS_CHA_NUM_LO                            0x0E
#define IMEAS_CHA_NUM_HI                            0x0F
#define MEAS_SYNC_EN                                0x10

#define IMEAS_ALARM_INT                             0x11
#define IMEAS_ALARM_INT_EN                          0x12
#define IMEAS_THRESHOLD_HI_0                        0x13
#define IMEAS_THRESHOLD_HI_1                        0x14
#define IMEAS_THRESHOLD_LO_0                        0x15
#define IMEAS_THRESHOLD_LO_1                        0x16
#define IMEAS_INPUT_FORMAT                          0x17
#define IMEAS_EN                                    0x18 



#define ZMEAS_REG_CTRL_0                            0x19
#define ZMEAS_REG_CTRL_1                            0x1A
#define ZMEAS_REG_CTRL_2                            0x1B
#define ZMEAS_REG_CTRL_3                            0x1C
#define ZMEAS_REG_STATUS_0                          0x1D
#define ZMEAS_REG_STATUS_1                          0x1E
#define ZMEAS_REG_DATAOUT_0                         0x1F
#define ZMEAS_REG_DATAOUT_1                         0x20
#define ZMEAS_REG_DATAOUT_2                         0x21
#define ZMEAS_REG_DATAOUT_3                         0x22

#define ZMEAS_ADC_ROM_REG_0                         0x23 //24
#define ZMEAS_ADC_ROM_REG_1                         0x24
#define ZMEAS_ADC_ROM_REG_2                         0x25
#define ZMEAS_ADC_ROM_REG_3                         0x26
#define ZMEAS_SUMMATION_OFFSET_FORREAL_0            0x27
#define ZMEAS_SUMMATION_OFFSET_FORREAL_1            0x28
#define ZMEAS_SUMMATION_OFFSET_FORREAL_2            0x29
#define ZMEAS_SUMMATION_OFFSET_FORREAL_3            0x2A
#define ZMEAS_SUMMATION_REAL_0                      0x2B
#define ZMEAS_SUMMATION_REAL_1                      0x2C
#define ZMEAS_SUMMATION_REAL_2                      0x2D
#define ZMEAS_SUMMATION_REAL_3                      0x2E
#define ZMEAS_SUMMATION_IMAG_0                      0x2F
#define ZMEAS_SUMMATION_IMAG_1                      0x30
#define ZMEAS_SUMMATION_IMAG_2                      0x31
#define ZMEAS_SUMMATION_IMAG_3                      0x32
#define ZMEAS_SUMMATION_SHIFT_0                     0x33
#define ZMEAS_SUMMATION_SHIFT_1                     0x34
#define ZMEAS_SUMMATION_SHIFT_2                     0x35
#define ZMEAS_SUMMATION_SHIFT_3                     0x36
#define ZMEAS_INT                                   0x37
#define ZMEAS_ADC_INT                               0x38
#define ZMEAS_EN                                    0x39


#define CLK_CTRL_REG                                0x3A
#define PMU_REG0                                    0x3B 

#define ALWAYS_ON_CLK_CTRL                          0x3C

#define FLASH_DEBUG1                                0x3D
#define FLASH_DEBUG2                                0x3E

#define FLASH_TRIMDATA0                             0x3F
#define FLASH_TRIMDATA1                             0x40
#define FLASH_TRIMDATA2                             0x41
#define FLASH_TRIMDATA3                             0x42
#define FLASH_TRIMDATA4                             0x43
#define FLASH_TRIMDATA5                             0x44
#define FLASH_TRIMDATA6                             0x45


//FIFO Registers
#define FIFO_WR_PTR_REG                             0x46
#define FIFO_RD_PTR_REG                             0x47
#define FIFO_COUNTER_1_REG                          0x48
#define FIFO_COUNTER_2_REG                          0x49
#define FIFO_CONFIG_1_REG                           0x4A
#define FIFO_CONFIG_2_REG                           0x4B
#define FIFO_CONFIG_3_REG                           0x4C
#define FIFO_STATUS_REG                             0x4D
#define FIFO_DATA_REG1                              0x4E
#define FIFO_DATA_REG2                              0x4F




//analog register define
#define ANA_PMU                                     0x50
#define ANA_TSC_0                                   0x51
#define ANA_TSC_1                                   0x52


#define ANA_IMEAS_CH1_WE1_0                         0x53
#define ANA_IMEAS_CH1_WE1_1                         0x54
#define ANA_IMEAS_CH1_WE2_0                         0x55
#define ANA_IMEAS_CH1_WE2_1                         0x56
#define ANA_IMEAS_CH1_RCE_ROUTSEL                   0x57
#define ANA_IMEAS_CH1_WE_DAC_EN                     0x58
#define ANA_IMEAS_CH1_DINWE_0                       0x59
#define ANA_IMEAS_CH1_DINWE_1                       0x5A
#define ANA_IMEAS_CH1_RCE_DAC_EN                    0x5B
#define ANA_IMEAS_CH1_DINRCE_0                      0x5C
#define ANA_IMEAS_CH1_DINRCE_1                      0x5D


#define ANA_BIST                                    0x5E
#define ANA_DDA                                     0x5F
#define ANA_PGA                                     0x60
#define ANA_ELE                                     0x61

#define ANA_SDM                                     0x62


//xin add 2/Oct/2022
#define ANA_Z_ADC_DAC_EN                            0x63
#define ANA_Z_ADC_DAC_EN_SEL                        0x64

//device status 
#define DEVICE_INT_STATUS_0                         0x65
#define DEVICE_INT_STATUS_1                         0x66

//Analog debug regsiters
#define ALWAYS_ON_CLK_CTRL_DEBUG                    0x67

#define ALWAYS_ON_ANA_IMEAS_CH1_WE1_0_DEBUG         0x68
#define ALWAYS_ON_ANA_IMEAS_CH1_WE1_1_DEBUG         0x69
#define ALWAYS_ON_ANA_IMEAS_CH1_WE2_0_DEBUG         0x6A
#define ALWAYS_ON_ANA_IMEAS_CH1_WE2_1_DEBUG         0x6B
#define ALWAYS_ON_ANA_IMEAS_CH1_RCE_ROUTSEL_DEBUG   0x6C
#define ALWAYS_ON_ANA_IMEAS_CH1_WE_DAC_EN_DEBUG     0x6D
#define ALWAYS_ON_ANA_IMEAS_CH1_DINWE_0_DEBUG       0x6E
#define ALWAYS_ON_ANA_IMEAS_CH1_DINWE_1_DEBUG       0x6F
#define ALWAYS_ON_ANA_IMEAS_CH1_RCE_DAC_EN_DEBUG    0x70
#define ALWAYS_ON_ANA_IMEAS_CH1_DINRCE_0_DEBUG      0x71
#define ALWAYS_ON_ANA_IMEAS_CH1_DINRCE_1_DEBUG      0x72


#define ALWAYS_ON_ANA_PMU_DEBUG                     0x73


#define ALWAYS_ON_ANA_BIST_DEBUG                    0x74
#define ALWAYS_ON_ANA_DDA_DEBUG                     0x75
#define ALWAYS_ON_ANA_ELE_DEBUG                     0x76


#define ALWAYS_ON_ANA_BGH_VTRIM_DEBUG               0x77
#define ALWAYS_ON_ANA_BGH_CTRIM_DEBUG               0x78
#define ALWAYS_ON_ANA_BGL_VTRIM_DEBUG               0x79
#define ALWAYS_ON_ANA_BGL_CTRIM_DEBUG               0x7A
#define ALWAYS_ON_ANA_LDOL1V5_TRIM_DEBUG            0x7B
#define ALWAYS_ON_ANA_DAC_BUF_TRIM_DEBUG            0x7C
#define ALWAYS_ON_ANA_OSC_TRIM_DEBUG                0x7D

#define	 WR_BURST_CMD                               0xA0
#define	 WR_SINGLE_CMD                              0x80
#define  RD_SINGLE_CMD                              0x00
#define	 RD_BURST_FIFO_CMD                          0x60
#define	 RD_BURST_REG_CMD                           0x20

#define	 PAD                                        0x0
#define ELE_BUF  0xf
#define CHA_NUM  0x54


/* Private typedef -----------------------------------------------------------*/
typedef void (*bms003_irq_callback)(void);


/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/
void bms003_init(void);
bool bms003_new_data_is_ready(void);
bool bms003_get_new_data(double* pNewData);
void bms003_register_irq_callback(bms003_irq_callback callback);
void bms003_start(void);
void bms003_stop(void);
#endif /* __BMS003_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/

