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
#ifdef USE_LCM_THREE_LANE
	{0xff, 4, {0xaa, 0x55, 0x25, 0x01}},
	{0x6f, 1, {0x16}},
	{0xf7, 1, {0x10}},
	{0xff, 4, {0xaa, 0x55, 0x25, 0x00}},
#endif
	{0xff, 4, {0xAA,0x55,0xA5,0x80}},
	{0x6f, 2, {0x11,0x00}},
	{0xf7, 2, {0x20,0x00}},
	{0x6f, 1, {0x06}},
	{0xf7, 1, {0xa0}},
	{0x6f, 1, {0x19}},
	{0xf7, 1, {0x12}},
	
	{0xf0, 5, {0x55,0xAA,0x52,0x08,0x00}},
	{0xc8, 1, {0x80}},
	
	{0xb1, 2, {0x6C,0x21}},
	{0xb6, 1, {0x08}},
	{0x6F, 1, {0x02}},
	{0xB8, 1, {0x08}},
 
	{0xBB, 2, {0x74,0x44}},
 
	{0xBC, 2, {0x00,0x00}},

	{0xBD, 5, {0x01,0xAC,0x10,0x10,0x01}},

	{0xF0, 5, {0x55,0xAA,0x52,0x08,0x01}},
 
	{0xB0, 2, {0x05,0x05}},
	{0xB1, 2, {0x05,0x05}},
 
	{0xBC, 2, {0xA0,0x01}},
	{0xBD, 2, {0xA0,0x01}},
 
	{0xCA, 1, {0x00}},

	{0xC0, 1, {0x0C}},
 
	{0xBE, 1, {0x65}},

	{0xB3, 2, {0x37,0x37}},
	{0xB4, 2, {0x0F,0x0F}},

	{0xB9, 2, {0x46,0x46}},
	{0xBA, 2, {0x25,0x25}},
 
	{0xF0, 5, {0x55,0xAA,0x52,0x08,0x02}},
 
	{0xEE, 1, {0x01}},
	{0xEF, 4, {0x09,0x06,0x15,0x18}},
                                                                                                                     
	{0xB0, 6, {0x00,0x00,0x00,0x2A,0x00,0x4F}},
	{0x6F, 1, {0x06}},
	{0xB0, 6, {0x00,0x68,0x00,0x80,0x00,0xA7}},
	{0x6F, 1, {0x0C}}, 
	{0xB0, 4, {0x00,0xD4,0x01,0x10}}, 
	{0xB1, 6, {0x01,0x3E,0x01,0x83,0x01,0xBB}},
	{0x6F, 1, {0x06}},
	{0xB1, 6, {0x02,0x10,0x02,0x55,0x02,0x57}},
	{0x6F, 1, {0x0C}},
	{0xB1, 4, {0x02,0x96,0x02,0xDC}},
	{0xB2, 6, {0x03,0x08,0x03,0x47,0x03,0x72}},
	{0x6F, 1, {0x06}},
	{0xB2, 6, {0x03,0x9D,0x03,0xB5,0x03,0xD4}},
	{0x6F,1, {0x0c}},
	{0xB2,4, {0x03,0xE3,0x03,0xF4}},
	{0xB3,4, {0x03,0xFD,0x03,0xFF}},

	{0xF0, 5, {0x55,0xAA,0x52,0x08,0x06}},
	{0xB0, 2, {0x29,0x2A}},
	{0xB1, 2, {0x10,0x12}},
	{0xB2, 2, {0x14,0x16}},
	{0xB3, 2, {0x18,0x1A}},
	{0xB4, 2, {0x08,0x0A}},
	{0xB5, 2, {0x2E,0x2E}},
	{0xB6, 2, {0x2E,0x2E}},
	{0xB7, 2, {0x2E,0x2E}},
	{0xB8, 2, {0x2E,0x00}},
	{0xB9, 2, {0x2E,0x2E}},
	{0xBA, 2, {0x2E,0x2E}},
	{0xBB, 2, {0x01,0x2E}},
	{0xBC, 2, {0x2E,0x2E}},
	{0xBD, 2, {0x2E,0x2E}},
	{0xBE, 2, {0x2E,0x2E}},
	{0xBF, 2, {0x0B,0x09}},
	{0xC0, 2, {0x1B,0x19}},
	{0xC1, 2, {0x17,0x15}},
	{0xC2, 2, {0x13,0x11}},
	{0xC3, 2, {0x2A,0x29}},
	{0xE5, 2, {0x2E,0x2E}},
	{0xC4, 2, {0x29,0x2A}},
	{0xC5, 2, {0x1B,0x19}},
	{0xC6, 2, {0x17,0x15}},
	{0xC7, 2, {0x13,0x11}},
	{0xC8, 2, {0x01,0x0B}},
	{0xC9, 2, {0x2E,0x2E}},
	{0xCA, 2, {0x2E,0x2E}},
	{0xCB, 2, {0x2E,0x2E}},
	{0xCC, 2, {0x2E,0x09}},
	{0xCD, 2, {0x2E,0x2E}},
	{0xCE, 2, {0x2E,0x2E}},
	{0xCF, 2, {0x08,0x2E}},
	{0xD0, 2, {0x2E,0x2E}},
	{0xD1, 2, {0x2E,0x2E}},
	{0xD2, 2, {0x2E,0x2E}},
	{0xD3, 2, {0x0A,0x00}},
	{0xD4, 2, {0x10,0x12}},
	{0xD5, 2, {0x14,0x16}},
	{0xD6, 2, {0x18,0x1A}},
	{0xD7, 2, {0x2A,0x29}},
	{0xE6, 2, {0x2E,0x2E}},
	{0xD8, 5, {0x00,0x00,0x00,0x00,0x00}},
	{0xD9, 5, {0x00,0x00,0x00,0x00,0x00}},
	{0xE7, 1, {0x00}},

	{0xF0, 5, {0x55,0xAA,0x52,0x08,0x03}},
	{0xB0, 2, {0x00,0x00}},
	{0xB1, 2, {0x00,0x00}},
	{0xB2, 5, {0x05,0x00,0x00,0x00,0x00}},

	{0xB6, 5, {0x05,0x00,0x00,0x00,0x00}},
	{0xB7, 5, {0x05,0x00,0x00,0x00,0x00}},

	{0xBA, 5, {0x57,0x00,0x00,0x00,0x00}},
	{0xBB, 5, {0x57,0x00,0x00,0x00,0x00}},

	{0xC0, 4, {0x00,0x00,0x00,0x00}},
	{0xC1, 4, {0x00,0x00,0x00,0x00}},

	{0xC4, 1, {0x60}},
	{0xC5, 1, {0x40}},

	{0xF0, 5, {0x55,0xAA,0x52,0x08,0x05}},
	{0xBD, 5, {0x03,0x01,0x03,0x03,0x03}},
	{0xB0, 2, {0x17,0x06}},
	{0xB1, 2, {0x17,0x06}},
	{0xB2, 2, {0x17,0x06}},
	{0xB3, 2, {0x17,0x06}},
	{0xB4, 2, {0x17,0x06}},
	{0xB5, 2, {0x17,0x06}},

	{0xB8, 1, {0x00}},
	{0xB9, 1, {0x00}},
	{0xBA, 1, {0x00}},
	{0xBB, 1, {0x02}},
	{0xBC, 1, {0x00}},

	{0xC0, 1, {0x07}},

	{0xC4, 1, {0x80}},
	{0xC5, 1, {0xA4}},

	{0xC8, 2, {0x05,0x30}},
	{0xC9, 2, {0x01,0x31}},

	{0xCC, 3, {0x00,0x00,0x3C}},
	{0xCD, 3, {0x00,0x00,0x3C}},

	{0xD1, 5, {0x00,0x05,0x09,0x07,0x10}},
	{0xD2, 5, {0x00,0x05,0x0E,0x07,0x10}},

	{0xE5, 1, {0x06}},
	{0xE6, 1, {0x06}},
	{0xE7, 1, {0x06}},
	{0xE8, 1, {0x06}},
	{0xE9, 1, {0x06}},
	{0xEA, 1, {0x06}},

	{0xED, 1, {0x30}},

	{0x6F,1, {0x11}},
	{0xF3,1, {0x01}},


	{0x35,1, {0x00}},
	
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
#endif

	// DSI
	/* Command mode setting */
