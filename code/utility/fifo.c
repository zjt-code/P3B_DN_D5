/****************************************************************************
* File Name          :  fifo.h
* Author             :  陈苏阳
* CPU Type           :  TLS8253
* IDE                :  Telink IDE
* Version            :  V1.0
* Date               :  2020/07/26
* Description        :  fifo drivers
****************************************************************************/
/* Includes ---------------------------------------------------------------*/
#include "fifo.h"
#include "string.h"
/* Private define --------------------------------------------------------*/

/* Private typedef --------------------------------------------------------*/

/* Private variables ------------------------------------------------------*/

/****************************************************************************
*                        陈苏阳@2020/07/26
* Function Name  :  fifo_create
* Description    :  创建FIFO环形队列
* Input          :  pFifo：环形队列指针
*                   pvBuffer：环形队列所指向的内存块
*                   uiWidth：原数据宽度
*                   uiDepth: 队列深度
* Output         :  None
* Return         :  uint8_t
*****************************************************************************/
uint8_t fifo_create(fifo_t *pFifo, void *pvBuffer, uint32_t uiWidth, uint32_t uiDepth)
{
    pFifo->pvBuffer = pvBuffer;
    pFifo->uiWidth = uiWidth;
    pFifo->uiDepth = uiDepth;
    pFifo->uiIn = 0;
    pFifo->uiOut = 0;
    return 1;
}


/****************************************************************************
*                        陈苏阳@2020/07/26
* Function Name  :  fifo_out
* Description    :  读取环形队列中的数据
* Input          :  pFifo：环形队列指针
*                   uiReqNum：读取数据个数
*                   ucNextFlag: 数据读出后是否要移至下一个数据
* Output         :  读出的数据内容
* Return         :  读出的数据个数
*****************************************************************************/
uint32_t fifo_out(fifo_t *pFifo, void *pvBuffer, uint32_t uiReqNum, uint8_t ucNextFlag)
{
    uint32_t i;
    uint32_t uiIn = pFifo->uiIn;
    uint32_t uiOut = pFifo->uiOut;
    uint32_t uiWidth = pFifo->uiWidth;
    uint8_t *pBuffer = (uint8_t *)pvBuffer;
    
    if (uiReqNum == 0)
    {
        return 0;
    }
    
    for (i = 0; i < uiReqNum; i++)
    {
        if (uiOut == uiIn)
        {
            break;
        }
        memcpy(pBuffer, &pFifo->pvBuffer[uiOut * uiWidth], uiWidth);
        pBuffer += uiWidth;
        uiOut++;
        if (uiOut >= pFifo->uiDepth)
        {
            uiOut = 0;
        }
    }
    if (ucNextFlag != 0)
    {
        pFifo->uiOut = uiOut;
    }
    
    return i;
}


/****************************************************************************
*                        陈苏阳@2020/07/26
* Function Name  :  fifo_in
* Description    :  环形队列写入数据
* Input          :  pFifo：环形队列指针
*                   pvBuffer：写入队列中的数据
*                   uiReqNum：写入的数据个数
* Output         :  None
* Return         :  OK=1,ERROR=0
*****************************************************************************/
uint8_t fifo_in(fifo_t *pFifo, void *pvBuffer, uint32_t uiReqNum)
{
    uint32_t i;
    uint32_t uiIn = pFifo->uiIn;
    uint32_t uiOut = pFifo->uiOut;
    uint32_t uiWidth = pFifo->uiWidth;
    uint8_t *pBuffer = (uint8_t *)pvBuffer;
    
    if (uiReqNum == 0)
    {
        return 0;
    }
    
    for (i = 0; i < uiReqNum; i++)
    {
        memcpy(&pFifo->pvBuffer[uiIn * uiWidth], pBuffer, uiWidth);
        pBuffer += uiWidth;
        uiIn++;
        if (uiIn >= pFifo->uiDepth)
        {
            uiIn = 0;
        }
        if (uiIn == uiOut)
        {
            if (uiIn == 0)
            {
                uiIn = pFifo->uiDepth - 1;
            }
            else
            {
                uiIn--;
            }
            break;
        }
    }
    pFifo->uiIn = uiIn;
    return 1;
}

/****************************************************************************
*                        陈苏阳@2020/07/26
* Function Name  :  fifo_flush
* Description    :  清空环形队列数据
* Input          :  pFifo：环形队列指针
* Output         :  None
* Return         :  None
*****************************************************************************/
void fifo_flush(fifo_t *pFifo)
{
    pFifo->uiIn = 0;
    pFifo->uiOut = 0;
}

/****************************************************************************
*                        陈苏阳@2020/07/26
* Function Name  :  fifo_len
* Description    :  获取环形队列数据个数
* Input          :  pFifo：环形队列指针
* Output         :  None
* Return         :  队列数据个数
*****************************************************************************/
uint32_t fifo_len(fifo_t *pFifo)
{
    if(pFifo->uiIn >= pFifo->uiOut)
    {
        return pFifo->uiIn - pFifo->uiOut;
    }
    return pFifo->uiIn + pFifo->uiDepth - pFifo->uiOut;
}

