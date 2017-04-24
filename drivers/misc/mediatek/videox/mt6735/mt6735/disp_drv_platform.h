#ifndef __DISP_DRV_PLATFORM_H__
#define __DISP_DRV_PLATFORM_H__

#include <linux/dma-mapping.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/m4u.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_irq.h>
#include <board-custom.h>
#include <linux/disp_assert_layer.h>
#include "ddp_hal.h"
#include "ddp_drv.h"
#include "ddp_path.h"
#include "ddp_rdma.h"
#include "ddp_ovl.h"

#include <mach/sync_write.h>

#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))

/************************Feature options****************************/

/**
 * SODI enable.
 */
#define MTK_FB_SODI_SUPPORT

/**
 * ESD recovery support.
 */
#define MTK_FB_ESD_ENABLE

/**
 * FB Ion support.
 */
#define MTK_FB_ION_SUPPORT

/**
 * Enable idle screen low power mode.
 */
#define MTK_DISP_IDLE_LP

/**
 * Enable Multipass support.
 */
//#define OVL_MULTIPASS_SUPPORT

/**
 * Build CMDQ command in trigger stage.
 */
#define CONFIG_ALL_IN_TRIGGER_STAGE

/**
 * Disable M4U of display engines.
 */
//#define MTKFB_NO_M4U

/**
 * Bring-up display in kernel stage (not LK stage).
 * Please also turn on the option MACH_FPGA_NO_DISPLAY=y in LK.
 */
//#define MTK_NO_DISP_IN_LK  // do not enable display in LK

/**
 * Disable using CMDQ in display driver.
 * The registers and system event would be processed by CPU.
 */
//#define MTK_FB_CMDQ_DISABLE

/**
 * Bypass display module. No frame would be updated to screen.
 */
//#define MTK_FB_DO_NOTHING

/**
 * Bypass ALL display PQ engine.
 */
//#define MTKFB_FB_BYPASS_PQ

/**
 * Enable display auto-update testing.
 * Display driver would fill the FB and output to panel directly while probe complete.
 */
//#define FPGA_DEBUG_PAN

/**
 * Disable dynamic display resolution adjustment.
 */
#define MTK_FB_DFO_DISABLE
//#define DFO_USE_NEW_API


/************************Display Capabilies****************************/
//These configurations should not be changed.

/**
 * FB alignment byte.
 */
#ifdef CONFIG_FPGA_EARLY_PORTING
#define MTK_FB_ALIGNMENT 16
#else
#define MTK_FB_ALIGNMENT 32
#endif

/**
 * DUAL OVL engine support.
 */
//#define OVL_CASCADE_SUPPORT

/**
 * OVL layer configurations.
 */
#define HW_OVERLAY_COUNT                 (OVL_LAYER_NUM)
#define RESERVED_LAYER_COUNT             (2)
#define VIDEO_LAYER_COUNT                (HW_OVERLAY_COUNT - RESERVED_LAYER_COUNT)
#define PRIMARY_DISPLAY_HW_OVERLAY_LAYER_NUMBER 		(4)
#define PRIMARY_DISPLAY_HW_OVERLAY_ENGINE_COUNT 		(2)
#ifdef OVL_CASCADE_SUPPORT
sdfdsf
#define PRIMARY_DISPLAY_HW_OVERLAY_CASCADE_COUNT 	(2)
#else
#define PRIMARY_DISPLAY_HW_OVERLAY_CASCADE_COUNT 	(1)
#endif
#define PRIMARY_DISPLAY_SESSION_LAYER_COUNT			(PRIMARY_DISPLAY_HW_OVERLAY_LAYER_NUMBER*PRIMARY_DISPLAY_HW_OVERLAY_CASCADE_COUNT)
#define EXTERNAL_DISPLAY_SESSION_LAYER_COUNT			(PRIMARY_DISPLAY_HW_OVERLAY_LAYER_NUMBER*PRIMARY_DISPLAY_HW_OVERLAY_CASCADE_COUNT)
#define DISP_SESSION_OVL_TIMELINE_ID(x)  		(x)
//#define DISP_SESSION_OUTPUT_TIMELINE_ID  	(PRIMARY_DISPLAY_SESSION_LAYER_COUNT)
//#define DISP_SESSION_PRESENT_TIMELINE_ID  	(PRIMARY_DISPLAY_SESSION_LAYER_COUNT+1)
//#define DISP_SESSION_TIMELINE_COUNT 			(DISP_SESSION_PRESENT_TIMELINE_ID+1)
typedef enum
{
	DISP_SESSION_OUTPUT_TIMELINE_ID = PRIMARY_DISPLAY_SESSION_LAYER_COUNT,
	DISP_SESSION_PRESENT_TIMELINE_ID,
	DISP_SESSION_OUTPUT_INTERFACE_TIMELINE_ID,
	DISP_SESSION_TIMELINE_COUNT,
}DISP_SESSION_ENUM;

/**
 * Disable sodi in video mode.
 */
#define DISABLE_SODI_IN_VDO

/**
 * Session count.
 */
#define MAX_SESSION_COUNT 5

/**
 * Need to control SODI enable/disable by SW.
 */
//#define FORCE_SODI_BY_SW

/**
 * Keep SODI in CG mode.
 */
#define FORCE_SODI_CG_MODE

/**
 * Support RDMA1 engine.
 */
#define MTK_FB_RDMA1_SUPPORT

/**
 * The maximum compose layer OVL can support in one pass.
 */
#define DISP_HW_MAX_LAYER 4

/**
 * WDMA_PATH_CLOCK_DYNAMIC_SWITCH:
 * Dynamice turn on/off WDMA path clock. This feature is necessary in MultiPass for SODI.
 */
#ifdef OVL_MULTIPASS_SUPPORT
#define WDMA_PATH_CLOCK_DYNAMIC_SWITCH
#endif

/**
 * HW_MODE_CAP: Direct-Link, Decouple or Switchable.
 * HW_PASS_MODE: Multi-Pass, Single-Pass.
 */
#ifdef CONFIG_MTK_GMO_RAM_OPTIMIZE
  #define DISP_HW_MODE_CAP DISP_OUTPUT_CAP_DIRECT_LINK
  #define DISP_HW_PASS_MODE DISP_OUTPUT_CAP_SINGLE_PASS

  //Disable MTK_DISP_IDLE_LP feature in LCA mode.
  #ifdef MTK_DISP_IDLE_LP
    #undef MTK_DISP_IDLE_LP
  #endif
#else
  #define DISP_HW_MODE_CAP DISP_OUTPUT_CAP_SWITCHABLE
  #ifdef OVL_MULTIPASS_SUPPORT
    #define DISP_HW_PASS_MODE DISP_OUTPUT_CAP_MULTI_PASS
  #else
    #define DISP_HW_PASS_MODE DISP_OUTPUT_CAP_SINGLE_PASS
  #endif
#endif

#endif //__DISP_DRV_PLATFORM_H__