#ifdef USE_LCM_THREE_LANE
	params->dsi.LANE_NUM				= LCM_THREE_LANE;
#else
	params->dsi.LANE_NUM				= LCM_FOUR_LANE; 
#endif
	params->dsi.data_format.format      		= LCM_DSI_FORMAT_RGB888;

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
    params->dsi.ssc_disable = 1;
}


static void lcm_init(void)
{
	 //enable VSP & VSN
		SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(10);//Must > 10ms
    SET_RESET_PIN(1);
    MDELAY(120);//Must > 120ms
//    lcm_id_pin_handle();
 
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);  

    LCD_DEBUG("uboot:boe_nt35521_lcm_init\n");
}

static void lcm_suspend(void)
{
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(20);//Must > 10ms
    SET_RESET_PIN(1);
    MDELAY(150);//Must > 120ms

    LCD_DEBUG("kernel:boe_nt35521_lcm_suspend\n");
}

static void lcm_resume(void)
{

    lcm_init();

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

    LCD_DEBUG("kernel:boe_nt35521_lcm_resume\n");
}

static struct LCM_setting_table lcm_read_id[] = {
    //read lcm id
    {0xf0, 5, {0x55,0xAA,0x52,0x08,0x01}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
static unsigned int lcm_compare_id(void)
{
    unsigned char buffer[3];
    int gpio;
    
    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(20);//Must > 10ms
    SET_RESET_PIN(1);
    MDELAY(150);//Must > 120ms
    
    gpio = mt_get_gpio_in(GPIO_DISP_ID0_PIN);
	
    push_table(lcm_read_id, sizeof(lcm_read_id) / sizeof(lcm_read_id), 1); 
    
    read_reg_v2(0xc5, buffer,3); 
#ifdef BUILD_LK
    //0x55,0x21,0x01
    printf("id = 0x%x 0x%x 0x%x, gpio=%d\n", buffer[0], buffer[1], buffer[2], gpio);
#endif
    if(buffer[0] == 0x55 && gpio == 0)
    	return 1;
    return 0;
}

LCM_DRIVER nt35521s_hd720_dsi_vdo_gj_lcm_drv =
{
    .name           	= "nt35521s_hd720_dsi_vdo_gj",
    .set_util_funcs 	= lcm_set_util_funcs,
    .get_params     	= lcm_get_params,
    .init           	= lcm_init,
    .suspend        	= lcm_suspend,
    .resume         	= lcm_resume,
    .compare_id     	= lcm_compare_id,
};
/* END PN: , Added by h84013687, 2013.08.13*/
