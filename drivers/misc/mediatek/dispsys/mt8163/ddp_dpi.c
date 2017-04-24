#ifdef BUILD_UBOOT
//#define ENABLE_DSI_INTERRUPT 0

#include <asm/arch/disp_drv_platform.h>
#else
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/mutex.h>

#include "cmdq_record.h"
#include <disp_drv_log.h>
#endif

#include "mach/mt_typedefs.h"
#include <mach/sync_write.h>
#include <linux/clk.h>
#include <mach/irqs.h>

#include <linux/xlog.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#include "mtkfb.h"
#include "ddp_drv.h"
#include "ddp_hal.h"
#include "ddp_info.h"
#include "ddp_manager.h"
#include "ddp_dpi_reg.h"
#include "ddp_dpi.h"
#include "ddp_reg.h"
#include "ddp_log.h"
#include "ddp_irq.h"
#include "disp_drv_platform.h"

#include <debug.h>

#undef  LOG_TAG
#define LOG_TAG "DPI" 
#define ENABLE_DPI_INTERRUPT        0
#define DPI_INTERFACE_NUM           2
//#define DPI_IDX(module)             ((module==DISP_MODULE_DPI0)?0:1)
#define DPI_IDX(module)             0


#define K2_SMT

#undef LCD_BASE
#define LCD_BASE (0xF4024000)
#define DPI_REG_OFFSET(r)       offsetof(DPI_REGS, r)
#define REG_ADDR(base, offset)  (((BYTE *)(base)) + (offset))
#define msleep(x)    mdelay(x)

#define DSI_INREG32(type,addr) INREG32(addr)

//#ifdef INREG32
//#undef INREG32
//#define INREG32(x)          (__raw_readl((unsigned long*)(x)))
//#endif

#if 0
static int dpi_reg_op_debug = 0;

#define DPI_OUTREG32(cmdq, addr, val) \
  	{\
  		if(dpi_reg_op_debug) \
			printk("[dsi/reg]0x%08x=0x%08x, cmdq:0x%08x\n", addr, val, cmdq);\
		if(cmdq) \
			cmdqRecWrite(cmdq, (unsigned int)(addr)&0x1fffffff, val, ~0); \
		else \
			mt65xx_reg_sync_writel(val, addr);}
#else
#define DPI_OUTREG32(cmdq, addr, val) mt_reg_sync_writel(val, addr)
#endif

/*#undef DISP_LOG_PRINT
#define DISP_LOG_PRINT(level, sub_module, fmt, arg...)  \
    do {                                                    \
        xlog_printk(level, "DISP/"sub_module, fmt, ##arg);  \
    }while(0)*/

#define DSI_MODULE_BEGIN(x)	(x == DISP_MODULE_DSIDUAL?0:DSI_MODULE_to_ID(x))
#define DSI_MODULE_END(x)		(x == DISP_MODULE_DSIDUAL?1:DSI_MODULE_to_ID(x))
#define DSI_MODULE_to_ID(x)	(x == DISP_MODULE_DSI0?0:1)

PDSI_PHY_REGS DSI_PHY_REG_DPI[2];

PLVDS_TX_REGS LVDS_TX_REG = 0;
PLVDS_ANA_REGS LVDS_ANA_REG = 0;
//PDPI_REGS DPI_REG = 0;

static BOOL s_isDpiPowerOn = FALSE;
static BOOL s_isDpiStart   = FALSE;
static BOOL s_isDpiConfig  = FALSE;
static int dpi_vsync_irq_count[DPI_INTERFACE_NUM];
static int dpi_undflow_irq_count[DPI_INTERFACE_NUM];
static DPI_REGS regBackup;
static PDPI_REGS DPI_REG[2];


static LCM_UTIL_FUNCS lcm_utils_dpi;

const UINT32 BACKUP_DPI_REG_OFFSETS[] =
{
    DPI_REG_OFFSET(INT_ENABLE),
    DPI_REG_OFFSET(CNTL),
    DPI_REG_OFFSET(SIZE),    

    DPI_REG_OFFSET(TGEN_HWIDTH),
    DPI_REG_OFFSET(TGEN_HPORCH),
    DPI_REG_OFFSET(TGEN_VWIDTH_LODD),
    DPI_REG_OFFSET(TGEN_VPORCH_LODD),

    DPI_REG_OFFSET(BG_HCNTL),  
    DPI_REG_OFFSET(BG_VCNTL),
    DPI_REG_OFFSET(BG_COLOR),

    DPI_REG_OFFSET(TGEN_VWIDTH_LEVEN),
    DPI_REG_OFFSET(TGEN_VPORCH_LEVEN),
    DPI_REG_OFFSET(TGEN_VWIDTH_RODD),

    DPI_REG_OFFSET(TGEN_VPORCH_RODD),
    DPI_REG_OFFSET(TGEN_VWIDTH_REVEN),

    DPI_REG_OFFSET(TGEN_VPORCH_REVEN),
    DPI_REG_OFFSET(ESAV_VTIM_LOAD),
    DPI_REG_OFFSET(ESAV_VTIM_ROAD),
    DPI_REG_OFFSET(ESAV_FTIM),
};

/*the static functions declare*/
static void lcm_udelay(UINT32 us)
{
	udelay(us);
}

static void lcm_mdelay(UINT32 ms)
{
	msleep(ms);
}

static void lcm_set_reset_pin(UINT32 value)
{
#ifndef K2_SMT
	DPI_OUTREG32(0, MMSYS_CONFIG_BASE+0x150, value);
#endif
}

static void lcm_send_cmd(UINT32 cmd)
{
#ifndef K2_SMT
	DPI_OUTREG32(0, LCD_BASE+0x0F80, cmd);
#endif
}

static void lcm_send_data(UINT32 data)
{
#ifndef K2_SMT
	DPI_OUTREG32(0, LCD_BASE+0x0F90, data);
#endif
}

static void _BackupDPIRegisters(DISP_MODULE_ENUM module)
{
    UINT32 i;
    DPI_REGS *reg = &regBackup;

    for (i = 0; i < ARY_SIZE(BACKUP_DPI_REG_OFFSETS); ++i)
    {
        DPI_OUTREG32(0, REG_ADDR(reg, BACKUP_DPI_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(DPI_REG[DPI_IDX(module)], BACKUP_DPI_REG_OFFSETS[i])));
    }
}

static void _RestoreDPIRegisters(DISP_MODULE_ENUM module)
{
    UINT32 i;
    DPI_REGS *reg = &regBackup;

    for (i = 0; i < ARY_SIZE(BACKUP_DPI_REG_OFFSETS); ++i)
    {
        DPI_OUTREG32(0, REG_ADDR(DPI_REG[DPI_IDX(module)], BACKUP_DPI_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(reg, BACKUP_DPI_REG_OFFSETS[i])));
    }
}

