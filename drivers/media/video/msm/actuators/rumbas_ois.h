#ifndef RUMBAS_OIS_H_
#define RUMBAS_OIS_H_

#define DEFAULT_OIS_LEVEL 4       
#define DEFAULT_MAX_ANGLE_COM 500 
#define ENDIAN(i)((i>>8)&0xff)+((i << 8)&0xff00)
#define ENDIAN32(i)((i>>24)&0xff)+((i >> 8)&0xff00)+((i << 8)&0xff0000)+((i << 24)&0xff000000)

static unsigned int tbl_threshold[5] = {1284, 800, 7000, 15000, 60000};
static unsigned int mapping_tbl[9][2] = {{7,800},{7,700},{6,500},{7,900},{7,800},{6,500},{7,1000},{7,900},{6,500}};
static unsigned int mfg_tbl_threshold[5] = {2399, 810, 7000, 15000, 600000000};
static unsigned int mfg_mapping_tbl[9][2] = {{4,500},{4,500},{4,500},{4,500},{4,500},{4,500},{4,500},{4,500},{4,500}};

static unsigned int *tbl_threshold_ptr;
typedef unsigned int table_data[2];
static table_data *mapping_tbl_ptr;

static uint8_t ois_off_tbl  = 1;

#if 0
static uint8_t ois_level_value = 0;
static uint8_t pan_mode_enable = 0;
static uint8_t tri_moode_enable = 0;
#endif

typedef enum{
USER_TABLE,
MFG_TABLE,
DEBUG_TABLE
}table_type;

#define OIS_READY_VERSION 0X34


#endif
