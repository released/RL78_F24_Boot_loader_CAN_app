/* Production CAN adapter + protocol, with register/driver stubs. C90 only.
 * This checks software observations, not CAN timing or physical bus faults.
 */
#include <assert.h>
#include <stdio.h>
#include "platform.h"
#define F24_DEVICE_TRANSFER
#include "device_can.c"
#include "can_transfer_config.h"
#ifdef TRANSFER_TEST_NO_TRACE
#undef CAN_TRANSFER_UART_TRACE_ENABLE
#define CAN_TRANSFER_UART_TRACE_ENABLE (0U)
#endif
#include "can_transfer_protocol.c"

volatile test_rules_t CAN_RXRULE_SFR;
volatile test_ram_t CAN_RAM_MSG_SFR;
volatile uint16_t GAFLCFG, CFDGRWC, GSTS, C0STSL, C0ERFLL, RFSTS0;
volatile uint8_t P1, GAFLECTRH, RFSTS0L, RFPCTR0L, TMC0, TMSTS0;
static e_can_err_t g_config_result, g_start_result;
static uint8_t g_channel_active;
static uint16_t g_tick;
void R_CAN_Create(void) { }
e_can_err_t R_CAN_SetConfig(void) { return g_config_result; }
e_can_err_t R_CAN_StartComm(void) { return g_start_result; }
e_can_err_t R_CAN_StopComm(void) { return CAN_SUCCESS; }
uint8_t R_CAN_GetChStatus(void) { return g_channel_active; }
uint8_t R_CAN_GetChBusErrFlag(void) { assert(0); return 0U; }
unsigned int get_tick(void) { return g_tick; }

static void expect_init(uint8_t stage, uint16_t result, uint8_t ready)
{
    transfer_can_diagnostics_t snapshot;
    assert(Device_CanInit(0x710U, 0U) == ready);
    Device_CanGetDiagnostics(&snapshot);
    assert(snapshot.stage == stage && snapshot.result == result);
    assert(snapshot.global_status == GSTS && snapshot.channel_status == C0STSL);
    assert(snapshot.standby == (uint8_t)!ready);
}
static void inject_errors(uint16_t flags)
{
    uint16_t observed, clear_write;
    C0ERFLL = flags;
    observed = Device_CanPollErrors();
    assert(observed == (flags & 0x7FFFU));
    clear_write = C0ERFLL;
    if (observed) {
        assert(clear_write == (uint16_t)(~observed & 0x7FFFU));
        assert(((0x7FFFU ^ observed) & clear_write) == (0x7FFFU ^ observed));
        /* Emulate write-zero clearing, retaining an unobserved new event. */
        C0ERFLL = (uint16_t)(flags & clear_write);
        CAN_TransferProtocol_OnChannelError(observed);
    }
}
int main(void)
{
    can_frame_t request;
    uint8_t response[8];
    uint8_t bit, index;
    uint16_t expected;
    GSTS = 1U; C0STSL = 1U; RFSTS0 = CAN_U16_RFSTS_RFEMP_ON;
    g_config_result = CAN_ERR_ILLEGAL_STS;
    expect_init(TRANSFER_CAN_STAGE_CONFIG, CAN_ERR_ILLEGAL_STS, 0U);
    g_config_result = CAN_SUCCESS; GSTS = 0U;
    expect_init(TRANSFER_CAN_STAGE_STATE, CAN_ERR_ILLEGAL_STS, 0U);
    GSTS = 1U; g_start_result = CAN_ERR_ILLEGAL_STS;
    expect_init(TRANSFER_CAN_STAGE_START, CAN_ERR_ILLEGAL_STS, 0U);
    g_start_result = CAN_ERR_WAITING;
    expect_init(TRANSFER_CAN_STAGE_START, CAN_ERR_WAITING, 0U);
    g_start_result = CAN_SUCCESS; g_channel_active = 0U;
    expect_init(TRANSFER_CAN_STAGE_ACTIVE, CAN_ERR_WAITING, 0U);
    g_channel_active = 0x80U;
    expect_init(TRANSFER_CAN_STAGE_READY, CAN_SUCCESS, 1U);

    CAN_TransferProtocol_Init(CAN_RTN_OK);
    inject_errors(0U); inject_errors(0x8000U);
    assert(g_stats.last_error_flags == 0U);
    for (bit = 0U; bit < 15U; ++bit) inject_errors((uint16_t)(1U << bit));
    assert(g_stats.warning_entries == 1U && g_stats.passive_entries == 1U);
    assert(g_stats.bus_off_entries == 1U && g_stats.last_error_flags == 0x4000U);
    assert(g_pending_error_flags == 0x7FFFU);
#if CAN_TRANSFER_UART_TRACE_ENABLE
    g_tick = 65530U; CAN_TransferProtocol_Process();
    assert(g_pending_error_flags == 0U);
    inject_errors(0x000EU); g_tick = 100U; CAN_TransferProtocol_Process();
    assert(g_pending_error_flags == 0x000EU);
    g_tick = 244U; CAN_TransferProtocol_Process();
    assert(g_pending_error_flags == 0U);
#else
    CAN_TransferProtocol_Process();
    assert(g_stats.warning_entries == 1U && g_stats.passive_entries == 1U);
    assert(g_stats.bus_off_entries == 1U);
#endif
    for (index = 7U; index <= 11U; ++index) {
        request.DB[2] = 1U; request.DB[3] = CAN_TRANSFER_CMD_GET_STATS;
        request.DB[4] = index; request.DB[5] = 0U; request.DB[6] = 0U;
        assert(dispatch_request(&request, response) == 1U);
        expected = get_stat_value(index);
        assert(response[4] == index && response[5] == (uint8_t)expected);
        assert(response[6] == (uint8_t)(expected >> 8));
        assert(crc8_atm(response, 7U) == response[7]);
    }
    g_stats.warning_entries = 0xFFFFU; inject_errors(2U);
    assert(g_stats.warning_entries == 0xFFFFU);
    request.DB[3] = CAN_TRANSFER_CMD_CLEAR_STATS;
    request.DB[4] = 0xC3U; request.DB[5] = 0x3CU; request.DB[6] = 0U;
    assert(dispatch_request(&request, response) == 1U);
    assert(g_stats.warning_entries == 0U && g_stats.last_error_flags == 0U);
    assert(g_pending_error_flags == 0U);

    /* PING/ECHO/PATTERN stay byte-compatible; no format or target change. */
    request.DB[4] = 0x12U; request.DB[5] = 0x34U; request.DB[6] = 0x56U;
    for (index = 2U; index <= 3U; ++index) {
        request.DB[3] = index;
        assert(dispatch_request(&request, response) == 1U);
        assert(memcmp(request.DB+4, response+4, 3U) == 0);
    }
    request.DB[3] = CAN_TRANSFER_CMD_PATTERN_TEST;
    assert(dispatch_request(&request, response) == 1U);
    assert(response[4] == 0xB7U && response[5] == 0x71U && response[6] == 0xACU);
    g_tx_state = CAN_TRANSFER_TX_SENDING; g_tx_started_at = 65530U;
    g_tick = 94U; TMSTS0 = 0U; poll_transmit();
    assert(g_tx_state == CAN_TRANSFER_TX_ABORTING && TMC0 == CAN_U08_TMC_TMTAR_ON);
    g_tick = 194U; poll_transmit();
    assert(g_tx_state == CAN_TRANSFER_TX_FAULT);
    puts("PASS: initialization stages,15 error bits/W0C,stats/saturation/clear,trace throttle/wrap,commands/TX timeout. Hardware NOT RUN.");
    return 0;
}