/*the fuctions declare*/
/*DPI clock setting - use TVDPLL provide DPI clock for HDMI*/
DPI_STATUS ddp_dpi_ConfigPclk(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, unsigned int clk_req, DPI_POLARITY polarity)
{
    UINT32 dpickpol = 1, dpickoutdiv = 1, dpickdut = 1;
    UINT32 pcw = 0, postdiv = 0;
    DPI_REG_OUTPUT_SETTING ctrl = DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING;
	DPI_REG_CLKCNTL clkcon = DPI_REG[DPI_IDX(module)]->DPI_CLKCON;

    switch(clk_req)
    {
        case DPI_VIDEO_720x480p_60Hz:
		case DPI_VIDEO_720x576p_50Hz:
        {
            pcw = 0xc7627;
			postdiv = 1;
            break;
        }
		
		case DPI_VIDEO_1920x1080p_30Hz:
		case DPI_VIDEO_1280x720p_50Hz:
		case DPI_VIDEO_1920x1080i_50Hz:
		case DPI_VIDEO_1920x1080p_25Hz:
		case DPI_VIDEO_1920x1080p_24Hz:
		case DPI_VIDEO_1920x1080p_50Hz:
		case DPI_VIDEO_1280x720p3d_50Hz:
		case DPI_VIDEO_1920x1080i3d_50Hz:
		case DPI_VIDEO_1920x1080p3d_24Hz:	
        {
			pcw = 0x112276;
			postdiv = 0;
            break;
        }
		
		case DPI_VIDEO_1280x720p_60Hz:
		case DPI_VIDEO_1920x1080i_60Hz:
		case DPI_VIDEO_1920x1080p_23Hz:
		case DPI_VIDEO_1920x1080p_29Hz:
		case DPI_VIDEO_1920x1080p_60Hz:
		case DPI_VIDEO_1280x720p3d_60Hz:
		case DPI_VIDEO_1920x1080i3d_60Hz:
		case DPI_VIDEO_1920x1080p3d_23Hz:			
        {
			pcw = 0x111e08;
			postdiv = 0;
            break;
        }

        default:
        {
            DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "unknown clock frequency: %d \n", clk_req);
            break;
        }
    }
	
	switch(clk_req)
	{
		case DPI_VIDEO_720x480p_60Hz:
		case DPI_VIDEO_720x576p_50Hz:
		case DPI_VIDEO_1920x1080p3d_24Hz:
		case DPI_VIDEO_1280x720p_60Hz:
		{
			dpickpol = 0;
			dpickdut = 0;
			break;
		}
		
		case DPI_VIDEO_1920x1080p_30Hz:
		case DPI_VIDEO_1280x720p_50Hz:
		case DPI_VIDEO_1920x1080i_50Hz:
		case DPI_VIDEO_1920x1080p_25Hz:
		case DPI_VIDEO_1920x1080p_24Hz:
		case DPI_VIDEO_1920x1080p_50Hz:
		case DPI_VIDEO_1280x720p3d_50Hz:
		case DPI_VIDEO_1920x1080i3d_50Hz:		
		case DPI_VIDEO_1920x1080i_60Hz:
		case DPI_VIDEO_1920x1080p_23Hz:
		case DPI_VIDEO_1920x1080p_29Hz:
		case DPI_VIDEO_1920x1080p_60Hz:
		case DPI_VIDEO_1280x720p3d_60Hz:
		case DPI_VIDEO_1920x1080i3d_60Hz:
		case DPI_VIDEO_1920x1080p3d_23Hz:			
		{
            break;
        }

        default:
        {
            DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "unknown clock frequency: %d \n", clk_req);
            break;
        }
    }

    //DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "TVDPLL clock setting clk %d, clksrc: %d\n", clk_req,  clksrc);

#ifndef CONFIG_FPGA_EARLY_PORTING    //FOR BRING_UP
    DPI_OUTREG32(cmdq, 0x10209000 + 0x270, (postdiv << 4)|(0x01 << 0)); // TVDPLL enable
    DPI_OUTREG32(cmdq, 0x10209000 + 0x274, pcw|(1 << 31));     // set TVDPLL output clock frequency     
