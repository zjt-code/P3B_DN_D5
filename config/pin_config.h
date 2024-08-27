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
// USART0 CLK on PC03
#ifndef USART0_CLK_PORT                         
#define USART0_CLK_PORT                          gpioPortC
#endif
#ifndef USART0_CLK_PIN                          
#define USART0_CLK_PIN                           3
#endif

// USART0 CS on PC04
#ifndef USART0_CS_PORT                          
#define USART0_CS_PORT                           gpioPortC
#endif
#ifndef USART0_CS_PIN                           
#define USART0_CS_PIN                            4
#endif

// USART0 RX on PB01
#ifndef USART0_RX_PORT                          
#define USART0_RX_PORT                           gpioPortB
#endif
#ifndef USART0_RX_PIN                           
#define USART0_RX_PIN                            1
#endif

// USART0 TX on PA03
#ifndef USART0_TX_PORT                          
#define USART0_TX_PORT                           gpioPortA
#endif
#ifndef USART0_TX_PIN                           
#define USART0_TX_PIN                            3
#endif

// [USART0]$

// $[USART1]
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
// [IADC0]$

// $[ACMP0]
// [ACMP0]$

// $[I2C0]
// [I2C0]$

// $[EUSART0]
// EUSART0 CS on PC06
#ifndef EUSART0_CS_PORT                         
#define EUSART0_CS_PORT                          gpioPortC
#endif
#ifndef EUSART0_CS_PIN                          
#define EUSART0_CS_PIN                           6
#endif

// EUSART0 RX on PB02
#ifndef EUSART0_RX_PORT                         
#define EUSART0_RX_PORT                          gpioPortB
#endif
#ifndef EUSART0_RX_PIN                          
#define EUSART0_RX_PIN                           2
#endif

// EUSART0 SCLK on PC03
#ifndef EUSART0_SCLK_PORT                       
#define EUSART0_SCLK_PORT                        gpioPortC
#endif
#ifndef EUSART0_SCLK_PIN                        
#define EUSART0_SCLK_PIN                         3
#endif

// EUSART0 TX on PA05
#ifndef EUSART0_TX_PORT                         
#define EUSART0_TX_PORT                          gpioPortA
#endif
#ifndef EUSART0_TX_PIN                          
#define EUSART0_TX_PIN                           5
#endif

// [EUSART0]$

// $[PTI]
// [PTI]$

// $[MODEM]
// [MODEM]$

// $[CUSTOM_PIN_NAME]
#ifndef _PORT                                   
#define _PORT                                    gpioPortA
#endif
#ifndef _PIN                                    
#define _PIN                                     0
#endif

#ifndef SPI_MOSI_PORT                           
#define SPI_MOSI_PORT                            gpioPortA
#endif
#ifndef SPI_MOSI_PIN                            
#define SPI_MOSI_PIN                             3
#endif

#ifndef AFE_INT_PORT                            
#define AFE_INT_PORT                             gpioPortA
#endif
#ifndef AFE_INT_PIN                             
#define AFE_INT_PIN                              5
#endif

#ifndef SPI_MISO_PORT                           
#define SPI_MISO_PORT                            gpioPortB
#endif
#ifndef SPI_MISO_PIN                            
#define SPI_MISO_PIN                             1
#endif

#ifndef AFE_WAKE_UP_PORT                        
#define AFE_WAKE_UP_PORT                         gpioPortB
#endif
#ifndef AFE_WAKE_UP_PIN                         
#define AFE_WAKE_UP_PIN                          2
#endif

#ifndef SPI_CLK_PORT                            
#define SPI_CLK_PORT                             gpioPortC
#endif
#ifndef SPI_CLK_PIN                             
#define SPI_CLK_PIN                              3
#endif

#ifndef SPI_CS_PORT                             
#define SPI_CS_PORT                              gpioPortC
#endif
#ifndef SPI_CS_PIN                              
#define SPI_CS_PIN                               4
#endif

#ifndef AFE_CHIP_EN_PORT                        
#define AFE_CHIP_EN_PORT                         gpioPortC
#endif
#ifndef AFE_CHIP_EN_PIN                         
#define AFE_CHIP_EN_PIN                          6
#endif

// [CUSTOM_PIN_NAME]$

#endif // PIN_CONFIG_H

