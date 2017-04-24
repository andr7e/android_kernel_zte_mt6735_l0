#include <linux/xlog.h>
#include <linux/delay.h>
#include <asm/div64.h>

#include <mach/upmu_common.h>
#include <mach/upmu_hw.h>
#include <mach/upmu_sw.h>

#include <mach/battery_meter_hal.h>
#include <cust_battery_meter.h>
#include <cust_pmic.h>

//============================================================ //
//define
//============================================================ //
#define STATUS_OK    0
#define STATUS_UNSUPPORTED    -1
#define VOLTAGE_FULL_RANGE    1800
#define ADC_PRECISE           32768  // 15 bits

#define UNIT_FGCURRENT     (158122)     // 158.122 uA

//============================================================ //
//global variable
//============================================================ //
kal_int32 chip_diff_trim_value_4_0 = 0;
kal_int32 chip_diff_trim_value = 0; // unit = 0.1

kal_int32 g_hw_ocv_tune_value = 0;

kal_bool g_fg_is_charging = 0;

//============================================================ //
//function prototype
//============================================================ //

//============================================================ //
//extern variable
//============================================================ //
 
//============================================================ //
//extern function
//============================================================ //
extern int PMIC_IMM_GetOneChannelValue(upmu_adc_chl_list_enum dwChannel, int deCount, int trimd);
extern kal_uint32 upmu_get_reg_value(kal_uint32 reg);
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
extern int IMM_IsAdcInitReady(void);
extern U32 pmic_config_interface (U32 RegNum, U32 val, U32 MASK, U32 SHIFT);
extern U32 pmic_read_interface (U32 RegNum, U32 *val, U32 MASK, U32 SHIFT);

//============================================================ // 


kal_int32 use_chip_trim_value(kal_int32 not_trim_val)
{
#if defined(CONFIG_POWER_EXT)
    return not_trim_val;
#else
    kal_int32 ret_val=0;

    ret_val=((not_trim_val*chip_diff_trim_value)/1000);

    bm_print(BM_LOG_FULL, "[use_chip_trim_value] %d -> %d\n", not_trim_val, ret_val);
    
    return ret_val;
#endif    
}

