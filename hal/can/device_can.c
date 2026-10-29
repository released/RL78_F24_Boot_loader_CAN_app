/* Project/API reading notes: Device-facing CAN adapter.
 * Return complete frames with their physical channel, preserve bounded receive/diagnostic
 * handling and report real TX completion.
 * Boot/App/Transfer protocol work remains in foreground code.
 */
#include "platform.h"
#include "device_board.h"
#include "r_rscanfd_rl78_if.h"
#include "r_rscanfd_rl78_private.h"
static uint8_t g_ready;
#ifdef F24_DEVICE_TRANSFER
#include "transfer_diagnostics.h"
static uint8_t g_init_stage;
static uint16_t g_init_result;
#define RECORD_INIT(stage, result)                                                                 \
    do                                                                                             \
    {                                                                                              \
        g_init_stage = (stage);                                                                    \
        g_init_result = (uint16_t)(result);                                                        \
    } while (0)

void Device_CanGetDiagnostics(transfer_can_diagnostics_t *diagnostics)
{
    diagnostics->stage = g_init_stage;
    diagnostics->result = g_init_result;
    diagnostics->global_status = GSTS;
    diagnostics->channel_status = C0STSL;
    diagnostics->standby = (uint8_t)((P1 & 4U) != 0U);
}

uint16_t Device_CanPollErrors(void)
{
    uint16_t flags;
    /* The low16 register has the same Classical error-bit positions as the
     * protocol trace. Poll both bytes: GetChBusErrFlag reads only bits8..14.
     * Write ones to unobserved flags so a different newly latched flag survives.
     * No driver error ISR reads/clears this register in the polling profile.
     */
    flags = (uint16_t)(C0ERFLL & 0x7FFFU);
    if (flags != 0U)
    {
        C0ERFLL = (uint16_t)(~flags & 0x7FFFU);
    }
    return flags;
}
#else
#define RECORD_INIT(stage, result) ((void)0)
#endif
/* Fixed Classical data-frame subset of the supplied Renesas driver. Keep the
 * generated source intact; access its documented typed message-RAM windows.
 * This avoids linking FD/remote/variable-storage conversion into an 8KiB Boot. */
