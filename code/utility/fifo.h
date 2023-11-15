/****************************************************************************
* File Name          :  fifo.h
* Author             :  ������
* CPU Type           :  NRF52832
* IDE                :  Telink IDE
* Version            :  V1.0
* Date               :  2020/07/26
* Description        :  fifo drivers header
****************************************************************************/

#ifndef __FIFO_H__
#define __FIFO_H__

/* Includes ---------------------------------------------------------------*/
#include "stdint.h"
/* Public typedef ---------------------------------------------------------*/
typedef struct
{
    uint8_t* pvBuffer;
    uint32_t uiWidth;
    uint32_t uiDepth;
    uint32_t uiIn;
    uint32_t uiOut;
}fifo_t;


/* Public Type ------------------------------------------------------------*/

/* Function declaration ---------------------------------------------------*/
uint8_t fifo_create(fifo_t*pFifo, void *pvBuffer,uint32_t uiWidth, uint32_t uiDepth);   // �������ζ���
uint32_t fifo_out(fifo_t*pFifo, void *pvBuffer, uint32_t uiReqNum, uint8_t ucNextFlag); // ��ȡ����������
uint8_t fifo_in(fifo_t*pFifo, void *pvBuffer, uint32_t uiReqNum);                       // ����д������
void fifo_flush(fifo_t*pFifo);                                                          // ��ն�������
uint32_t fifo_len(fifo_t*pFifo);                                                        // ��ȡ�������ݸ���

#endif /* __FIFO_H__ */
