/* BEGIN PN: , Added by h84013687, 2013.08.13*/
#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
    #include <platform/disp_drv_platform.h>
	
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
//    #include <linux/delay.h>
    #include <mach/mt_gpio.h>
#endif

#ifdef BUILD_LK
#include <stdio.h>
#include <string.h>
#else
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include <cust_gpio_usage.h>
extern void rmidev_reset(void);

#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL,fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

//#define USE_LCM_THREE_LANE 1

const static unsigned char LCD_MODULE_ID = 0x09;//ID0->1;ID1->X
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)

#define REGFLAG_DELAY             								0XFEFF
#define REGFLAG_END_OF_TABLE      							0xFFFF   // END OF REGISTERS MARKER

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

const static unsigned int BL_MIN_LEVEL =20;
static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[128];
};

//update initial param for IC boe_nt35521 0.01
static struct LCM_setting_table lcm_initialization_setting[] = {
#if 0
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x00}},
	{0xff, 4, {0xaa,0x55,0xa5,0x80}},
	{0xc8, 1, {0x80}},
	{0x6f, 1, {0x13}},
	{0xf7, 1, {0x00}},
	{0xb1, 2, {0x7a,0x21}},
	{0x6f, 1, {0x02}},
	{0xb8, 1, {0x0c}},
	
	{0xbb, 2, {0x11,0x11}},
	{0xbc, 2, {0x00,0x00}},
	{0xb6, 1, {0x0f}},
	{0x0f, 5, {0x55,0x11,0x52,0x08,0x01}},
	{0xb0, 2, {0x09,0x09}},
	{0xb1, 2, {0x09,0x09}},
	{0xbc, 2, {0x78,0x00}},
	{0xbd, 2, {0x78,0x00}},
	{0xca, 1, {0x00}},
	{0xc0, 1, {0x04}},
	{0xb5, 2, {0x03,0x03}},
	{0xbe, 1, {0x91}},
	{0xb3, 2, {0x2e,0x2e}},
	{0xb4, 2, {0x0f,0x0f}},
	{0xb9, 2, {0x36,0x36}},
	{0xba, 2, {0x26,0x26}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x02}},
	{0xee, 1, {0x01}},
	{0xb0, 16, {0x00,0x00,0x00,0x09,0x00,0x1d,0x00,0x2e,0x00,0x3d,0x00,0x5a,0x00,0x73,0x00,0x9e}},
	{0xb1, 16, {0x00,0xc1,0x00,0xfc,0x01,0x2c,0x01,0x7a,0x01,0xbc,0x01,0xbe,0x01,0xfc,0x02,0x45}},
	{0xb2, 16, {0x02,0x6f,0x02,0xaa,0x02,0xd1,0x03,0x04,0x03,0x27,0x03,0x50,0x03,0x6a,0x03,0x87}},
	{0xb3, 4, {0x03,0xaa,0x03,0xff}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x06}},
	{0xb0, 2, {0x2d,0x2e}},
	{0xb1, 2, {0x29,0x2a}},
	{0xb2, 2, {0x16,0x18}},
	{0xb3, 2, {0x10,0x12}},
	{0xb4, 2, {0x00,0x02}},
	{0xb5, 2, {0x31,0x31}},
	{0xb6, 2, {0x31,0x31}},
	{0xb7, 2, {0x31,0x31}},
	{0xb8, 2, {0x31,0x31}},
	{0xb9, 2, {0x31,0x31}},
	{0xba, 2, {0x31,0x31}},
	{0xbb, 2, {0x31,0x31}},
	{0xbc, 2, {0x31,0x31}},
	{0xbd, 2, {0x31,0x31}},
	{0xbe, 2, {0x31,0x31}},
	{0xbf, 2, {0x03,0x01}},
	{0xc0, 2, {0x13,0x11}},
	{0xc1, 2, {0x19,0x17}},
	{0xc2, 2, {0x2a,0x29}},
	{0xc3, 2, {0x2e,0x2d}},
	{0xe5, 2, {0x31,0x31}},
	{0xc4, 2, {0x2e,0x2d}},
	{0xc5, 2, {0x29,0x2a}},
	{0xc6, 2, {0x13,0x11}},
	{0xc7, 2, {0x19,0x17}},
	{0xc8, 2, {0x03,0x01}},
	{0xc9, 2, {0x31,0x31}},
	{0xca, 2, {0x31,0x31}},
	{0xcb, 2, {0x31,0x31}},
	{0xcc, 2, {0x31,0x31}},
	{0xcd, 2, {0x31,0x31}},
	{0xce, 2, {0x31,0x31}},
	{0xcf, 2, {0x31,0x31}},
	{0xd0, 2, {0x31,0x31}},
	{0xd1, 2, {0x31,0x31}},
	{0xd2, 2, {0x31,0x31}},
	{0xd3, 2, {0x00,0x02}},
	{0xd4, 2, {0x16,0x18}},
	{0xd5, 2, {0x10,0x12}},
	{0xd6, 2, {0x2a,0x29}},
	{0xd7, 2, {0x2d,0x2e}},
	{0xe6, 2, {0x31,0x31}},
	{0xd8, 5, {0x00,0x00,0x00,0x00,0x00}},
	{0xd9, 5, {0x00,0x00,0x00,0x00,0x00}},
	{0xe7, 1, {0x00}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x05}},
	{0xed, 1, {0x30}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x03}},
	{0xb1, 2, {0x20,0x00}},
	{0xb0, 2, {0x20,0x00}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x05}},
	{0xe5, 1, {0x00}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x05}},
	{0xb0, 2, {0x17,0x06}},
	{0xb8, 1, {0x00}},
	{0xbd, 5, {0x03,0x03,0x00,0x03,0x03}},
	{0xb1, 2, {0x17,0x06}},
	{0xb9, 2, {0x00,0x03}},
	{0xb2, 2, {0x17,0x06}},
	{0xba, 2, {0x00,0x00}},
	{0xb3, 2, {0x17,0x06}},
	{0xbb, 2, {0x02,0x03}},
	{0xb4, 2, {0x17,0x06}},
	{0xb5, 2, {0x17,0x06}},
	{0xb6, 2, {0x17,0x06}},
	{0xb7, 2, {0x17,0x06}},
	{0xbc, 2, {0x02,0x03}},
	{0xe5, 1, {0x06}},
	{0xe6, 1, {0x06}},
	{0xe7, 1, {0x00}},
	{0xe8, 1, {0x06}},
	{0xe9, 1, {0x06}},
	{0xea, 1, {0x06}},
	{0xeb, 1, {0x00}},
	{0xec, 1, {0x00}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x05}},
	{0xc0, 1, {0x0b}},
	{0xc1, 1, {0x09}},
	{0xc2, 1, {0xa6}},
	{0xc3, 1, {0x05}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x03}},
	{0xb2, 5, {0x05,0x00,0x4b,0x00,0x00}},
	{0xb3, 5, {0x05,0x00,0x4b,0x00,0x00}},
	{0xb4, 5, {0x05,0x00,0x17,0x00,0x00}},
	{0xb5, 5, {0x05,0x00,0x17,0x00,0x00}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x05}},
	{0xc4, 1, {0x00}},
	{0xc5, 1, {0x02}},
	{0xc6, 1, {0x22}},
	{0xc7, 1, {0x03}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x03}},
	{0xb6, 5, {0x02,0x00,0x19,0x00,0x00}},
	{0xb7, 5, {0x02,0x00,0x19,0x00,0x00}},
	{0xb8, 5, {0x02,0x00,0x19,0x00,0x00}},
	{0xb9, 5, {0x02,0x00,0x19,0x00,0x00}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x05}},
	{0xc8, 2, {0x07,0x20}},
	{0xc9, 2, {0x03,0x20}},
	{0xca, 2, {0x01,0x60}},
	{0xcb, 2, {0x01,0x60}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x03}},
	{0xba, 5, {0x53,0x00,0x4b,0x00,0x00}},
	{0xbb, 5, {0x53,0x00,0x4b,0x00,0x00}},
	{0xbc, 5, {0x53,0x00,0x1a,0x00,0x00}},
	{0xbd, 5, {0x53,0x00,0x1a,0x00,0x00}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x05}},
	{0xd1, 5, {0x00,0x05,0x01,0x07,0x10}},
	{0xd2, 5, {0x10,0x05,0x05,0x03,0x10}},
	{0xd3, 5, {0x20,0x00,0x43,0x07,0x10}},
	{0xd4, 5, {0x30,0x00,0x43,0x07,0x10}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x05}},
	{0xd0, 7, {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0xd5, 11, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0xd6, 11, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0xd7, 11, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0xd8, 5, {0x00,0x00,0x00,0x00,0x00}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x05}},
	{0xcc, 3, {0x00,0x00,0x01}},
	{0xcd, 3, {0x00,0x00,0x01}},
	{0xce, 3, {0x00,0x00,0x02}},
	{0xcf, 3, {0x00,0x00,0x02}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x03}},
	{0xc0, 4, {0x00,0x34,0x00,0x00}},
	{0xc1, 4, {0x00,0x00,0x34,0x00}},
	{0xc2, 4, {0x00,0x00,0x34,0x00}},
	{0xc3, 4, {0x00,0x00,0x34,0x00}},
	{0xf0, 5, {0x55,0xaa,0x52,0x08,0x03}},
	{0xc4, 1, {0x60}},
	{0xc5, 1, {0xc0}},
	{0xc6, 1, {0x00}},
	{0xc7, 1, {0x00}},