#endif

    /*IO driving setting*/
    MASKREG32(DISPSYS_IO_DRIVING, 0x3C00, 0x0); // 0x1400 for 8mA, 0x0 for 4mA

    /*DPI output clock polarity*/
    ctrl.CLK_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
    DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING, AS_UINT32(&ctrl));

	clkcon.DPI_CKOUT_DIV = dpickoutdiv;
	clkcon.DPI_CK_POL = dpickpol;
	clkcon.DPI_CK_DUT = dpickdut;
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->DPI_CLKCON , AS_UINT32(&clkcon));

    return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigDE(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, DPI_POLARITY polarity)
{
    DPI_REG_OUTPUT_SETTING pol = DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING;    
    
    pol.DE_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
    DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING, AS_UINT32(&pol));    
    
    return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigVsync(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, DPI_POLARITY polarity, UINT32 pulseWidth, UINT32 backPorch, UINT32 frontPorch)
{
    DPI_REG_TGEN_VWIDTH_LODD vwidth_lodd  = DPI_REG[DPI_IDX(module)]->TGEN_VWIDTH_LODD;
    DPI_REG_TGEN_VPORCH_LODD vporch_lodd  = DPI_REG[DPI_IDX(module)]->TGEN_VPORCH_LODD;
    DPI_REG_OUTPUT_SETTING pol = DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING;
    DPI_REG_CNTL VS = DPI_REG[DPI_IDX(module)]->CNTL;
	
    pol.VSYNC_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
    vwidth_lodd.VPW_LODD = pulseWidth;
    vporch_lodd.VBP_LODD= backPorch;
    vporch_lodd.VFP_LODD= frontPorch;

    VS.VS_LODD_EN = 1;
    VS.VS_LEVEN_EN = 0;
    VS.VS_RODD_EN = 0;
    VS.VS_REVEN_EN = 0;

    DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING, AS_UINT32(&pol));
    DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_VWIDTH_LODD, AS_UINT32(&vwidth_lodd));
    DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_VPORCH_LODD, AS_UINT32(&vporch_lodd));
    DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->CNTL, AS_UINT32(&VS));

    return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigHsync(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, DPI_POLARITY polarity, UINT32 pulseWidth, UINT32 backPorch, UINT32 frontPorch)
{
    DPI_REG_TGEN_HPORCH hporch = DPI_REG[DPI_IDX(module)]->TGEN_HPORCH;
    DPI_REG_OUTPUT_SETTING pol = DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING;
        
    hporch.HBP = backPorch;
    hporch.HFP = frontPorch;
    pol.HSYNC_POL = (DPI_POLARITY_FALLING == polarity) ? 1 : 0;
    DPI_REG[DPI_IDX(module)]->TGEN_HWIDTH = pulseWidth;
    
    DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_HWIDTH,pulseWidth);    
    DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_HPORCH, AS_UINT32(&hporch));
    DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING, AS_UINT32(&pol));

    return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigDualEdge(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, bool enable, UINT32 mode)
{
    DPI_OUTREGBIT(cmdq, DPI_REG_OUTPUT_SETTING, DPI_REG[DPI_IDX(module)]->OUTPUT_SETTING, DUAL_EDGE_SEL, enable);
    
#ifndef CONFIG_FPGA_EARLY_PORTING
    DPI_OUTREGBIT(cmdq, DPI_REG_DDR_SETTING, DPI_REG[DPI_IDX(module)]->DDR_SETTING, DDR_4PHASE, 1);
    DPI_OUTREGBIT(cmdq, DPI_REG_DDR_SETTING, DPI_REG[DPI_IDX(module)]->DDR_SETTING, DDR_EN, 1);
#endif

    return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigBG(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, bool enable, int BG_W, int BG_H)
{        
    if(enable == false)
    {
        DPI_OUTREGBIT(cmdq, DPI_REG_CNTL, DPI_REG[DPI_IDX(module)]->CNTL, BG_EN, 0);
    }
    else if(BG_W || BG_H)
    {
        DPI_OUTREGBIT(cmdq, DPI_REG_CNTL, DPI_REG[DPI_IDX(module)]->CNTL, BG_EN, 1);
        DPI_OUTREGBIT(cmdq, DPI_REG_BG_HCNTL, DPI_REG[DPI_IDX(module)]->BG_HCNTL, BG_RIGHT, BG_W/4);
        DPI_OUTREGBIT(cmdq, DPI_REG_BG_HCNTL, DPI_REG[DPI_IDX(module)]->BG_HCNTL, BG_LEFT, BG_W - BG_W/4);
        DPI_OUTREGBIT(cmdq, DPI_REG_BG_VCNTL, DPI_REG[DPI_IDX(module)]->BG_VCNTL, BG_BOT, BG_H/4);
        DPI_OUTREGBIT(cmdq, DPI_REG_BG_VCNTL, DPI_REG[DPI_IDX(module)]->BG_VCNTL, BG_TOP, BG_H - BG_H/4);
        DPI_OUTREGBIT(cmdq, DPI_REG_CNTL, DPI_REG[DPI_IDX(module)]->CNTL, BG_EN, 1);
        DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->BG_COLOR, 0);
    }

    return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigSize(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 width, UINT32 height)
{
    DPI_REG_SIZE size = DPI_REG[DPI_IDX(module)]->SIZE;
    size.WIDTH  = width;
    size.HEIGHT = height;
    DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->SIZE, AS_UINT32(&size));

    return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_EnableColorBar(DISP_MODULE_ENUM module)
{
    /*enable internal pattern - color bar*/
    if (module == DISP_MODULE_DPI0)
        DPI_OUTREG32(0, DISPSYS_DPI0_BASE + 0xF00, 0x41);
    else
        DPI_OUTREG32(0, DISPSYS_DPI1_BASE + 0xF00, 0x41);

    return DPI_STATUS_OK;
}

int ddp_dpi_power_on(DISP_MODULE_ENUM module, void *cmdq_handle)
{
    int ret = 0;
    DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_power_on, s_isDpiPowerOn %d\n", s_isDpiPowerOn);
    if (!s_isDpiPowerOn)
    {
#if 0 //ndef DIABLE_CLOCK_API
        ret += clk_prepare(ddp_clk_map[MM_CLK_DPI_PIXEL]);
		ret += clk_enable(ddp_clk_map[MM_CLK_DPI_PIXEL]);
        ret += clk_prepare(ddp_clk_map[MM_CLK_DPI_ENGINE]);
		ret += clk_enable(ddp_clk_map[MM_CLK_DPI_ENGINE]);
#endif
        if(ret > 0)
        {
            DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "power manager API return FALSE\n");
        }     
        //_RestoreDPIRegisters();
        s_isDpiPowerOn = TRUE;
    }

    return 0;
}

int ddp_dpi_power_off(DISP_MODULE_ENUM module, void *cmdq_handle)
{
    int ret = 0;
    DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_power_off, s_isDpiPowerOn %d\n", s_isDpiPowerOn);
    if (s_isDpiPowerOn)
    {   
#if 0 //ndef DIABLE_CLOCK_API
        _BackupDPIRegisters();
        ret += clk_disable(ddp_clk_map[MM_CLK_DPI_PIXEL]);
		clk_unprepare(ddp_clk_map[MM_CLK_DPI_PIXEL]);
       	ret += clk_disable(ddp_clk_map[MM_CLK_DPI_ENGINE]);
		clk_unprepare(ddp_clk_map[MM_CLK_DPI_ENGINE]);
#endif
        if(ret >0)
        {
            DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "power manager API return FALSE\n");
        }       
        s_isDpiPowerOn = FALSE;
    }

    return 0;

}

DPI_STATUS ddp_dpi_yuv422_setting(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 uvsw)
{	
    DPI_REG_YUV422_SETTING uvset = DPI_REG[DPI_IDX(module)]->YUV422_SETTING;    
    
    uvset.UV_SWAP = uvsw;
    DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->YUV422_SETTING, AS_UINT32(&uvset));    

    return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_CLPFSetting(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT8 clpfType, BOOL roundingEnable)
{    
	DPI_REG_CLPF_SETTING setting = DPI_REG[DPI_IDX(module)]->CLPF_SETTING;
	
	setting.CLPF_TYPE = clpfType;
	setting.ROUND_EN = roundingEnable ? 1 : 0;
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->CLPF_SETTING, AS_UINT32(&setting));

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigHDMI(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 yuv422en, UINT32 rgb2yuven, UINT32 ydfpen, UINT32 r601sel, UINT32 clpfen)
{	
	DPI_REG_CNTL ctrl = DPI_REG[DPI_IDX(module)]->CNTL;
	
	ctrl.YUV422_EN = yuv422en;
	ctrl.RGB2YUV_EN = rgb2yuven;
	ctrl.TDFP_EN = ydfpen;
	ctrl.R601_SEL = r601sel;
	ctrl.CLPF_EN = clpfen;

	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->CNTL, AS_UINT32(&ctrl));

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigVsync_LEVEN(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 pulseWidth, UINT32 backPorch, UINT32 frontPorch, BOOL fgInterlace)
{	
	DPI_REG_TGEN_VWIDTH_LEVEN vwidth_leven  = DPI_REG[DPI_IDX(module)]->TGEN_VWIDTH_LEVEN;
	DPI_REG_TGEN_VPORCH_LEVEN vporch_leven  = DPI_REG[DPI_IDX(module)]->TGEN_VPORCH_LEVEN;

	vwidth_leven.VPW_LEVEN = pulseWidth;
	vwidth_leven.VPW_HALF_LEVEN = fgInterlace;
	vporch_leven.VBP_LEVEN = backPorch;	//vporch_leven.VFP_HALF_LEVEN = fgInterlace;
	vporch_leven.VFP_LEVEN = frontPorch;

	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_VWIDTH_LEVEN, AS_UINT32(&vwidth_leven));
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_VPORCH_LEVEN, AS_UINT32(&vporch_leven));

	return DPI_STATUS_OK;
}

