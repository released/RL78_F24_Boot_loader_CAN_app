#ifndef F24_DEVICE_PREINCLUDE_H
#define F24_DEVICE_PREINCLUDE_H
/* Load this project's own generated SC settings before the board overrides. */
#include "r_bsp_config.h"
#include "r_rscanfd_rl78_config.h"
#if (BSP_CFG_FMX_HZ != 4000000) || (BSP_CFG_HISYSCLK_SOURCE != 1)
#error F24_device_requires_the_target_board_4MHz_X1_configuration
#endif
/* Do not enter generated, unbounded crystal/PLL waits before main. */
#undef BSP_CFG_MAINCLK_SOURCE
#define BSP_CFG_MAINCLK_SOURCE (0)
#undef BSP_CFG_HISYSCLK_OPERATION
#define BSP_CFG_HISYSCLK_OPERATION (1)
#undef BSP_CFG_PLL_OPERATION
#define BSP_CFG_PLL_OPERATION (0)
#undef BSP_CFG_PLL_MODE
#define BSP_CFG_PLL_MODE (0)
#undef BSP_CFG_FMP_DIVIDE
#define BSP_CFG_FMP_DIVIDE (0)
#undef BSP_CFG_PIOR46
#define BSP_CFG_PIOR46 (0)
#ifdef F24_DEVICE_APP
/* Boot hands over a locked 80MHz PLL / CPU40. CMC is write-once. */
#undef BSP_CFG_STARTUP_DISABLE
#define BSP_CFG_STARTUP_DISABLE (1)
#endif
/* fCAN=PLL80/2=40MHz: 40M / (4 * (1+15+4)) = 500 kbit/s. */
#undef CAN_CFG_NBRP
#define CAN_CFG_NBRP (3)
#undef CAN_CFG_NMNL_TSEG1
#define CAN_CFG_NMNL_TSEG1 (15)
#undef CAN_CFG_NMNL_TSEG2
#define CAN_CFG_NMNL_TSEG2 (4)
#undef CAN_CFG_NMNL_SJW
#define CAN_CFG_NMNL_SJW (1)
#undef CAN_CFG_CLOE_AND_FDOE
#define CAN_CFG_CLOE_AND_FDOE (2)
/* 16 entries hold a complete BEGIN + 8 data + COMMIT burst. */
#undef CAN_CFG_RF0_RFDC
#define CAN_CFG_RF0_RFDC (3)
/* Poll FIFO0 and TX result; no driver ISR may consume the protocol frames. */
#undef CAN_CFG_INTRCANGRVC_USE
#define CAN_CFG_INTRCANGRVC_USE (0)
#undef CAN_CFG_INTRCANGERR_USE
#define CAN_CFG_INTRCANGERR_USE (0)
#undef CAN_CFG_INTRCAN0TRM_USE
#define CAN_CFG_INTRCAN0TRM_USE (0)
#undef CAN_CFG_INTRCAN0CFR_USE
#define CAN_CFG_INTRCAN0CFR_USE (0)
#undef CAN_CFG_INTRCAN0ERR_USE
#define CAN_CFG_INTRCAN0ERR_USE (0)
#endif