static void add_rule(uint16_t index, uint16_t id, uint16_t mask)
{
    volatile st_can_rxrule_sfr_t CAN_NO_BIT_ACCESS *rule;
    GAFLCFG = index + 1U;
    GAFLECTRH = CAN_U08_GAFLECTRH_AFLDAE_ON;
    CFDGRWC = CAN_U16_CFDGRWC_PAGE0;
    rule = &CAN_RXRULE_SFR.RxRule[index];
    rule->IDL = id;
    rule->IDH = 0U;
    rule->ML = mask;
    rule->MH = CAN_U16_GAFLM0H_GAFLIDEM_ON | CAN_U16_GAFLM0H_GAFLRTRM_ON;
    rule->P0LL = 0U;
    rule->P0LH = 0U;
    rule->P0H = 0U;
    rule->P1L = CAN_STORE_RF0;
    GAFLECTRH = 0U;
}
uint8_t Device_CanInit(uint16_t request_id, uint8_t blocks)
{
    e_can_err_t result;
    uint32_t budget;
    g_ready = 0U;
    P1 |= 4U;
    R_CAN_Create();
    result = R_CAN_SetConfig();
    RECORD_INIT(TRANSFER_CAN_STAGE_CONFIG, result);
    if (result != CAN_SUCCESS)
    {
        return 0U;
    }
    RECORD_INIT(TRANSFER_CAN_STAGE_STATE, CAN_ERR_ILLEGAL_STS);
    if (request_id > 0x7FFU || (GSTS & 7U) != 1U || (C0STSL & 7U) != 1U)
    {
        return 0U;
    }
    add_rule(0U, request_id, 0x7FFU);
    if (blocks)
    {
        add_rule(1U, CAN_BOOT_BLOCK_DATA_ID, 0x7F8U);
    }
    budget = 10000UL;
    do
    {
        result = R_CAN_StartComm();
    } while (result == CAN_ERR_WAITING && --budget);
    RECORD_INIT(TRANSFER_CAN_STAGE_START, result);
    if (result != CAN_SUCCESS)
    {
        return 0U;
    }
    budget = 10000UL;
    RECORD_INIT(TRANSFER_CAN_STAGE_ACTIVE, CAN_ERR_WAITING);
    while ((R_CAN_GetChStatus() & 0x80U) == 0U && --budget)
    {
        BSP_NOP();
    }
    if (!budget)
    {
        return 0U;
    }
    P1 &= 0xFBU;
    g_ready = 1U;
    RECORD_INIT(TRANSFER_CAN_STAGE_READY, CAN_SUCCESS);
    return 1U;
}
Can_RtnType Device_CanRead(can_frame_t *frame)
{
    volatile st_can_msgbuf_sfr_t CAN_NO_BIT_ACCESS *msg;
    uint16_t status, page, data;
    uint8_t invalid, index;
    if (!g_ready)
    {
        return CAN_RTN_BUFFER_EMPTY;
    }
    status = RFSTS0;
    if (status & CAN_U16_RFSTS_RFEMP_ON)
    {
        return CAN_RTN_BUFFER_EMPTY;
    }
    if (status & CAN_U16_RFSTS_RFMLT_ON)
    {
        RFSTS0L = CAN_U08_RFSTSL_CLR_RFMLT;
    }
    page = CFDGRWC;
    CFDGRWC = CAN_U16_CFDGRWC_PAGE1;
    msg = &CAN_RAM_MSG_SFR.RxFIFOMsg[0];
    frame->IDL = msg->IDL;
    frame->IDH = msg->IDH;
    frame->IDE = (uint8_t)(frame->IDH >> 15);
    frame->RTR = (uint8_t)((frame->IDH >> 14) & 1U);
    frame->DLC = (uint8_t)(msg->PTRHH >> CAN_POS_RFPTRHH_RFDLC);
    invalid = (uint8_t)(frame->IDE || frame->RTR || (msg->FDCSTSLL & 4U) || frame->DLC > 8U);
    if (!invalid)
    {
        for (index = 0U; index < (frame->DLC + 1U) / 2U; ++index)
        {
            data = msg->DF[index];
            frame->DB[index * 2U] = (uint8_t)data;
            frame->DB[index * 2U + 1U] = (uint8_t)(data >> 8);
        }
    }
    CFDGRWC = CAN_U16_CFDGRWC_KEY | page;
    RFPCTR0L = CAN_U08_RFPCTRL_FIFO_INC;
    if (invalid)
    {
        return CAN_RTN_STS_ERROR;
    }
    return (status & CAN_U16_RFSTS_RFMLT_ON) ? CAN_RTN_OK_WITH_LOST : CAN_RTN_OK;
}
Can_RtnType Device_CanStartTx(const can_frame_t *frame)
{
    volatile st_can_msgbuf_sfr_t CAN_NO_BIT_ACCESS *msg;
    uint16_t page;
    uint8_t index;
    if (!g_ready || frame->DLC > 8U || frame->IDL > 0x7FFU || frame->IDE || frame->RTR)
    {
        return CAN_RTN_STS_ERROR;
    }
    if (TMC0 & 1U)
    {
        return CAN_RTN_STS_ERROR;
    }
    TMSTS0 = 0U;
    if (TMSTS0)
    {
        return CAN_RTN_STS_ERROR;
    }
    page = CFDGRWC;
    CFDGRWC = CAN_U16_CFDGRWC_PAGE1;
    msg = &CAN_RAM_MSG_SFR.TxBufMsg[0];
    msg->IDL = frame->IDL;
    msg->IDH = 0U;
    msg->PTRHH = (uint8_t)(frame->DLC << CAN_POS_TMPTRHH_TMDLC);
    msg->FDCSTSLL = 0U;
    msg->FDCSTSLH = 0U;
    msg->FDCSTSH = 0U;
    for (index = 0U; index < (frame->DLC + 1U) / 2U; ++index)
    {
        msg->DF[index] =
            (uint16_t)frame->DB[index * 2U] |
            ((index * 2U + 1U < frame->DLC) ? ((uint16_t)frame->DB[index * 2U + 1U] << 8) : 0U);
    }
    CFDGRWC = CAN_U16_CFDGRWC_KEY | page;
    TMC0 = CAN_U08_TMC_TMTR_ON;
    return CAN_RTN_OK;
}
Can_RtnType Device_CanTxResult(void)
{
    uint8_t result;
    result = (uint8_t)((TMSTS0 & CAN_U08_TMSTS_TMTRF_MASK) >> CAN_POS_TMSTS_TMTRF);
    if (result)
    {
        TMSTS0 = 0U;
    }
    if (result == 2U)
    {
        return CAN_RTN_TX_END;
    }
    if (!result)
    {
        return CAN_RTN_TRANSMITTING;
    }
    if (result == 3U)
    {
        return CAN_RTN_TX_END_WITH_ABORT_REQ;
    }
    return CAN_RTN_TX_ABORT_OVER;
}
Can_RtnType Device_CanAbortTx(void)
{
    TMC0 = CAN_U08_TMC_TMTAR_ON;
    return CAN_RTN_OK;
}
uint8_t Device_CanSend(can_frame_t *frame)
{
    uint32_t budget;
    Can_RtnType result;
    if (Device_CanStartTx(frame) != CAN_RTN_OK)
    {
        return 0U;
    }
    budget = 200000UL;
    do
    {
        result = Device_CanTxResult();
    } while (result == CAN_RTN_TRANSMITTING && --budget);
    if (result == CAN_RTN_TX_END)
    {
        return 1U;
    }
    (void)Device_CanAbortTx();
    /* Stop the controller on an incomplete abort: no stale buffer reuse. */
    budget = 60000UL;
    while (Device_CanTxResult() == CAN_RTN_TRANSMITTING && --budget)
    {
        BSP_NOP();
    }
    if (!budget)
    {
        Device_CanStop();
    }
    return 0U;
}
void Device_CanStop(void)
{
    P1 |= 4U;
    if (g_ready)
    {
        g_ready = 0U;
        (void)R_CAN_StopComm();
    }
}
uint16_t Device_CanErrors(void)
{
    return R_CAN_GetChBusErrFlag();
}