DPI_STATUS ddp_dpi_ConfigVsync_RODD(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, UINT32 pulseWidth, UINT32 backPorch, UINT32 frontPorch)
{	
	DPI_REG_TGEN_VWIDTH_RODD vwidth_rodd  = DPI_REG[DPI_IDX(module)]->TGEN_VWIDTH_RODD;
	DPI_REG_TGEN_VPORCH_RODD vporch_rodd  = DPI_REG[DPI_IDX(module)]->TGEN_VPORCH_RODD;

	vwidth_rodd.VPW_RODD = pulseWidth;
	vporch_rodd.VBP_RODD = backPorch;
	vporch_rodd.VFP_RODD = frontPorch;

	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_VWIDTH_RODD, AS_UINT32(&vwidth_rodd));
	DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->TGEN_VPORCH_RODD, AS_UINT32(&vporch_rodd));

	return DPI_STATUS_OK;
}

int ddp_dpi_config(DISP_MODULE_ENUM module, disp_ddp_path_config *config, void *cmdq_handle)
{
    if(s_isDpiConfig == FALSE)
    {
        LCM_DPI_PARAMS *dpi_config = &(config->dispif_config.dpi);
        DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_config DPI status:%x, cmdq:%x\n", INREG32(&DPI_REG[DPI_IDX(module)]->STATUS), (long unsigned int)cmdq_handle);
 		if (module == DISP_MODULE_DPI1)
			ddp_dpi_ConfigPclk(module, cmdq_handle, dpi_config->dpi_clock,
					   dpi_config->clk_pol);
		else if (module == DISP_MODULE_DPI0)
			ddp_dpi_lvds_config(module, dpi_config, cmdq_handle);
		else {
			DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "unknown clock interface: %d\n",
				       module);
			return 0;
		}
        ddp_dpi_ConfigSize(module, cmdq_handle, dpi_config->width, dpi_config->height);
        ddp_dpi_ConfigBG(module, cmdq_handle, true, dpi_config->bg_width, dpi_config->bg_height);
        ddp_dpi_ConfigDE(module, cmdq_handle, dpi_config->de_pol);
        ddp_dpi_ConfigVsync(module, cmdq_handle, dpi_config->vsync_pol, dpi_config->vsync_pulse_width,
                        dpi_config->vsync_back_porch, dpi_config->vsync_front_porch );
        ddp_dpi_ConfigHsync(module, cmdq_handle, dpi_config->hsync_pol, dpi_config->hsync_pulse_width,
                        dpi_config->hsync_back_porch, dpi_config->hsync_front_porch );    

        ddp_dpi_ConfigDualEdge(module, cmdq_handle, dpi_config->i2x_en, dpi_config->i2x_edge);

        s_isDpiConfig = TRUE;
        DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_config done\n");
    }

	return 0;
}

int ddp_dpi_reset( DISP_MODULE_ENUM module, void *cmdq_handle)
{
    DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_reset\n");
	
    DPI_OUTREGBIT(NULL, DPI_REG_RST, DPI_REG[DPI_IDX(module)]->DPI_RST, RST, 1);
    DPI_OUTREGBIT(NULL, DPI_REG_RST, DPI_REG[DPI_IDX(module)]->DPI_RST, RST, 0);

    return 0;
}

int ddp_dpi_start(DISP_MODULE_ENUM module, void *cmdq)
{
	return 0;
}

int ddp_dpi_trigger(DISP_MODULE_ENUM module, void *cmdq)
{    
    if(s_isDpiStart == FALSE)
    {
        DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_start\n");
        ddp_dpi_reset(module, cmdq);
        /*enable DPI*/
        DPI_OUTREG32(cmdq, &DPI_REG[DPI_IDX(module)]->DPI_EN, 0x00000001);

        s_isDpiStart = TRUE;
    }
	return 0;
}

int ddp_dpi_stop(DISP_MODULE_ENUM module, void *cmdq_handle)
{
    DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_stop\n");

    /*disable DPI and background, and reset DPI*/	
    DPI_OUTREG32(cmdq_handle, &DPI_REG[DPI_IDX(module)]->DPI_EN, 0x00000000);
    ddp_dpi_ConfigBG(module, cmdq_handle, false, 0, 0);
    ddp_dpi_reset(module, cmdq_handle);

    s_isDpiStart  = FALSE;
    s_isDpiConfig = FALSE;
    dpi_vsync_irq_count[0]   = 0;
    dpi_vsync_irq_count[1]   = 0;
    dpi_undflow_irq_count[0] = 0;
    dpi_undflow_irq_count[1] = 0;

    return 0;
}
 
int ddp_dpi_is_busy(DISP_MODULE_ENUM module)
{
    unsigned int status = INREG32(&DPI_REG[DPI_IDX(module)]->STATUS);

    return (status & (0x1<<16) ? 1 : 0);
}

int ddp_dpi_is_idle(DISP_MODULE_ENUM module)
{
    return !ddp_dpi_is_busy(module);
}

unsigned int ddp_dpi_get_cur_addr(bool rdma_mode, int layerid )
{
    if(rdma_mode)
        return (INREG32(DISP_REG_RDMA_MEM_START_ADDR+DISP_INDEX_OFFSET*2));
    else
    {
        if(INREG32(DISP_INDEX_OFFSET+DISP_REG_OVL_RDMA0_CTRL+layerid* 0x20 ) & 0x1)
            return (INREG32(DISP_INDEX_OFFSET+DISP_REG_OVL_L0_ADDR+layerid * 0x20));
        else
            return 0;
    }
}


#if ENABLE_DPI_INTERRUPT
static irqreturn_t _DPI_InterruptHandler(DISP_MODULE_ENUM module, unsigned int param)
{
    static int counter = 0;
    DPI_REG_INTERRUPT status = DPI_REG[DPI_IDX(module)]->INT_STATUS;
    
    if(status.VSYNC)
    {
        dpi_vsync_irq_count[DPI_IDX(module)]++;
        if(dpi_vsync_irq_count[DPI_IDX(module)] > 30)
        {
            //printk("dpi vsync %d\n", dpi_vsync_irq_count[DPI_IDX(module)]);
            dpi_vsync_irq_count[DPI_IDX(module)] = 0;
        }

        if (counter)
        {
            DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "[Error] DPI FIFO is empty, "
               "received %d times interrupt !!!\n", counter);
            counter = 0;
        }
    }

    DPI_OUTREG32(0, &DPI_REG[DPI_IDX(module)]->INT_STATUS, 0);

    return IRQ_HANDLED;
}
#endif

