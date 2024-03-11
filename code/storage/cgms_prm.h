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
    uint16_t          rsvd; //for 4 byte alignment
    int16_t		        prmOffset;
    uint16_t          prmVD1;
    uint16_t          prmRL1;
    int16_t         	prmAD1;
    uint16_t          prmRL2;
    int16_t         	prmAD2;
    uint16_t          prmCrc1;
}P1_t;

typedef struct
{
    uint16_t          prmVD2;
    uint16_t          prmRL2;
    uint16_t         	prmAD2;
    uint16_t          prmCrc2;
}P2_t;

typedef struct
{
    uint8_t          prmDX0;
    uint8_t          prmDX1;
    uint8_t          prmDX2;
    uint8_t          prmDX3;
    uint8_t          prmDX4;
    uint8_t          prmDX5;
    uint8_t          prmDX6;
    uint8_t          prmDX7;
    uint8_t          prmDX8;
    uint8_t          prmDX9;
    uint8_t          prmDXA;
    uint8_t          prmDXB;
    uint16_t         prmDXC;
    //uint8_t          prmDXD;	//ADB_WOO 2021.11.11
    uint16_t         prmCrc3;
} P3_t;


typedef struct
{
    uint16_t  P500;
    uint16_t  P501;
    uint16_t  P502;
    uint16_t  P503;
    uint16_t  P504;
    uint16_t  P505;
    uint16_t  P506;
    uint16_t  P507;
    uint16_t  P508;
    uint16_t  P509;
    uint16_t  P510;
    uint16_t  P511;
    uint16_t  P512;
    uint16_t  P513;
    uint16_t  P514;
    uint16_t  Crc;

}P5_t;


typedef struct
{
    uint8_t           prmWMY[4];
    uint16_t          SN;
    uint16_t          prmCrc4;
} P4_t;

typedef struct
{
    uint8_t          data[20];
} Bond_t;

typedef struct
{
    P1_t          Pone;   // 16 bytes
    P2_t          Ptwo;   // 8 bytes
    P3_t          P3;    // 16 bytes
    P4_t          P4;   //8 bytes
    P5_t          P5;  // 32 bytes
    //Bond_t        Bond_data;
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