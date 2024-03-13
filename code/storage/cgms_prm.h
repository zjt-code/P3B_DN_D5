/******************** (C) COPYRIGHT 2023 ³ÂËÕÑô ********************************
* File Name          :  cgms_prm.h
* Author             :  ³ÂËÕÑô
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  30/11/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CGMS_PRM_H
#define __CGMS_PRM_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "cgms_db.h"
#include "app.h"


typedef struct
{
    uint8_t           prmWMY[4];
    uint16_t          SN;
    uint16_t          Crc16;
} prm_t;




#define LEN_MAX_ERROR_FILE_NAME   128

typedef struct
{
    uint32_t        id;
    uint32_t        line_num;    /**< The line number where the error occurred. */

    uint32_t        err_code;
    uint8_t         file_name[LEN_MAX_ERROR_FILE_NAME]; /**< The file in which the error occurred. */
    uint32_t        err_enter_pos;

    uint16_t   	 reserved;
    uint16_t   	 crc_value;


}
softreset_error_log_backup_t;






extern prm_t g_PrmDb;
extern uint8_t g_ucSn[];

/* Private function prototypes -----------------------------------------------*/
ret_code_t cgms_prm_get_sn(unsigned char* buff);
void cgms_prm_db_power_on_init(void);
uint8_t* cgms_prm_get_sn_p(void);
bool cgms_prm_get_bonded_flag(void);
ret_code_t cgms_prm_db_write_flash(void);

extern softreset_error_log_backup_t softreset_error_log;

#endif /* __CGMS_PRM_H */

/******************* (C) COPYRIGHT 2023 ³ÂËÕÑô **** END OF FILE ****************/