#else
#ifdef USE_LCM_THREE_LANE
	{0xff, 4, {0xaa, 0x55, 0x25, 0x01}},
	{0x6f, 1, {0x16}},
	{0xf7, 1, {0x10}},
	{0xff, 4, {0xaa, 0x55, 0x25, 0x00}},
#endif
	{0xf0, 5, {0x55, 0xaa, 0x52, 0x08, 0x00}},
	{0xff, 4, {0xaa, 0x55, 0xa5, 0x80}},
	
	{0xc8, 1, {0x80}},
	{0x6f, 1, {0x13}},
	{0xf7, 1, {0x00}},
	
	{0xb1, 2, {0x7a, 0x21}},
	{0x6f, 1, {0x02}},
	{0xb8, 1, {0x0c}},
	{0xbb, 2, {0x11, 0x11}},
	{0xbc, 2, {0x00, 0x00}},
	{0xb6, 1, {0x0f}},
	{0xf0, 5, {0x55, 0xaa, 0x52, 0x08, 0x01}},
	{0xb0, 2, {0x09, 0x09}},
	{0xb1, 2, {0x09, 0x09}},
	
	{0xbc, 2, {0x78, 0x00}},
	{0xbd, 2, {0x78, 0x00}},
	
	{0xca, 1, {0x00}},
	{0xc0, 1, {0x0c}},
	{0xb5, 2, {0x03, 0x03}},
	{0xbe, 1, {0x77}},
	{0xb3, 2, {0x2e, 0x2e}},
	{0xb4, 2, {0x0f, 0x0f}},
	{0xb9, 2, {0x36, 0x36}},
	{0xba, 2, {0x26, 0x26}},
	
	{0xf0, 5, {0x55, 0xaa, 0x52, 0x08, 0x02}},
	{0xee, 1, {0x01}},
	{0xb0, 16,{0x00,0x00,0x00,0x09,0x00,0x1D,0x00,0x2E,0x00,0x3D,0x00,0x5A,0x00,0x73,0x00,0x9E}},
	{0xb1, 16,{0x00,0xC1,0x00,0xFC,0x01,0x2C,0x01,0x7A,0x01,0xBC,0x01,0xBE,0x01,0xFC,0x02,0x45}},
	{0xb2, 16,{0x02,0x6F,0x02,0xAA,0x02,0xD1,0x03,0x04,0x03,0x27,0x03,0x50,0x03,0x6A,0x03,0x87}},
	{0xb3, 4, {0x03, 0xaa, 0x03, 0xff}},
	
	{0xf0, 5, {0x55, 0xaa, 0x52, 0x08, 0x06}},
	{0xb0, 2, {0x2d, 0x2e}},
	{0xb1, 2, {0x29, 0x2a}},
	{0xb2, 2, {0x16, 0x18}},
	{0xb3, 2, {0x10, 0x12}},
	{0xb4, 2, {0x00, 0x02}},
	{0xb5, 2, {0x31, 0x31}},
	{0xb6, 2, {0x31, 0x31}},
	{0xb7, 2, {0x31, 0x31}},
	{0xb8, 2, {0x31, 0x31}},
	{0xb9, 2, {0x31, 0x31}},
	
	{0xba, 2, {0x31, 0x31}},
	{0xbb, 2, {0x31, 0x31}},
	{0xbc, 2, {0x31, 0x31}},
	{0xbd, 2, {0x31, 0x31}},
	{0xbe, 2, {0x31, 0x31}},
	{0xbf, 2, {0x03, 0x01}},
	{0xc0, 2, {0x13, 0x11}},
	{0xc1, 2, {0x19, 0x17}},
	{0xc2, 2, {0x2a, 0x29}},
	{0xc3, 2, {0x2e, 0x2d}},
	{0xe5, 2, {0x31, 0x31}},
	
	{0xc4, 2, {0x2e, 0x2d}},
	{0xc5, 2, {0x29, 0x2a}},
	{0xc6, 2, {0x13, 0x11}},
	{0xc7, 2, {0x19, 0x17}},
	{0xc8, 2, {0x03, 0x01}},
	{0xc9, 2, {0x31, 0x31}},
	{0xca, 2, {0x31, 0x31}},
	{0xcb, 2, {0x31, 0x31}},
	{0xcc, 2, {0x31, 0x31}},
	{0xcd, 2, {0x31, 0x31}},
	
	{0xce, 2, {0x31, 0x31}},
	{0xcf, 2, {0x31, 0x31}},
	{0xd0, 2, {0x31, 0x31}},
	{0xd1, 2, {0x31, 0x31}},
	{0xd2, 2, {0x31, 0x31}},
	{0xd3, 2, {0x00, 0x02}},
	{0xd4, 2, {0x16, 0x18}},
	{0xd5, 2, {0x10, 0x12}},
	{0xd6, 2, {0x2a, 0x29}},
	{0xd7, 2, {0x2d, 0x2e}},
	{0xe6, 2, {0x31, 0x31}},
	
	{0xd8, 5, {0x00, 0x00, 0x00, 0x00, 0x00}},
	{0xd9, 5, {0x00, 0x00, 0x00, 0x00, 0x00}},
	{0xe7, 1, {0x00}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x05}},
	{0xed, 1, {0x30}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x03}},
	{0xb1, 2, {0x20, 0x00}},
	{0xb0, 2, {0x20, 0x00}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x05}},
	{0xe5, 1, {0x00}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x05}},
	{0xb0, 2, {0x17, 0x06}},
	{0xb8, 1, {0x00}},
	{0xbd, 5, {0x03,0x03,0x00,0x03,0x03}},
	{0xb1, 2, {0x17, 0x06}},
	{0xb9, 2, {0x00, 0x03}},
	{0xb2, 2, {0x17, 0x06}},
	{0xba, 2, {0x00, 0x00}},
	{0xb3, 2, {0x17, 0x06}},
	{0xbb, 2, {0x02, 0x03}},
	{0xb4, 2, {0x17, 0x06}},
	{0xb5, 2, {0x17, 0x06}},
	{0xb6, 2, {0x17, 0x06}},
	{0xb7, 2, {0x17, 0x06}},
	{0xbc, 2, {0x02, 0x03}},
	{0xe5, 1, {0x06}},
	{0xe6, 1, {0x06}},
	{0xe7, 1, {0x00}},
	{0xe8, 1, {0x06}},
	{0xe9, 1, {0x06}},
	{0xea, 1, {0x06}},
	{0xeb, 1, {0x00}},
	{0xec, 1, {0x00}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x05}},
	{0xc0, 1, {0x0b}},
	{0xc1, 1, {0x09}},
	{0xc2, 1, {0xa6}},
	{0xc3, 1, {0x05}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x03}},
	{0xb2, 5, {0x05,0x00,0x4B,0x00,0x00}},
	{0xb3, 5, {0x05,0x00,0x4B,0x00,0x00}},
	{0xb4, 5, {0x05,0x00,0x17,0x00,0x00}},
	{0xb5, 5, {0x05,0x00,0x17,0x00,0x00}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x05}},
	{0xc4, 1, {0x00}},
	{0xc5, 1, {0x02}},
	{0xc6, 1, {0x22}},
	{0xc7, 1, {0x03}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x03}},
	{0xb6, 5, {0x02,0x00,0x19,0x00,0x00}},
	{0xb7, 5, {0x02,0x00,0x19,0x00,0x00}},
	{0xb8, 5, {0x02,0x00,0x19,0x00,0x00}},
	{0xb9, 5, {0x02,0x00,0x19,0x00,0x00}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x05}},
	{0xc8, 2, {0x07, 0x20}},
	{0xc9, 2, {0x03, 0x20}},
	{0xca, 2, {0x01, 0x60}},
	{0xcb, 2, {0x01, 0x60}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x03}},
	{0xba, 5, {0x53,0x00,0x4B,0x00,0x00}},
	{0xbb, 5, {0x53,0x00,0x4B,0x00,0x00}},
	{0xbc, 5, {0x53,0x00,0x1A,0x00,0x00}},
	{0xbd, 5, {0x53,0x00,0x1A,0x00,0x00}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x05}},
	{0xd1, 5, {0x00,0x05,0x01,0x07,0x10}},
	{0xd2, 5, {0x10,0x05,0x05,0x03,0x10}},
	{0xd3, 5, {0x20,0x00,0x43,0x07,0x10}},
	{0xd4, 5, {0x30,0x00,0x43,0x07,0x10}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x05}},
	{0xd0, 7, {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0xd5, 11, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0xd6, 11, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0xd7, 11, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
	{0xd8, 5, {0x00,0x00,0x00,0x00,0x00}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x05}},
	{0xcc, 3, {0x00,0x00,0x01}},
	{0xcd, 3, {0x00,0x00,0x01}},
	{0xce, 3, {0x00,0x00,0x02}},
	{0xcf, 3, {0x00,0x00,0x02}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x03}},
	{0xc0, 4, {0x00,0x34,0x00,0x00}},
	{0xc1, 4, {0x00,0x00,0x34,0x00}},
	{0xc2, 4, {0x00,0x00,0x34,0x00}},
	{0xc3, 4, {0x00,0x00,0x34,0x00}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x03}},
	{0xc4, 1, {0x60}},
	{0xc5, 1, {0xc0}},
	{0xc6, 1, {0x00}},
	{0xc7, 1, {0x00}},
