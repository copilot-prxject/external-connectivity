#include "BleService.hpp"

#include <esp_log.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <services/ans/ble_svc_ans.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

#include <algorithm>
#include <array>

namespace ble {
constexpr auto logTag = "ble";
constexpr auto deviceName = "esp32-copilot-ext-con";
constexpr size_t characteristicCount{4};

static uint8_t deviceAddress{};
static uint16_t connectionHandle{};
static std::array<std::string, characteristicCount> values{};
static std::array<uint16_t, characteristicCount> valueHandles{};
static std::array<ble_uuid128_t, characteristicCount> characteristicUuids{{
    BLE_UUID128_INIT(0x61, 0x5D, 0x7D, 0xFB, 0xDD, 0x1A, 0x48, 0x0B, 0xAD, 0x99, 0x1E,
                     0xF9, 0x8F, 0x77, 0x69, 0x75),
    BLE_UUID128_INIT(0x62, 0x5D, 0x7D, 0xFB, 0xDD, 0x1A, 0x48, 0x0B, 0xAD, 0x99, 0x1E,
                     0xF9, 0x8F, 0x77, 0x69, 0x75),
    BLE_UUID128_INIT(0x63, 0x5D, 0x7D, 0xFB, 0xDD, 0x1A, 0x48, 0x0B, 0xAD, 0x99, 0x1E,
                     0xF9, 0x8F, 0x77, 0x69, 0x75),
    BLE_UUID128_INIT(0x64, 0x5D, 0x7D, 0xFB, 0xDD, 0x1A, 0x48, 0x0B, 0xAD, 0x99, 0x1E,
                     0xF9, 0x8F, 0x77, 0x69, 0x75),
}};
static constexpr ble_uuid128_t serviceUuid =
    BLE_UUID128_INIT(0x60, 0x5D, 0x7D, 0xFB, 0xDD, 0x1A, 0x48, 0x0B, 0xAD, 0x99, 0x1E,
                     0xF9, 0x8F, 0x77, 0x69, 0x75);
static constexpr ble_gatt_svc_def bleGattServices[]{
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &serviceUuid.u,
        .characteristics =
            (ble_gatt_chr_def[]){
                {
                    .uuid = &characteristicUuids[0].u,
                    .access_cb = BleService::onAccess,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &valueHandles[0],
                },
                {
                    .uuid = &characteristicUuids[1].u,
                    .access_cb = BleService::onAccess,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &valueHandles[1],
                },
                {
                    .uuid = &characteristicUuids[2].u,
                    .access_cb = BleService::onAccess,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &valueHandles[2],
                },
                {
                    .uuid = &characteristicUuids[3].u,
                    .access_cb = BleService::onAccess,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &valueHandles[3],
                },
                {0},
            },
    },
    {0},
};

void BleService::setValue(uint8_t index, const std::string &value) {
    ESP_LOGI(logTag, "Setting value for characteristic %d: %s", index, value.c_str());
    values[index] = value;
    ble_gatts_chr_updated(valueHandles[index]);
}

int BleService::onAccess(uint16_t conn_handle, uint16_t attr_handle,
                         ble_gatt_access_ctxt *context, void *arg) {
    const auto valueHandle =
        std::find(valueHandles.begin(), valueHandles.end(), attr_handle);
    const auto index = std::distance(valueHandles.begin(), valueHandle);
    switch (context->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            ESP_LOGI(logTag, "Read characteristic");
            ESP_ERROR_CHECK(
                os_mbuf_append(context->om, values[index].data(), values[index].size()));
            break;
        case BLE_GATT_ACCESS_OP_READ_DSC:
            ESP_LOGI(logTag, "Read descriptor");
            break;
        default:
            ESP_LOGI(logTag, "Unknown operation %d", context->op);
            break;
    }
    return 0;
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
        case BLE_GATT_REGISTER_OP_DSC:
            ESP_LOGI(logTag, "Registered descriptor %s with dsc_handle: %d",
                     ble_uuid_to_str(context->dsc.dsc_def->uuid, buffer),
                     context->dsc.handle);
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
        default:
            ESP_LOGI(logTag, "Unknown event %d", event->type);
            break;
    }
    return 0;
}

void BleService::advertise() {
    const auto deviceName = ble_svc_gap_device_name();
    ble_hs_adv_fields fields{
        .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
        .name = reinterpret_cast<const uint8_t *>(deviceName),
        .name_len = static_cast<uint8_t>(strlen(deviceName)),
        .name_is_complete = 1,
        .tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO,
        .tx_pwr_lvl_is_present = 1,
    };
    ESP_ERROR_CHECK(ble_gap_adv_set_fields(&fields));

    ble_gap_adv_params advertisementParams = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
    };
    ESP_ERROR_CHECK(ble_gap_adv_start(deviceAddress, nullptr, BLE_HS_FOREVER,
                                      &advertisementParams, onEvent, nullptr));
}

BleService::BleService() {
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