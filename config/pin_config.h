#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// $[CMU]
// [CMU]$

// $[LFXO]
// [LFXO]$

// $[PRS.ASYNCH0]
// [PRS.ASYNCH0]$

// $[PRS.ASYNCH1]
// [PRS.ASYNCH1]$

// $[PRS.ASYNCH2]
// [PRS.ASYNCH2]$

// $[PRS.ASYNCH3]
// [PRS.ASYNCH3]$

// $[PRS.ASYNCH4]
// [PRS.ASYNCH4]$

// $[PRS.ASYNCH5]
// [PRS.ASYNCH5]$

// $[PRS.ASYNCH6]
// [PRS.ASYNCH6]$

// $[PRS.ASYNCH7]
// [PRS.ASYNCH7]$

// $[PRS.ASYNCH8]
// [PRS.ASYNCH8]$

// $[PRS.ASYNCH9]
// [PRS.ASYNCH9]$

// $[PRS.ASYNCH10]
// [PRS.ASYNCH10]$

// $[PRS.ASYNCH11]
// [PRS.ASYNCH11]$

// $[PRS.SYNCH0]
// [PRS.SYNCH0]$

// $[PRS.SYNCH1]
// [PRS.SYNCH1]$

// $[PRS.SYNCH2]
// [PRS.SYNCH2]$

// $[PRS.SYNCH3]
// [PRS.SYNCH3]$

// $[GPIO]
// [GPIO]$

// $[TIMER0]
// [TIMER0]$

// $[TIMER1]
// [TIMER1]$

// $[TIMER2]
// [TIMER2]$

// $[TIMER3]
// [TIMER3]$

// $[TIMER4]
// [TIMER4]$

// $[USART0]
// [USART0]$

// $[USART1]
// USART1 CLK on PA05
#ifndef USART1_CLK_PORT                         
#define USART1_CLK_PORT                          gpioPortA
#endif
#ifndef USART1_CLK_PIN                          
#define USART1_CLK_PIN                           5
#endif

// USART1 RX on PA03
#ifndef USART1_RX_PORT                          
#define USART1_RX_PORT                           gpioPortA
#endif
#ifndef USART1_RX_PIN                           
#define USART1_RX_PIN                            3
#endif

// USART1 TX on PA04
#ifndef USART1_TX_PORT                          
#define USART1_TX_PORT                           gpioPortA
#endif
#ifndef USART1_TX_PIN                           
#define USART1_TX_PIN                            4
#endif

// [USART1]$

// $[I2C1]
// [I2C1]$

// $[PDM]
// [PDM]$

// $[ETAMPDET]
// [ETAMPDET]$

// $[LETIMER0]
// [LETIMER0]$

// $[IADC0]
// IADC0 POS on PB00
#ifndef IADC0_POS_PORT                          
#define IADC0_POS_PORT                           gpioPortB
#endif
#ifndef IADC0_POS_PIN                           
#define IADC0_POS_PIN                            0
#endif

// [IADC0]$

// $[ACMP0]
// [ACMP0]$

// $[I2C0]
// [I2C0]$

// $[EUSART0]
// [EUSART0]$

// $[PTI]
// [PTI]$

// $[MODEM]
// [MODEM]$

// $[CUSTOM_PIN_NAME]
#ifndef SPI_MISO_PORT                           
#define SPI_MISO_PORT                            gpioPortA
#endif
#ifndef SPI_MISO_PIN                            
#define SPI_MISO_PIN                             3
#endif

#ifndef SPI_MOSI_PORT                           
#define SPI_MOSI_PORT                            gpioPortA
#endif
#ifndef SPI_MOSI_PIN                            
#define SPI_MOSI_PIN                             4
#endif

#ifndef SPI_CLK_PORT                            
#define SPI_CLK_PORT                             gpioPortA
#endif
#ifndef SPI_CLK_PIN                             
#define SPI_CLK_PIN                              5
#endif

#ifndef SPI_CS_PORT                             
#define SPI_CS_PORT                              gpioPortA
#endif
#ifndef SPI_CS_PIN                              
#define SPI_CS_PIN                               6
#endif

#ifndef AFE_INT_PORT                            
#define AFE_INT_PORT                             gpioPortA
#endif
#ifndef AFE_INT_PIN                             
#define AFE_INT_PIN                              7
#endif

#ifndef NTC_ADC_PORT                            
#define NTC_ADC_PORT                             gpioPortB
#endif
#ifndef NTC_ADC_PIN                             
#define NTC_ADC_PIN                              0
#endif

#ifndef NTC_EN_PORT                             
#define NTC_EN_PORT                              gpioPortB
#endif
#ifndef NTC_EN_PIN                              
#define NTC_EN_PIN                               1
#endif

#ifndef AFE_WAKE_UP_PORT                        
#define AFE_WAKE_UP_PORT                         gpioPortB
#endif
#ifndef AFE_WAKE_UP_PIN                         
#define AFE_WAKE_UP_PIN                          2
#endif

#ifndef AFE_CHIP_EN_PORT                        
#define AFE_CHIP_EN_PORT                         gpioPortB
#endif
#ifndef AFE_CHIP_EN_PIN                         
#define AFE_CHIP_EN_PIN                          3
#endif

// [CUSTOM_PIN_NAME]$

#endif // PIN_CONFIG_H

