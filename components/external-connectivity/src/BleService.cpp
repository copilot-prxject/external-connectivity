#include "BleService.hpp"

#include <copilot/BleConsts.h>
#include <esp_log.h>
#include <host/ble_gatt.h>
#include <host/ble_uuid.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <services/ans/ble_svc_ans.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

#include "host/ble_hs_mbuf.h"
#include "os/os_mbuf.h"

namespace ble {

constexpr auto logTag = "ble";
constexpr auto deviceName = "esp32-copilot-ext-con";

static uint8_t deviceAddress{};
static uint16_t connectionHandle{};
static char charValue[64]{""};
static uint16_t valueHandles[3]{0};
static const ble_uuid16_t serviceUuid = BLE_UUID16_INIT(0x5100);
static const ble_uuid16_t serviceUuid2 = BLE_UUID16_INIT(0x5000);
static const ble_uuid16_t serviceUuids[2]{serviceUuid, serviceUuid2};
static const ble_uuid16_t characteristicUuid = BLE_UUID16_INIT(0x5101);
static const ble_uuid16_t characteristicUuid2 = BLE_UUID16_INIT(0x5102);
static const ble_uuid16_t characteristicUuid3 = BLE_UUID16_INIT(0x5001);

static const ble_gatt_svc_def bleGattServices[]{
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &serviceUuid2.u,
        .characteristics =
            (ble_gatt_chr_def[]){
                {
                    .uuid = (ble_uuid_t *)&characteristicUuid3,
                    .access_cb = BleService::onAccess,
                    .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY |
                             BLE_GATT_CHR_F_READ,
                    .val_handle = &valueHandles[0],
                },
                {0},
            },
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &serviceUuid.u,
        .characteristics =
            (ble_gatt_chr_def[]){
                {
                    .uuid = (ble_uuid_t *)&characteristicUuid,
                    .access_cb = BleService::onAccess,
                    .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY |
                             BLE_GATT_CHR_F_READ,
                    .val_handle = &valueHandles[1],
                },
                {
                    .uuid = (ble_uuid_t *)&characteristicUuid2,
                    .access_cb = BleService::onAccess,
                    .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY |
                             BLE_GATT_CHR_F_READ,
                    .val_handle = &valueHandles[2],
                },
                {0},
            },
    },
    {0},
};

int BleService::onAccess(uint16_t conn_handle, uint16_t attr_handle,
                         ble_gatt_access_ctxt *context, void *arg) {
    int result{0};
    switch (context->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            ESP_LOGI(logTag, "Read characteristic");
            ESP_ERROR_CHECK(os_mbuf_append(context->om, charValue, strlen(charValue)));
            break;
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            ESP_LOGI(logTag, "Write characteristic");
            ESP_LOGI(logTag, "UUID type: %d", context->chr->uuid->type);
            ESP_LOGI(logTag, "UUID value: %d",
                     ((ble_uuid16_t *)context->chr->uuid)->value);

            uint16_t omLen;
            omLen = OS_MBUF_PKTLEN(context->om);

            result =
                ble_hs_mbuf_to_flat(context->om, &charValue, sizeof(charValue), &omLen);
            charValue[omLen] = '\0';

            if (result != 0) {
                ESP_LOGE(logTag, "Failed to write characteristic, result: %d", result);
            }

            ble_gatts_chr_updated(attr_handle);
            break;
        default:
            ESP_LOGI(logTag, "Unknown operation %d", context->op);
            break;
    }
    return result;
}

void BleService::loop(void *) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void BleService::onReset(int reason) {
    ESP_LOGI(logTag, "Resetting state; reason=%d", reason);
}

void BleService::onSync() {
    ESP_ERROR_CHECK(ble_hs_util_ensure_addr(0));
    ESP_ERROR_CHECK(ble_hs_id_infer_auto(0, &deviceAddress));
    advertise();
}

void BleService::onRegister(ble_gatt_register_ctxt *context, void *arg) {
    char buffer[BLE_UUID_STR_LEN];
    switch (context->op) {
        case BLE_GATT_REGISTER_OP_SVC:
            ESP_LOGI(logTag, "Registered service %s with handle: %d",
                     ble_uuid_to_str(context->svc.svc_def->uuid, buffer),
                     context->svc.handle);
            break;
        case BLE_GATT_REGISTER_OP_CHR:
            ESP_LOGI(logTag,
                     "Registered characteristic %s with def_handle: %d, val_handle: %d",
                     ble_uuid_to_str(context->chr.chr_def->uuid, buffer),
                     context->chr.def_handle, context->chr.val_handle);
            break;
    }
}

int BleService::onEvent(ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(logTag, "Connected");
            connectionHandle = event->connect.conn_handle;
            if (event->connect.status != 0) {
                ESP_LOGW(logTag, "Connection failed; status=%d", event->connect.status);
                advertise();
            }
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(logTag, "Disconnected");
            connectionHandle = 0;
            advertise();
            break;
        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(logTag, "Subscribe");
            ble_gatts_notify(connectionHandle, valueHandles[0]);
            break;
        default:
            ESP_LOGI(logTag, "Unknown event %d", event->type);
            break;
    }
    return 0;
}

void BleService::advertise() {
    ble_hs_adv_fields fields{
        .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
        .uuids16 = serviceUuids,
        .num_uuids16 = 2,
        .uuids16_is_complete = 1,
        .tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO,
        .tx_pwr_lvl_is_present = 1,
    };

    int result{ble_gap_adv_set_fields(&fields)};
    if (result != 0) {
        ESP_LOGE(logTag, "Failed to set advertisement fields, result: %d", result);
        return;
    }

    ble_gap_adv_params advertisementParams = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
    };
    ESP_ERROR_CHECK(ble_gap_adv_start(deviceAddress, nullptr, BLE_HS_FOREVER,
                                      &advertisementParams, onEvent, nullptr));
}

bool BleService::init() {
    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.reset_cb = onReset;
    ble_hs_cfg.sync_cb = onSync;
    ble_hs_cfg.gatts_register_cb = onRegister;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    ESP_ERROR_CHECK(ble_gatts_count_cfg(bleGattServices));
    ESP_ERROR_CHECK(ble_gatts_add_svcs(bleGattServices));

    ESP_ERROR_CHECK(ble_svc_gap_device_name_set(deviceName));

    return true;
}

void BleService::start() {
    ESP_LOGI(logTag, "BLE service started.");
    nimble_port_freertos_init(loop);
}
}  // namespace ble