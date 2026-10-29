/* Project/API reading notes: Project-owned initialization and foreground work.
 * The selected main/startup adapter calls hardware_init() once and loop() repeatedly.
 * Keep application changes here so generated entry sources remain replaceable; lower-level APIs
 * live in this project's hal directories.
 */
#include "platform.h"
#include "device_board.h"
#include "can_boot_transport.h"
#include "app_identity.h"
#include "app_runtime_log.h"
#include "image_metadata.h"
/* App handles discovery/ENTER only; all Flash programming belongs to Boot. */
static unsigned long g_health;
static const unsigned long g_info[] = {
    IMAGE_TARGET_ID | ((unsigned long)CAN_BOOT_PROTOCOL_VERSION << 16) |
        ((unsigned long)IMAGE_FORMAT_VERSION << 24),
    IMAGE_BOOT_VERSION,
    IMAGE_BOOT_BUILD,
    DEVICE_APP_VERSION,
    DEVICE_APP_BUILD,
    IMAGE_APPLICATION_START,
    IMAGE_APPLICATION_CAPACITY,
    IMAGE_CODE_FLASH_SIZE,
    IMAGE_RESERVED_SIZE,
    IMAGE_ERASE_BLOCK_SIZE,
    CAN_BOOT_CAN_MODE_CLASSICAL | ((unsigned long)IMAGE_CHECKSUM_CRC32 << 16) |
        ((unsigned long)CAN_BOOT_PROTOCOL_ID << 24),
    CAN_BOOT_NOMINAL_500K_MASK,
    CAN_BOOT_DATA_BITRATE_MASK,
    CAN_BOOT_REQUEST_ID,
    CAN_BOOT_RESPONSE_ID,
    IMAGE_MCU_PART_0,
    IMAGE_MCU_PART_1,
    IMAGE_MCU_PART_2,
    IMAGE_MCU_PART_3,
    IMAGE_MCU_FAMILY_0,
    IMAGE_MCU_FAMILY_1,
    IMAGE_MCU_FAMILY_2,
    0xFFFFFFFFUL,
    IMAGE_MINIMUM_IMAGE_SIZE,
    IMAGE_POLICY_WORD,
    IMAGE_IRQ_ABI,
    CAN_BOOT_BLOCK_INFO};
/* Handle App-mode discovery/control in foreground context.
 * App does not erase/write itself; accepted ENTER is handed back to Boot through the reset
 * contract.
 */
static void process_control(void)
{
    can_frame_t request, reply;
    unsigned long value;
    unsigned char page;
    if (Device_CanRead(&request) != CAN_RTN_OK || request.IDL != CAN_BOOT_REQUEST_ID ||
        !request.DLC)
    {
        return;
    }
    reply.IDL = CAN_BOOT_RESPONSE_ID;
    reply.IDH = 0U;
    reply.IDE = 0U;
    reply.RTR = 0U;
    reply.DLC = 8U;
    reply.DB[0] = request.DB[0];
    reply.DB[1] = 0U;
    reply.DB[2] = 0U;
    reply.DB[3] = CAN_BOOT_STATE_APPLICATION;
    value = 0UL;
    if (request.DB[0] == CAN_BOOT_CMD_INFO)
    {
        page = (request.DLC == 2U) ? request.DB[1] : 0U;
        reply.DB[2] = page;
        if (request.DLC == 2U && page == 0x1BU)
        {
            if (get_tick() != 0U)
            {
                g_health |= 0x401UL;
            }
            value = g_health;
        }
        else if (request.DLC == 2U && page == CAN_BOOT_INFO_PAGE_VERSION_POLICY)
        {
            ImageMetadataRead32(
                (const unsigned char __far *)(IMAGE_APPLICATION_START + IMAGE_MANIFEST_OFFSET),
                IMAGE_METADATA_REGION_SIZE, IMAGE_VERSION_CONTROL_OFFSET, &value);
            value = 1UL | (value << 1);
        }
        else if (request.DLC != 2U || page > CAN_BOOT_INFO_PAGE_BLOCK)
        {
            reply.DB[1] = CAN_BOOT_STATUS_BAD_SIZE;
        }
        else
        {
            value = g_info[page];
            if (page == CAN_BOOT_INFO_PAGE_APP_IMAGE_SIZE)
            {
                ImageMetadataRead32(
                    (const unsigned char __far *)(IMAGE_APPLICATION_START + IMAGE_MANIFEST_OFFSET),
                    IMAGE_METADATA_REGION_SIZE, IMAGE_MANIFEST_SIZE_OFFSET, &value);
            }
        }
    }
    else if (request.DB[0] == CAN_BOOT_CMD_ENTER)
    {
        reply.DLC = 2U;
        if (request.DLC != 1U)
        {
            reply.DB[1] = CAN_BOOT_STATUS_BAD_SIZE;
        }
        if (Device_CanSend(&reply) && reply.DB[1] == 0U)
        {
            Device_RequestBoot();
        }
        return;
    }
    else
    {
        reply.DB[1] = CAN_BOOT_STATUS_BAD_COMMAND;
    }
    reply.DB[4] = (unsigned char)value;
    reply.DB[5] = (unsigned char)(value >> 8);
    reply.DB[6] = (unsigned char)(value >> 16);
    reply.DB[7] = (unsigned char)(value >> 24);
    (void)Device_CanSend(&reply);
}