int get_hw_ocv(void)
{
#if defined(CONFIG_POWER_EXT)
    return 4001;    
    bm_print(BM_LOG_CRTI, "[get_hw_ocv] TBD\n");
#else
    kal_int32 adc_result_reg=0;
    kal_int32 adc_result=0;
    kal_int32 r_val_temp=4;    

    #if defined(SWCHR_POWER_PATH)
    adc_result_reg = upmu_get_rg_adc_out_wakeup_swchr();
    adc_result = (adc_result_reg*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;        
    bm_print(BM_LOG_CRTI, "[oam] get_hw_ocv (swchr) : adc_result_reg=%d, adc_result=%d\n", 
        adc_result_reg, adc_result);
    #else
    adc_result_reg = upmu_get_rg_adc_out_wakeup_pchr();
    adc_result = (adc_result_reg*r_val_temp*VOLTAGE_FULL_RANGE)/ADC_PRECISE;        
    bm_print(BM_LOG_CRTI, "[oam] get_hw_ocv (pchr) : adc_result_reg=%d, adc_result=%d\n", 
        adc_result_reg, adc_result);
    #endif

    adc_result += g_hw_ocv_tune_value;
    return adc_result;
#endif    
}


//============================================================//
   
static kal_int32 fgauge_read_current(void *data);
static kal_int32 fgauge_initialization(void *data)
{
	return STATUS_OK;
}

static kal_int32 fgauge_read_current(void *data)
{
    return STATUS_OK;
}

static kal_int32 fgauge_read_current_sign(void *data)
{
    return STATUS_OK;
}

static kal_int32 fgauge_read_columb_internal(void *data, int reset)
{
    return STATUS_OK;
}

static kal_int32 fgauge_read_columb(void *data)
{
    return STATUS_OK;
}

static kal_int32 fgauge_hw_reset(void *data)
{  
    return STATUS_OK;
}


static kal_int32 read_adc_v_bat_sense(void *data)
{
#if defined(CONFIG_POWER_EXT)
    *(kal_int32*)(data) = 4201;
#else
#if defined(SWCHR_POWER_PATH)
	*(kal_int32*)(data) = PMIC_IMM_GetOneChannelValue(AUX_ISENSE_AP,*(kal_int32*)(data),1);
#else
    *(kal_int32*)(data) = PMIC_IMM_GetOneChannelValue(AUX_BATSNS_AP,*(kal_int32*)(data),1);
#endif
#endif

    return STATUS_OK;
}



static kal_int32 read_adc_v_i_sense(void *data)
{
#if defined(CONFIG_POWER_EXT)
    *(kal_int32*)(data) = 4202;
#else
#if defined(SWCHR_POWER_PATH)
	*(kal_int32*)(data) = PMIC_IMM_GetOneChannelValue(AUX_BATSNS_AP,*(kal_int32*)(data),1);
#else
    *(kal_int32*)(data) = PMIC_IMM_GetOneChannelValue(AUX_ISENSE_AP,*(kal_int32*)(data),1);
#endif
#endif

    return STATUS_OK;
}

static kal_int32 read_adc_v_bat_temp(void *data)
{
#if defined(CONFIG_POWER_EXT)
    *(kal_int32*)(data) = 0;
#else
    #if defined(MTK_PCB_TBAT_FEATURE)
        //no HW support
    #else
        bm_print(BM_LOG_FULL, "[read_adc_v_bat_temp] return PMIC_IMM_GetOneChannelValue(4,times,1);\n");
        *(kal_int32*)(data) = PMIC_IMM_GetOneChannelValue(AUX_BATON_AP,*(kal_int32*)(data),1);
    #endif
#endif

    return STATUS_OK;
}

static kal_int32 read_adc_v_charger(void *data)
{    
#if defined(CONFIG_POWER_EXT)
    *(kal_int32*)(data) = 5001;
#else
    kal_int32 val;
    val = PMIC_IMM_GetOneChannelValue(AUX_VCDT_AP,*(kal_int32*)(data),1);
    val = (((R_CHARGER_1+R_CHARGER_2)*100*val)/R_CHARGER_2)/100;
    *(kal_int32*)(data) = val;
#endif

    return STATUS_OK;
}

static kal_int32 read_hw_ocv(void *data)
{
#if defined(CONFIG_POWER_EXT)
    *(kal_int32*)(data) = 3999;
#else
    #if 0
    *(kal_int32*)(data) = PMIC_IMM_GetOneChannelValue(AUX_BATSNS_AP,5,1);
    bm_print(BM_LOG_CRTI, "[read_hw_ocv] By SW AUXADC for bring up\n");
    #else
    *(kal_int32*)(data) = get_hw_ocv();
    #endif
#endif

    return STATUS_OK;
}

static kal_int32 dump_register_fgadc(void *data)
{
    return STATUS_OK;
}

static kal_int32 (* const bm_func[BATTERY_METER_CMD_NUMBER])(void *data)=
{
    fgauge_initialization		//hw fuel gague used only
    
    ,fgauge_read_current		//hw fuel gague used only
    ,fgauge_read_current_sign	//hw fuel gague used only
    ,fgauge_read_columb			//hw fuel gague used only

    ,fgauge_hw_reset			//hw fuel gague used only
    
    ,read_adc_v_bat_sense
    ,read_adc_v_i_sense
    ,read_adc_v_bat_temp
    ,read_adc_v_charger

    ,read_hw_ocv
    ,dump_register_fgadc		//hw fuel gague used only
};

kal_int32 bm_ctrl_cmd(BATTERY_METER_CTRL_CMD cmd, void *data)
{
    kal_int32 status; 

    if(cmd < BATTERY_METER_CMD_NUMBER)
        status = bm_func[cmd](data);
    else
        return STATUS_UNSUPPORTED;
    
    return status;
}

