/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  ltcgm1272.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  6/11/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LTCGM1272_H
#define __LTCGM1272_H

/* Includes ------------------------------------------------------------------*/
#include "stdbool.h"

/* Private define ------------------------------------------------------------*/



// define the CGM1272 register values 

//adc_ctrl register	address
#define CGM1272_CONFIG                              0x00
#define CGM1272_DATA_READY                          0x01
#define CGM1272_ADC_RESULT_L                        0x02
#define CGM1272_ADC_RESULT_H                        0x03
#define CGM1272_ADFCLK_DIV                          0x04
#define CGM1272_TSAMPLE                             0x05

//power_ctrl register address
#define CGM1272_POWER_CONTROL                       0x10
#define CGM1272_ODR_SAMPLE_RATE                     0x11
#define CGM1272_OPRCE_CTRL                          0x12
#define CGM1272_OPTIA_CTRL                          0x13   
#define CGM1272_POSTSTA_SEL                         0x14
#define CGM1272_POSTSTA_SW                          0x15
#define CGM1272_BUFF_CTRL                           0x16
#define CGM1272_DAC2_DATA                           0x17
#define CGM1272_DAC1_DATA                           0x18
#define CGM1272_DAC_CTRL                            0x19
#define CGM1272_FIFO_WATER_LINE                     0x1A

//ibus2efuse register address
#define CGM1272_CYCLE                               0x30
#define CGM1272_BGLP_TRIM_R                         0x31
#define CGM1272_IBLP_TRIM_R                         0x32
#define CGM1272_OPRCE_POS_TRIM_R                    0x33
#define CGM1272_OPRTIA_POS_TRIM_R                   0x34
#define CGM1272_OSC32K_TRIM_R                       0x35
#define CGM1272_OSC4M_TRIM_R                        0x36
#define CGM1272_CHIP_ID_R                           0x37
#define CGM1272_EFUSE_VALID_FLAG_R                  0x3C
#define CGM1272_ERROR                               0x3D
#define CGM1272_ADDR                                0x3D

//misc register address
#define CGM1272_LEFT_DATA_NUM                       0x40
#define CGM1272_FIFO_RESULT_L                       0x41
#define CGM1272_FIFO_RESULT_H                       0x42
#define CGM1272_TEST_CTRL                           0x43
#define CGM1272_SOFT_RESET_CLR                      0x44
#define CGM1272_TEST_CLK_CTRL                       0x45



/* Private typedef -----------------------------------------------------------*/
typedef void (*ltcgm1272_irq_callback)(void);


/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/


void ltcgm1272_start(void);
void ltcgm1272_stop(void);
bool ltcgm1272_new_data_is_ready(void);
bool ltcgm1272_get_new_data(double* pNewData);
void ltcgm1272_init(void);
void ltcgm1272_register_irq_callback(ltcgm1272_irq_callback callback);
#endif /* __LTCGM1272_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/