int ddp_dpi_init(DISP_MODULE_ENUM module, void *cmdq)
{
    UINT32 i;

    DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_init- %x\n", (long unsigned int)cmdq);

#ifdef CONFIG_FPGA_EARLY_PORTING
	DPI_OUTREG32(0, DISPSYS_DPI0_BASE, 0x1);
	DPI_OUTREG32(0, DISPSYS_DPI0_BASE + 0xE0, 0x404);
#endif

    DPI_REG[0] = (PDPI_REGS) (DISPSYS_DPI0_BASE);
    DPI_REG[1] = (PDPI_REGS) (DISPSYS_DPI1_BASE);

    ddp_dpi_power_on(DISP_MODULE_DPI0, cmdq);
    ddp_dpi_power_on(DISP_MODULE_DPI1, cmdq);

    
#if ENABLE_DPI_INTERRUPT
    for (i = 0; i < DPI_INTERFACE_NUM; i++)
    {
        DPI_OUTREGBIT(cmdq, DPI_REG_INTERRUPT, DPI_REG[i]->INT_ENABLE, VSYNC, 1);
    }
    
    disp_register_module_irq_callback(DISP_MODULE_DPI0, _DPI_InterruptHandler);
    disp_register_module_irq_callback(DISP_MODULE_DPI1, _DPI_InterruptHandler);
    
#endif
    for (i = 0; i < DPI_INTERFACE_NUM; i++) 
    {
		DISPCHECK("dsi%d init finished\n", i);
	}

    return 0;
}

int ddp_dpi_deinit(DISP_MODULE_ENUM module, void *cmdq_handle)
{
    DISP_LOG_PRINT(ANDROID_LOG_WARN, "DPI", "ddp_dpi_deinit- %x\n", (long unsigned int)cmdq_handle);

	ddp_dpi_stop(module, cmdq_handle);
    ddp_dpi_power_off(module, cmdq_handle);

    return 0;
}

int ddp_dpi_set_lcm_utils(DISP_MODULE_ENUM module, LCM_DRIVER *lcm_drv)
{
    DISPFUNC();
    LCM_UTIL_FUNCS *utils = NULL;
		
    if(lcm_drv == NULL)
    {
        DISP_LOG_PRINT(ANDROID_LOG_ERROR, "DPI", "lcm_drv is null!\n");
        return -1;
    }

    utils = &lcm_utils_dpi;

    utils->set_reset_pin = lcm_set_reset_pin;
    utils->udelay        = lcm_udelay;
    utils->mdelay        = lcm_mdelay;
    utils->send_cmd      = lcm_send_cmd,
    utils->send_data     = lcm_send_data,

    lcm_drv->set_util_funcs(utils);

    return 0;
}

int ddp_dpi_build_cmdq(DISP_MODULE_ENUM module, void *cmdq_trigger_handle, CMDQ_STATE state)
{
    return 0;
}

int ddp_dpi_dump(DISP_MODULE_ENUM module, int level)
{
    UINT32 i;
    DDPDUMP("---------- Start dump DPI registers ----------\n");

    for (i = 0; i <= 0x40; i += 4)
    {
		DDPDUMP("DPI+%04x : 0x%08x\n", i, INREG32(DISPSYS_DPI0_BASE + i));
    }   
    	
    for (i = 0x68; i <= 0x7C; i += 4)
    {
        DDPDUMP("DPI+%04x : 0x%08x\n", i, INREG32(DISPSYS_DPI0_BASE + i));
    }

    DDPDUMP("DPI+Color Bar : %04x : 0x%08x\n", 0xF00, INREG32(DISPSYS_DPI0_BASE + 0xF00));
#if 0  	
    DDPDUMP("DPI Addr IO Driving : 0x%08x\n", INREG32(DISPSYS_IO_DRIVING));

    DDPDUMP("DPI TVDPLL CON0 : 0x%08x\n",  INREG32(DDP_REG_TVDPLL_CON0));
    DDPDUMP("DPI TVDPLL CON1 : 0x%08x\n",  INREG32(DDP_REG_TVDPLL_CON1));
    
    DDPDUMP("DPI TVDPLL CON6 : 0x%08x\n",  INREG32(DDP_REG_TVDPLL_CON6));
    DDPDUMP("DPI MMSYS_CG_CON1:0x%08x\n",  INREG32(DISP_REG_CONFIG_MMSYS_CG_CON1));
#endif

    return 0;
}

int ddp_dpi_ioctl(DISP_MODULE_ENUM module, void *cmdq_handle, unsigned int ioctl_cmd, unsigned long *params)
{
    DISPFUNC();
    
    int ret = 0;
    DDP_IOCTL_NAME ioctl = (DDP_IOCTL_NAME)ioctl_cmd;
    DISP_LOG_PRINT(ANDROID_LOG_DEBUG, "DPI", "DPI ioctl: %d \n", ioctl);

    switch(ioctl)
    {
        case DDP_DPI_FACTORY_TEST:
        {
            disp_ddp_path_config *config_info = (disp_ddp_path_config *)params;
	
            ddp_dpi_power_on(module, NULL);
            ddp_dpi_stop(module, NULL);
            ddp_dpi_config(module, config_info, NULL);
            ddp_dpi_EnableColorBar(module);
	
            ddp_dpi_trigger(module, NULL);
            ddp_dpi_start(module, NULL);
            ddp_dpi_dump(module, 1);
            break;
        }
        default:
            break;
    }
    
    return ret;
}

static int ddp_dpi_clock_on(DISP_MODULE_ENUM module,void * handle)
{
    //ddp_enable_module_clock(module);
    return 0;
}

static int ddp_dpi_clock_off(DISP_MODULE_ENUM module,void * handle)
{
    //ddp_disable_module_clock(module);
    return 0;
}
DDP_MODULE_DRIVER ddp_driver_dpi0 = 
{
    .module        = DISP_MODULE_DPI0,
    #if HDMI_MAIN_PATH_LK
    
    .init          = ddp_dpi_clock_on,
    .deinit        = ddp_dpi_clock_off,
    #else
    .init          = ddp_dpi_init,
    .deinit        = ddp_dpi_deinit,
    .config        = ddp_dpi_config,
    .build_cmdq    = ddp_dpi_build_cmdq,
    .trigger       = ddp_dpi_trigger,
   	.start         = ddp_dpi_start,
   	.stop          = ddp_dpi_stop,
   	.reset         = ddp_dpi_reset,
   	.power_on      = ddp_dpi_power_on,
   	.power_off     = ddp_dpi_power_off,
   	.is_idle       = ddp_dpi_is_idle,
   	.is_busy       = ddp_dpi_is_busy,
   	.dump_info     = ddp_dpi_dump,
   	.set_lcm_utils = ddp_dpi_set_lcm_utils,
   	//.ioctl         = ddp_dpi_ioctl
   	#endif
};