static unsigned long g_custom_handoff, g_custom_updated, g_custom_log_counter;
static uint16_t g_custom_last;
static char g_custom_counter_text[APP_RUNTIME_COUNTER_BUFFER_SIZE];

/* Initialize the role selected by this project before foreground processing.
 * Preserve clock, image/handoff and peripheral/IRQ enable order when adding application
 * initialization.
 */
void hardware_init(void)
{
    g_custom_handoff = *(volatile unsigned long __far *)(BOOT_APP_SHARED_RAM_ADDRESS + 4UL);
    g_custom_updated = *(volatile unsigned long __far *)(BOOT_APP_SHARED_RAM_ADDRESS + 8UL);
    *(volatile unsigned long __far *)(BOOT_APP_SHARED_RAM_ADDRESS + 4UL) = 0UL;
    *(volatile unsigned long __far *)(BOOT_APP_SHARED_RAM_ADDRESS + 8UL) = 0UL;
    if (g_custom_handoff != BOOT_APP_HANDOFF_SIGNATURE)
    {
        Device_RequestBoot();
    }
    if (!Device_BoardInit())
    {
        Device_RequestBoot();
    }
    Device_Log("F24 CAN DEVICE APP: boot-validated; X1=4M CPU/CAN40\r\n");
    Device_Log("app image tag: " APP_RUNTIME_LOG_TAG "\r\n");
    if (g_custom_updated == BOOT_APP_UPDATE_COMPLETE_SIGNATURE)
    {
        Device_Log("CAN update: completed\r\n");
    }
    if (!Device_CanInit(CAN_BOOT_REQUEST_ID, 0U))
    {
        Device_RequestBoot();
    }
    g_health = 0x00010300UL | ((unsigned long)RESF << 24);
    if (g_custom_updated == BOOT_APP_UPDATE_COMPLETE_SIGNATURE)
    {
        g_health |= 0x00020000UL;
    }
    g_custom_last = get_tick();
    g_custom_log_counter = 0UL;
}

/* Foreground service point called repeatedly by the selected main adapter.
 * Keep protocol/application work and text logging bounded so pending traffic and deferred actions
 * can continue.
 */
void loop(void)
{

    process_control();
    Device_Heartbeat();
    if ((uint16_t)(get_tick() - g_custom_last) >= 1000U)
    {
        g_custom_last = get_tick();
        App_FormatRuntimeCounter(g_custom_counter_text, g_custom_log_counter++);
        Device_Log("log(timer)-" APP_RUNTIME_LOG_TAG ": ");
        Device_Log((const char __far *)g_custom_counter_text);
        Device_Log("\r\n");
    }
}
