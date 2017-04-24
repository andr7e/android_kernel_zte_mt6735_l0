#ifndef _H_DDP_HAL_
#define _H_DDP_HAL_

/* DISP Mutex */
#define DISP_MUTEX_TOTAL      (10)
#define DISP_MUTEX_DDP_FIRST  (0)
#define DISP_MUTEX_DDP_LAST   (4)
#define DISP_MUTEX_DDP_COUNT  (5)
#define DISP_MUTEX_MDP_FIRST  (5)
#define DISP_MUTEX_MDP_COUNT  (5)

/* DISP MODULE */
typedef enum
{    
    DISP_MODULE_OVL0 = 0,
    DISP_MODULE_OVL1  ,
    DISP_MODULE_RDMA0 ,
    DISP_MODULE_RDMA1 ,
    DISP_MODULE_WDMA0 ,
    DISP_MODULE_COLOR0,
    DISP_MODULE_CCORR ,
    DISP_MODULE_AAL   ,
    DISP_MODULE_GAMMA ,
    DISP_MODULE_DITHER,
    DISP_MODULE_UFOE  , //10
    DISP_MODULE_PWM0   ,
    DISP_MODULE_WDMA1 ,
    DISP_MODULE_DSI0  ,
    DISP_MODULE_DPI0   ,
    DISP_MODULE_DPI1  ,
    DISP_MODULE_SMI,
    DISP_MODULE_CONFIG,
    DISP_MODULE_CMDQ,
    DISP_MODULE_MUTEX,	

    DISP_MODULE_COLOR1,
    DISP_MODULE_RDMA2,
    DISP_MODULE_PWM1,
    DISP_MODULE_OD,
    DISP_MODULE_MERGE,
    DISP_MODULE_SPLIT0,
    DISP_MODULE_SPLIT1,
    DISP_MODULE_DSI1,
    DISP_MODULE_DSIDUAL,    

	DISP_MODULE_SMI_LARB0 ,
    DISP_MODULE_SMI_COMMON,
    DISP_MODULE_UNKNOWN, //20
    DISP_MODULE_NUM
} DISP_MODULE_ENUM;

typedef enum
{
    DISP_REG_OVL0   ,
    DISP_REG_OVL1   ,
    DISP_REG_RDMA0  ,
    DISP_REG_RDMA1  ,
    DISP_REG_WDMA0  ,
    DISP_REG_COLOR  ,
    DISP_REG_CCORR  ,
    DISP_REG_AAL    ,
    DISP_REG_GAMMA  ,
    DISP_REG_DITHER ,
    DISP_REG_UFOE   ,
    DISP_REG_PWM    ,
    DISP_REG_WDMA1  ,
    DISP_REG_MUTEX  ,
    DISP_REG_DSI0   ,
    DISP_REG_DPI0   ,
    DISP_REG_DPI1   ,
    DISP_REG_CONFIG ,
    DISP_REG_SMI_LARB0 ,
    DISP_REG_SMI_COMMON,
    DISP_REG_MIPI   ,
    DISP_REG_CONFIG2 ,
    DISP_REG_CONFIG3 ,
    DISP_REG_IO_DRIVING,
    DISP_TVDPLL_CFG6,
    DISP_TVDPLL_CON0,
    DISP_TVDPLL_CON1,
    DISP_REG_NUM
} DISP_REG_ENUM;

typedef enum {
    SOF_SINGLE = 0,
    SOF_DSI0,
    SOF_DSI1,
    SOF_DPI0,
} MUTEX_SOF;  

enum OVL_LAYER_SOURCE {
    OVL_LAYER_SOURCE_MEM    = 0,
    OVL_LAYER_SOURCE_RESERVED = 1,
    OVL_LAYER_SOURCE_SCL     = 2,
    OVL_LAYER_SOURCE_PQ     = 3,
};

enum OVL_LAYER_SECURE_MODE {
    OVL_LAYER_NORMAL_BUFFER    = 0,
    OVL_LAYER_SECURE_BUFFER    = 1,
    OVL_LAYER_PROTECTED_BUFFER = 2
};

typedef enum
{
    CMDQ_DISABLE = 0,
    CMDQ_ENABLE
}CMDQ_SWITCH;

typedef enum
{
    CMDQ_BEFORE_STREAM_SOF,
    CMDQ_WAIT_STREAM_EOF_EVENT,
    CMDQ_CHECK_IDLE_AFTER_STREAM_EOF,
    CMDQ_AFTER_STREAM_EOF,
    CMDQ_ESD_CHECK_READ,
    CMDQ_ESD_CHECK_CMP,    
    CMDQ_ESD_ALLC_SLOT,
    CMDQ_ESD_FREE_SLOT,
    CMDQ_STOP_VDO_MODE,
    CMDQ_START_VDO_MODE,
    CMDQ_DSI_RESET
}CMDQ_STATE;



#endif