DDP_MODULE_DRIVER ddp_driver_dpi1 = 
{
    .module        = DISP_MODULE_DPI1,
    .init          = ddp_dpi_init,
    .deinit        = ddp_dpi_deinit,
    .config        = ddp_dpi_config,
    .build_cmdq    = ddp_dpi_build_cmdq,
    .trigger       = ddp_dpi_trigger,
   	.start         = ddp_dpi_start,
   	.stop          = ddp_dpi_stop,
   	.reset         = ddp_dpi_reset,
   	.power_on      = ddp_dpi_power_on,
   	.power_off     = ddp_dpi_power_off,
   	.is_idle       = ddp_dpi_is_idle,
   	.is_busy       = ddp_dpi_is_busy,
   	.dump_info     = ddp_dpi_dump,
   	.set_lcm_utils = ddp_dpi_set_lcm_utils,
   	.ioctl         = ddp_dpi_ioctl
};

void LVDS_Clk_Path_Config(void)
{
	/* top_clk setting */
	/* CLK_CFG_6[2:0]=001; 0x100000a0, bit0~bit2 :1 */
	MASKREG32(DISPSYS_CONFIG3_BASE + 0xa0, (0x1 ), (0x1));	/* bit0 */
	MASKREG32(DISPSYS_CONFIG3_BASE + 0xa0, (0x01<<1 ), (0x00<<1));	/* bit1 */
	MASKREG32(DISPSYS_CONFIG3_BASE + 0xa0, (0x01<<2 ), (0x00<<2));	/* bit2 */

	/* lvds_sys_cfg_clk setting 0x1400090c[0]; 0->LVDS,1 DPI */
	MASKREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x90c, (0x01), (0x00));	/* bit0 */

	/* lvds_cts_clk setting 0x14000118[8]*/
	MASKREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x118, (0x01 << 8), (0x01 << 8));	/* bit8 */

	/* dpi0 pixel/engine clock setting 0x14000118[6] */
	MASKREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x118, (0x01 << 6), (0x01 << 6));	/* bit6 */

	/* lvds_pxl_clk setting 0x14000118[7]*/
	MASKREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x118, (0x01 << 7), (0x01 << 7));	/* bit7 */
}

void LVDS_ANA_Init(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
	//if(lcm_params->dpi.lvds_tx_en == 1)

	DPI_OUTREGBIT(cmdq, LVDS_VPLL_REG_CTL2, LVDS_ANA_REG->LVDS_VPLL_CTL2, LVDS_VPLL_CLK_SEL, 0x01); /*From mipi pLL*/
	DPI_OUTREGBIT(cmdq, LVDS_VPLL_REG_CTL2, LVDS_ANA_REG->LVDS_VPLL_CTL2, LVDS_VPLL_EN, 0x01);  /*Enable LVDS VPLL*/
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL3, LVDS_ANA_REG->LVDSTX_ANA_CTL3, LVDSTX_ANA_EXT_EN, 0x1f);  
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL3, LVDS_ANA_REG->LVDSTX_ANA_CTL3, LVDSTX_ANA_DRV_EN, 0x1f); 
	
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL2, LVDS_ANA_REG->LVDSTX_ANA_CTL2, LVDSTX_ANA_TVO, 0x03);  
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL2, LVDS_ANA_REG->LVDSTX_ANA_CTL2, LVDSTX_ANA_TVCM, 0x0b);
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL2, LVDS_ANA_REG->LVDSTX_ANA_CTL2, LVDSTX_ANA_NSRC, 0x0f); 
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL2, LVDS_ANA_REG->LVDSTX_ANA_CTL2, LVDSTX_ANA_PSRC, 0x00); 
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL2, LVDS_ANA_REG->LVDSTX_ANA_CTL2, LVDSTX_ANA_BIAS_SEL, 0x01);
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL2, LVDS_ANA_REG->LVDSTX_ANA_CTL2, LVDSTX_ANA_R_TERM, 0x00); 
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL2, LVDS_ANA_REG->LVDSTX_ANA_CTL2, LVDSTX_ANA_SEL_CKTST, 0x00);
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL2, LVDS_ANA_REG->LVDSTX_ANA_CTL2, LVDSTX_ANA_SEL_MERGE, 0x00); 
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL2, LVDS_ANA_REG->LVDSTX_ANA_CTL2, LVDSTX_ANA_LDO_EN, 0x01);
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL2, LVDS_ANA_REG->LVDSTX_ANA_CTL2, LVDSTX_ANA_BIAS_EN, 0x01); 
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL2, LVDS_ANA_REG->LVDSTX_ANA_CTL2, LVDSTX_ANA_SER_ABIST_EN, 0x00); 
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL2, LVDS_ANA_REG->LVDSTX_ANA_CTL2, LVDSTX_ANA_SER_ABEDG_EN, 0x00);
	DPI_OUTREGBIT(cmdq, LVDS_ANA_REG_CTL2, LVDS_ANA_REG->LVDSTX_ANA_CTL2, LVDSTX_ANA_SER_BIST_TOG, 0x00); 
	
}
	void LVDS_DIG_Init(DISP_MODULE_ENUM module, cmdqRecHandle cmdq)
{
	DPI_OUTREGBIT(cmdq, LVDS_REG_CLK_CTRL, LVDS_TX_REG->LVDS_CLK_CTRL, TX_CK_EN, 0x01); 
	DPI_OUTREGBIT(cmdq, LVDS_REG_CLK_CTRL, LVDS_TX_REG->LVDS_CLK_CTRL, RX_CK_EN, 0x01);  
	DPI_OUTREGBIT(cmdq, LVDS_REG_CLK_CTRL, LVDS_TX_REG->LVDS_CLK_CTRL, TEST_CK_EN, 0x01); 

	DPI_OUTREGBIT(cmdq, LVDS_REG_OUT_CTRL, LVDS_TX_REG->LVDS_OUT_CTRL, LVDS_EN, 0x01);
	//if(lcm_params->dpi.format	 == LCM_DPI_FORMAT_RGB666)
	DPI_OUTREGBIT(cmdq, LVDS_REG_FMTCNTL, LVDS_TX_REG->LVDS_FMTCTRL, DATA_FMT, 0x01);  /*6bit*/
	//else
	DPI_OUTREGBIT(cmdq, LVDS_REG_FMTCNTL, LVDS_TX_REG->LVDS_FMTCTRL, DATA_FMT, 0x00);  /*8bit*/

	 // MASKREG32(DISPSYS_BASE + 0x090c, 0x10000, 0x10000);  // enable LVDS out 
#if 1
	/* pattern enable for 800 x 1280 */
	DPI_OUTREGBIT(cmdq, LVDS_REG_RGTST_PAT, LVDS_TX_REG->LVDS_RGTST_PAT, RG_TST_PATN_EN, 0x01);
	DPI_OUTREGBIT(cmdq, LVDS_REG_RGTST_PAT, LVDS_TX_REG->LVDS_RGTST_PAT, RG_TST_PATN_TYPE, 0x02);
	DPI_OUTREGBIT(cmdq, LVDS_REG_RGPAT_EDGE, LVDS_TX_REG->LVDS_RGPAT_EDGE, RG_BD_R, 0xff);
	/* pattern width */
	DPI_OUTREGBIT(cmdq, LVDS_REG_PATN_TOTAL, LVDS_TX_REG->LVDS_PATN_TOTAL, RG_PTGEN_HTOTAL, 0x360);
	DPI_OUTREGBIT(cmdq, LVDS_REG_PATN_ACTIVE, LVDS_TX_REG->LVDS_PATN_ACTIVE, RG_PTGEN_HACTIVE, 0x320);
	/* pattern height */
	DPI_OUTREGBIT(cmdq, LVDS_REG_PATN_TOTAL, LVDS_TX_REG->LVDS_PATN_TOTAL, RG_PTGEN_VTOTAL, 0x508);
	DPI_OUTREGBIT(cmdq, LVDS_REG_PATN_ACTIVE, LVDS_TX_REG->LVDS_PATN_ACTIVE, RG_PTGEN_VACTIVE, 0x500);
	/* pattern start */
	DPI_OUTREGBIT(cmdq, LVDS_REG_PATN_START, LVDS_TX_REG->LVDS_PATN_START, RG_PTGEN_VSTART, 0x08);
	DPI_OUTREGBIT(cmdq, LVDS_REG_PATN_START, LVDS_TX_REG->LVDS_PATN_START, RG_PTGEN_HSTART, 0x20);
	/* pattern sync width */
	DPI_OUTREGBIT(cmdq, LVDS_REG_PATN_WIDTH, LVDS_TX_REG->LVDS_PATN_WIDTH, RG_PTGEN_VWIDTH, 0x01);
	DPI_OUTREGBIT(cmdq, LVDS_REG_PATN_WIDTH, LVDS_TX_REG->LVDS_PATN_WIDTH, RG_PTGEN_HWIDTH, 0x01);
#endif
}


