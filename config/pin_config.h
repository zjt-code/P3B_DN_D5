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
// EUSART0 CS on PC04
#ifndef EUSART0_CS_PORT                         
#define EUSART0_CS_PORT                          gpioPortC
#endif
#ifndef EUSART0_CS_PIN                          
#define EUSART0_CS_PIN                           4
#endif

// EUSART0 CTS on PA02
#ifndef EUSART0_CTS_PORT                        
#define EUSART0_CTS_PORT                         gpioPortA
#endif
#ifndef EUSART0_CTS_PIN                         
#define EUSART0_CTS_PIN                          2
#endif

// EUSART0 RTS on PA03
#ifndef EUSART0_RTS_PORT                        
#define EUSART0_RTS_PORT                         gpioPortA
#endif
#ifndef EUSART0_RTS_PIN                         
#define EUSART0_RTS_PIN                          3
#endif

// EUSART0 RX on PC02
#ifndef EUSART0_RX_PORT                         
#define EUSART0_RX_PORT                          gpioPortC
#endif
#ifndef EUSART0_RX_PIN                          
#define EUSART0_RX_PIN                           2
#endif

// EUSART0 SCLK on PC06
#ifndef EUSART0_SCLK_PORT                       
#define EUSART0_SCLK_PORT                        gpioPortC
#endif
#ifndef EUSART0_SCLK_PIN                        
#define EUSART0_SCLK_PIN                         6
#endif

// EUSART0 TX on PC05
#ifndef EUSART0_TX_PORT                         
#define EUSART0_TX_PORT                          gpioPortC
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

#ifndef AFE_CHIP_EN_PORT                        
#define AFE_CHIP_EN_PORT                         gpioPortA
#endif
#ifndef AFE_CHIP_EN_PIN                         
#define AFE_CHIP_EN_PIN                          5
#endif

#ifndef AFE_INT_PORT                            
#define AFE_INT_PORT                             gpioPortA
#endif
#ifndef AFE_INT_PIN                             
#define AFE_INT_PIN                              6
#endif

#ifndef MCU_POWER_LOCK_PORT                     
#define MCU_POWER_LOCK_PORT                      gpioPortB
#endif
#ifndef MCU_POWER_LOCK_PIN                      
#define MCU_POWER_LOCK_PIN                       1
#endif

#ifndef TMODE1_PORT                             
#define TMODE1_PORT                              gpioPortB
#endif
#ifndef TMODE1_PIN                              
#define TMODE1_PIN                               2
#endif

#ifndef AFE_WAKE_UP_PORT                        
#define AFE_WAKE_UP_PORT                         gpioPortC
#endif
#ifndef AFE_WAKE_UP_PIN                         
#define AFE_WAKE_UP_PIN                          1
#endif

#ifndef SPI_MISO_PORT                           
#define SPI_MISO_PORT                            gpioPortC
#endif
#ifndef SPI_MISO_PIN                            
#define SPI_MISO_PIN                             2
#endif

#ifndef SPI_CS_PORT                             
#define SPI_CS_PORT                              gpioPortC
#endif
#ifndef SPI_CS_PIN                              
#define SPI_CS_PIN                               4
#endif

#ifndef SPI_MOSI_PORT                           
#define SPI_MOSI_PORT                            gpioPortC
#endif
#ifndef SPI_MOSI_PIN                            
#define SPI_MOSI_PIN                             5
#endif

#ifndef SPI_CLK_PORT                            
#define SPI_CLK_PORT                             gpioPortC
#endif
#ifndef SPI_CLK_PIN                             
#define SPI_CLK_PIN                              6
#endif

// [CUSTOM_PIN_NAME]$

#endif // PIN_CONFIG_H