#endif
	{0x11, 0, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 0, {0x00}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    //Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {
            case REGFLAG_DELAY :
                    MDELAY(table[i].count);
							break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));
	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
    	params->dsi.mode   = CMD_MODE;
#else
    	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
//    	params->dsi.mode   = BURST_VDO_MODE;
#endif

	// DSI
	/* Command mode setting */
#ifdef USE_LCM_THREE_LANE
	params->dsi.LANE_NUM				= LCM_THREE_LANE;
#else
	params->dsi.LANE_NUM				= LCM_FOUR_LANE; 
#endif

	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
	
	params->dsi.packet_size=256;
	params->dsi.intermediat_buffer_num = 0;
	
	//video mode timing
    	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.vertical_sync_active				= 2;
    params->dsi.vertical_backporch				= 14;
    params->dsi.vertical_frontporch				= 16;
    params->dsi.vertical_active_line				= FRAME_HEIGHT;

    params->dsi.horizontal_sync_active			= 8;
    params->dsi.horizontal_backporch				= 42;
    params->dsi.horizontal_frontporch				= 44;
    params->dsi.horizontal_active_pixel			= FRAME_WIDTH;

    //improve clk quality
#ifdef USE_LCM_THREE_LANE
    params->dsi.PLL_CLOCK = 250;//156;