void LVDS_MIPI_PLL_Init(DISP_MODULE_ENUM module, cmdqRecHandle cmdq, LCM_DPI_PARAMS *dpi_params)
{
	DISPFUNC();
	int i = 0;
	unsigned int data_Rate = dpi_params->PLL_CLOCK*2;
	unsigned int txdiv = 0;
	unsigned int txdiv0 = 0;
	unsigned int txdiv1 = 0;
	unsigned int pcw = 0;
	//unsigned int fmod = 30;//Fmod = 30KHz by default
	unsigned int delta1 = 5;//Delta1 is SSC range, default is 0%~-5%
	unsigned int pdelta1= 0;
	u32 m_hw_res3 = 0;
	u32 temp1 =0;
	u32 temp2 = 0;
	u32 temp3 = 0;
	u32 temp4 = 0;
	u32 temp5 = 0;
	u32 lnt = 0;
	
	// temp1~5 is used for impedence calibration, not enable now
#if 0
	m_hw_res3 = INREG32(0xF0206180);
	temp1 = (m_hw_res3 >> 28) & 0xF;
	temp2 = (m_hw_res3 >> 24) & 0xF;
	temp3 = (m_hw_res3 >> 20) & 0xF;
	temp4 = (m_hw_res3 >> 16) & 0xF;
	temp5 = (m_hw_res3 >> 12) & 0xF;
#endif
	
	for(i = DSI_MODULE_BEGIN(module);i <= DSI_MODULE_END(module);i++)
	{
		// step 0
		/*lnt = MIPITX_INREG32(0x10206190);
		MIPITX_OUTREGBIT(MIPITX_DSI_CLOCK_LANE_REG,DSI_PHY_REG[i]->MIPITX_DSI_CLOCK_LANE,RG_DSI_LNTC_RT_CODE,((lnt >> 0) & 0xf));
		MIPITX_OUTREGBIT(MIPITX_DSI_DATA_LANE3_REG,DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE3,RG_DSI_LNT3_RT_CODE,((lnt >> 4) & 0xf));
		MIPITX_OUTREGBIT(MIPITX_DSI_DATA_LANE2_REG,DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE2,RG_DSI_LNT2_RT_CODE,((lnt >> 8) & 0xf));
		MIPITX_OUTREGBIT(MIPITX_DSI_DATA_LANE1_REG,DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE1,RG_DSI_LNT1_RT_CODE,((lnt >> 12) & 0xf));
		MIPITX_OUTREGBIT(MIPITX_DSI_DATA_LANE0_REG,DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE0,RG_DSI_LNT0_RT_CODE,((lnt >> 16) & 0xf));
	
		DISPMSG("PLL config: LNT=0x%x,clk=0x%x,lan3=0x%x,lan2=0x%x,lan1=0x%x,lan0=0x%x\n",
		lnt,INREG32(&DSI_PHY_REG[i]->MIPITX_DSI_CLOCK_LANE), INREG32(&DSI_PHY_REG[i]->MIPITX_DSI_CLOCK_LANE),
		INREG32(&DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE3), INREG32(&DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE2), 
		INREG32(&DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE1), INREG32(&DSI_PHY_REG[i]->MIPITX_DSI_DATA_LANE0));*/
		// step 1
		//MIPITX_MASKREG32(APMIXED_BASE+0x00, (0x1<<6), 1);
	
		// step 2
		MIPITX_OUTREGBIT(MIPITX_DSI_BG_CON_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_BG_CON,RG_DSI_BG_CORE_EN,1);
		MIPITX_OUTREGBIT(MIPITX_DSI_BG_CON_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_BG_CON,RG_DSI_BG_CKEN,1); 	
	
		// step 3
		udelay(30);
	
		// step 4
		MIPITX_OUTREGBIT(MIPITX_DSI_TOP_CON_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_TOP_CON,RG_DSI_LNT_HS_BIAS_EN,1);
	
		// step 5
		MIPITX_OUTREGBIT(MIPITX_DSI_CON_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_CON,RG_DSI_CKG_LDOOUT_EN,1);
		MIPITX_OUTREGBIT(MIPITX_DSI_CON_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_CON,RG_DSI_LDOCORE_EN,1);
	
		// step 6
		MIPITX_OUTREGBIT(MIPITX_DSI_PLL_PWR_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_PWR,DA_DSI_MPPLL_SDM_PWR_ON,1);
	
		// step 7
		MIPITX_OUTREGBIT(MIPITX_DSI_PLL_PWR_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_PWR,DA_DSI_MPPLL_SDM_ISO_EN,0);
	
		if(0!=data_Rate)
		{
			if(data_Rate > 1250)			{	DISPCHECK("mipitx Data Rate exceed limitation(%d)\n", data_Rate);ASSERT(0);}
			else if(data_Rate >= 500)		{	txdiv = 1;	txdiv0 = 0; 	txdiv1 = 0; }
			else if(data_Rate >= 250)		{	txdiv = 2;	txdiv0 = 1; 	txdiv1 = 0; }
			else if(data_Rate >= 125)		{	txdiv = 4;	txdiv0 = 2; txdiv1 = 0; }
			else if(data_Rate > 62) 		{	txdiv = 8;	txdiv0 = 2; txdiv1 = 1; }
			else if(data_Rate >= 50)		{	txdiv = 16; txdiv0 = 2; txdiv1 = 2; }
			else							{	DISPCHECK("dataRate is too low(%d)\n", data_Rate);	ASSERT(0);}
	
			// step 8
			MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON0_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON0,RG_DSI0_MPPLL_TXDIV0, txdiv0);
			MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON0_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON0,RG_DSI0_MPPLL_TXDIV1, txdiv1);
			MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON0_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON0,RG_DSI0_MPPLL_PREDIV, 0);
	
			// step 9
			MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON1_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON1,RG_DSI0_MPPLL_SDM_FRA_EN,1);
	
			// step 10
			// PLL PCW config
			/*
			PCW bit 24~30 = floor(pcw)
			PCW bit 16~23 = (pcw - floor(pcw))*256
			PCW bit 8~15 = (pcw*256 - floor(pcw)*256)*256
			PCW bit 8~15 = (pcw*256*256 - floor(pcw)*256*256)*256
			*/
			//	pcw = data_Rate*4*txdiv/(26*2);//Post DIV =4, so need data_Rate*4
			pcw = data_Rate*txdiv/13;
	
			MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON2_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON2,RG_DSI0_MPPLL_SDM_PCW_H,(pcw & 0x7F));
			MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON2_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON2,RG_DSI0_MPPLL_SDM_PCW_16_23,((256*(data_Rate*txdiv%13)/13) & 0xFF));
			MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON2_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON2,RG_DSI0_MPPLL_SDM_PCW_8_15,((256*(256*(data_Rate*txdiv%13)%13)/13) & 0xFF));
			MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON2_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON2,RG_DSI0_MPPLL_SDM_PCW_0_7,((256*(256*(256*(data_Rate*txdiv%13)%13)%13)/13) & 0xFF));
	
			if(1 != dpi_params->ssc_disable)
			{
				MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON1_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON1,RG_DSI0_MPPLL_SDM_SSC_PH_INIT,1);
				MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON1_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON1,RG_DSI0_MPPLL_SDM_SSC_PRD,0x1B1);//PRD=ROUND(pmod) = 433;
				if(0 != dpi_params->ssc_range)
				{
					delta1 = dpi_params->ssc_range;
				}
				ASSERT(delta1<=8);
				pdelta1 = (delta1*data_Rate*txdiv*262144+281664)/563329;
				MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON3_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON3,RG_DSI0_MPPLL_SDM_SSC_DELTA,pdelta1);
				MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON3_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON3,RG_DSI0_MPPLL_SDM_SSC_DELTA1,pdelta1);
				//DSI_OUTREGBIT(MIPITX_DSI_PLL_CON1_REG,DSI_PHY_REG_DPI->MIPITX_DSI_PLL_CON1,RG_DSI0_MPPLL_SDM_FRA_EN,1);
				DISPMSG("[dpi_drv.c] DPI PLL config:data_rate=%d,txdiv=%d,pcw=%d,delta1=%d,pdelta1=0x%x\n",data_Rate,txdiv,DSI_INREG32(PMIPITX_DSI_PLL_CON2_REG,&DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON2),delta1,pdelta1);
			}
		}
		else
		{
			DISPERR("[dsi_dsi.c] PLL clock should not be 0!!!\n");
			ASSERT(0);
		}
			
		if((0 != data_Rate) && (1 != dpi_params->ssc_disable))
		{
			MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON1_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON1,RG_DSI0_MPPLL_SDM_SSC_EN,1);
		}
		else
		{
			MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON1_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON1,RG_DSI0_MPPLL_SDM_SSC_EN,0);
		}
			
		// step 11
		MIPITX_OUTREGBIT(MIPITX_DSI_CLOCK_LANE_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_CLOCK_LANE,RG_DSI_LNTC_LDOOUT_EN,1);
	
		// step 12
		/*if(dsi_params->LANE_NUM > 0)
		{
			MIPITX_OUTREGBIT(MIPITX_DSI_DATA_LANE0_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_DATA_LANE0,RG_DSI_LNT0_LDOOUT_EN,1);
		}
	
		// step 13
		if(dsi_params->LANE_NUM > 1)
		{
			MIPITX_OUTREGBIT(MIPITX_DSI_DATA_LANE1_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_DATA_LANE1,RG_DSI_LNT1_LDOOUT_EN,1);
		}
	
		// step 14
		if(dsi_params->LANE_NUM > 2)
		{
			MIPITX_OUTREGBIT(MIPITX_DSI_DATA_LANE2_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_DATA_LANE2,RG_DSI_LNT2_LDOOUT_EN,1);
		}
	
		// step 15
		if(dsi_params->LANE_NUM > 3)
		{
			MIPITX_OUTREGBIT(MIPITX_DSI_DATA_LANE3_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_DATA_LANE3,RG_DSI_LNT3_LDOOUT_EN,1);
		}*/
	
		// step 16		
		MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CON0_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CON0,RG_DSI0_MPPLL_PLL_EN,1);
	
		// step 17
		udelay(20);
	
		MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CHG_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CHG, RG_DSI0_MPPLL_SDM_PCW_CHG,0);
		MIPITX_OUTREGBIT(MIPITX_DSI_PLL_CHG_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_PLL_CHG, RG_DSI0_MPPLL_SDM_PCW_CHG,1);
	
		// step 18		
		MIPITX_OUTREGBIT(MIPITX_DSI_TOP_CON_REG,DSI_PHY_REG_DPI[i]->MIPITX_DSI_TOP_CON,RG_DSI_PAD_TIE_LOW_EN, 0);
	
		// DONT_KNOW_WHY, for dsi 8 lane, it seems that if we wait more here, the 2 dsi port's data will be always right. othersie will have random color wrong issue
		udelay(200);
	}
}

void ddp_dpi_lvds_config(DISP_MODULE_ENUM module, LCM_DPI_PARAMS *dpi_config, void *cmdq_handle)
{

	LVDS_ANA_Init(module, cmdq_handle);
	LVDS_Clk_Path_Config();
	LVDS_MIPI_PLL_Init(module, cmdq_handle, dpi_config->PLL_CLOCK);
	LVDS_DIG_Init(module, cmdq_handle);
}

