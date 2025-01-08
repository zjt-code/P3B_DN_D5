#ifndef CGMS_PRM_H_
#define CGMS_PRM_H_

/* ----------------------------------------------------------------------------
 * If building with a C++ compiler, make all of the definitions in this header
 * have a C binding.
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C"
{
#endif    /* ifdef __cplusplus */


//#include "sdk_errors.h"

#include "stdint.h"
#include "app.h"
//#include "ble_cgms.h"


typedef struct
{
  uint16_t          rsvd; //for 4 byte alignment
  int16_t		    prmOffset;
  uint16_t          prmVD1;             
  uint16_t          prmRL1;               
  int16_t         	prmAD1;
  uint16_t          prmRL2;               
  int16_t         	prmAD2;	
  uint16_t          prmCrc1; 
} P1_t;

typedef struct
{
  uint16_t          prmVD2;             
  uint16_t          prmRL2;               
  uint16_t         	prmAD2;              
  uint16_t          prmCrc2;             
} P2_t;

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


extern prm_t m_prm_db;

uint32_t cgms_prm_db_init(void);

//ret_code_t cgms_prm_db_write_flash();//add by woo

//====================
//batt life calc


#define BATT_MAX_HOUR		 4000 //UINIT 0.1Hr,400.0Hr under 30K
#define BATT_CNT_PRD		 360 //period of battCnt ,360s = 0.1Hr
#define BATT_ADC_SAMPLE_PRD  60  // battery ADC sample period 60s

extern void cgms_batt_reset();
//====================


/** @} */
#ifdef __cplusplus
}
#endif


#endif /* CGMS_PRM_H_ */