#else
		params->dsi.PLL_CLOCK = 208;//156;
#endif
//    params->dsi.compatibility_for_nvk = 1;
//    params->dsi.ssc_disable = 1;
		params->dsi.ssc_disable = 1;
}

static void lcm_init(void)
{
	 //enable VSP & VSN
		SET_RESET_PIN(1);
		MDELAY(20);
    SET_RESET_PIN(0);
    MDELAY(20);//Must > 10ms
    SET_RESET_PIN(1);
    MDELAY(150);//Must > 120ms
//    lcm_id_pin_handle();
 
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);  

    LCD_DEBUG("uboot:boe_nt35521_lcm_init\n");
}

static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
/*    SET_RESET_PIN(1);
    MDELAY(20);
    SET_RESET_PIN(0);
    MDELAY(20);//Must > 10ms
    SET_RESET_PIN(1);
    MDELAY(150);//Must > 120ms

    LCD_DEBUG("kernel:boe_nt35521_lcm_suspend\n");*/
}

static void lcm_resume(void)
{

//    lcm_init();

    push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);

//    LCD_DEBUG("kernel:boe_nt35521_lcm_resume\n");
//		rmidev_reset();
}


static struct LCM_setting_table lcm_read_id[] = {
    //read lcm id
    {0xf0, 5, {0x55,0xAA,0x52,0x08,0x01}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
static unsigned int lcm_compare_id(void)
{
    unsigned char buffer[3];
    
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(20);//Must > 10ms
    SET_RESET_PIN(1);
    MDELAY(150);//Must > 120ms
    
    push_table(lcm_read_id, sizeof(lcm_read_id) / sizeof(lcm_read_id), 1); 
    
    read_reg_v2(0xc5, buffer,3); 
#ifdef BUILD_LK
    //0x55,0x21,0x01
    printf("id = 0x%x 0x%x 0x%x\n", buffer[0], buffer[1], buffer[2]);
#endif
    if(buffer[0] == 0x55)
    	return 1;
    return 0;
}

LCM_DRIVER nt35521s_hd720_dsi_vdo_txd_lcm_drv =
{
    .name           	= "nt35521s_hd720_dsi_vdo_txd",
    .set_util_funcs 	= lcm_set_util_funcs,
    .get_params     	= lcm_get_params,
    .init           	= lcm_init,
    .suspend        	= lcm_suspend,
    .resume         	= lcm_resume,
    .compare_id     	= lcm_compare_id,
};
/* END PN: , Added by h84013687, 2013.08.13*/