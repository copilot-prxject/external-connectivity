#include "BleService.hpp"

#include <sdkconfig.h>

#ifdef CONFIG_EXT_CON_DEBUG_LOGGING
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif

#include <copilot/BleConsts.h>
#include <esp_err.h>
#include <esp_log.h>
#include <host/ble_gap.h>
#include <host/ble_gatt.h>
#include <host/ble_hs.h>
#include <host/ble_hs_adv.h>
#include <host/ble_hs_id.h>
#include <host/ble_uuid.h>
#include <nimble/ble.h>
#include <nimble/hci_common.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <services/ans/ble_svc_ans.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

#include <LoraService.hpp>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <format>
#include <string>

namespace {

constexpr auto logTag{"ble"};
constexpr auto deviceName{"external-connectivity"};

ble::Peer peer;
uint8_t addressType{};

const auto targetAddress{[] {
    std::string address{CONFIG_EXT_CON_PERIPHERAL_ADDRESS};
    std::transform(address.begin(), address.end(), address.begin(), ::toupper);
    return address;
}()};

const std::array targetServices{
    GATT_SVC_OBD_MODULE,
    // GATT_SVC_CONTROL_MEASUREMENT_MODULE,
};

std::string to_string(const ble_addr_t &address) {
    return std::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", address.val[5],
                       address.val[4], address.val[3], address.val[2], address.val[1],
                       address.val[0]);
}

void printPeer() {
    ESP_LOGD(logTag, "============================================================");
    ESP_LOGD(logTag, "Peer:");
    ESP_LOGD(logTag, "  Connection handle: %d", peer.connectionHandle);
    ESP_LOGD(logTag, "  Address: %s", peer.address.c_str());
    ESP_LOGD(logTag, "  Services:");
    for (const auto &service : peer.services) {
        ESP_LOGD(logTag, "    UUID:  0x%02X", service.definition.uuid.u16.value);
        ESP_LOGD(logTag, "    start: %d", service.definition.start_handle);
        ESP_LOGD(logTag, "    end:   %d", service.definition.end_handle);
        ESP_LOGD(logTag, "    Characteristics:");
        for (const auto &characteristic : service.characteristics) {
            ESP_LOGD(logTag, "      UUID:  0x%02X",
                     characteristic.definition.uuid.u16.value);
            ESP_LOGD(logTag, "      def:    %d", characteristic.definition.def_handle);
            ESP_LOGD(logTag, "      handle: %d", characteristic.definition.val_handle);
            ESP_LOGD(logTag, "      Descriptor:");
            ESP_LOGD(logTag, "        UUID:  0x%02X",
                     characteristic.descriptor.uuid.u16.value);
            ESP_LOGD(logTag, "        handle: %d", characteristic.descriptor.handle);
        }
    }
    ESP_LOGD(logTag, "============================================================");
}

}  // namespace

namespace ble {

void BleService::setValue(uint8_t index, const std::string &value) {
    ESP_LOGD(logTag, "Setting value for characteristic %d: %s", index, value.c_str());
    // values[index] = value;
    // ble_gatts_chr_updated(valueHandles[index]);
}

bool BleService::init() {
    esp_log_level_set(logTag, ESP_LOG_DEBUG);

    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.gatts_register_cb = nullptr;
    ble_hs_cfg.reset_cb = onReset;
    ble_hs_cfg.sync_cb = onSync;

    ESP_ERROR_CHECK(ble_svc_gap_device_name_set(deviceName));

    return true;
}

void BleService::start() {
    ESP_LOGI(logTag, "Starting BleService.");
    nimble_port_freertos_init(loop);
}

void BleService::loop(void *) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void BleService::onReset(int reason) {
    ESP_LOGD(logTag, "Resetting, reason: %d", reason);
}

void BleService::onSync() {
    ESP_LOGD(logTag, "Synchronized");
    ESP_ERROR_CHECK(ble_hs_util_ensure_addr(0));
    ESP_ERROR_CHECK(ble_hs_id_infer_auto(0, &addressType));
    scan();
}

void BleService::scan() {
    const ble_gap_disc_params discoveryParams{
        .itvl = 0,
        .window = 0,
        .filter_policy = 0,
        .limited = 0,
        .passive = 1,
        .filter_duplicates = 1,
    };

    ESP_ERROR_CHECK(
        ble_gap_disc(addressType, BLE_HS_FOREVER, &discoveryParams, onEvent, nullptr));
}

int BleService::onEvent(ble_gap_event *event, void *arg) {
    ble_hs_adv_fields fields;
    int result;

    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            ESP_LOGD(logTag, "Discovery complete");
            result = ble_hs_adv_parse_fields(&fields, event->disc.data,
                                             event->disc.length_data);
            if (result != 0) {
                ESP_LOGW(logTag, "Advertisement parse failed, result: %d", result);
                break;
            }

            tryConnecting(event->disc);
            break;
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGD(logTag, "Connected");
            peer.connectionHandle = event->connect.conn_handle;
            if (event->connect.status != 0) {
                ESP_LOGW(logTag, "Connection failed, status: %d", event->connect.status);
                scan();
                break;
            }

            discoverServices();
            ESP_LOGD(logTag, "Services discovered");
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGD(logTag, "Disconnected");
            peer.connectionHandle = 0;
            scan();
            break;
        case BLE_GAP_EVENT_NOTIFY_RX: {
            ESP_LOGD(logTag,
                     "Notification received %s, conn_handle=%d attr_handle=%d "
                     "attr_len=%d",
                     event->notify_rx.indication ? "indication" : "notification",
                     event->notify_rx.conn_handle, event->notify_rx.attr_handle,
                     OS_MBUF_PKTLEN(event->notify_rx.om));
            char buffer[OS_MBUF_PKTLEN(event->notify_rx.om) + 1];
            os_mbuf_copydata(event->notify_rx.om, 0, sizeof(buffer), buffer);
            buffer[sizeof(buffer)] = '\0';
            ESP_LOGD(logTag, "Data: %s", buffer);
            break;
        }
        default:
            ESP_LOGW(logTag, "Unknown event %d", event->type);
            break;
    }
    return 0;
}

void BleService::tryConnecting(const ble_gap_disc_desc &advertisingReport) {
    ESP_LOGD(logTag, "Found address: %s", to_string(advertisingReport.addr).c_str());
    if (!shouldConnect(advertisingReport)) {
        return;
    }

    ESP_LOGD(logTag, "Connecting");
    ESP_ERROR_CHECK(ble_gap_disc_cancel());

    const int32_t maxDurationMs{30000};
    ESP_ERROR_CHECK(ble_gap_connect(addressType, &advertisingReport.addr, maxDurationMs,
                                    nullptr, onEvent, nullptr));
}

bool BleService::shouldConnect(const ble_gap_disc_desc &advertisingReport) {
    if (advertisingReport.event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND &&
        advertisingReport.event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
        return false;
    }

    ble_hs_adv_fields fields;
    int result{ble_hs_adv_parse_fields(&fields, advertisingReport.data,
                                       advertisingReport.length_data)};

    if (result != 0) {
        return false;
    }

    const auto address{to_string(advertisingReport.addr)};
    if (address != targetAddress) {
        return false;
    }

    return true;
}

void BleService::discoverServices() {
    peer.services.clear();
    int result{
        ble_gattc_disc_all_svcs(peer.connectionHandle, onServiceDiscovery, nullptr)};
    if (result != 0) {
        ESP_LOGE(logTag, "Service discovery failed, result: %d", result);
    }

    discoverCharacteristics();
}

int BleService::onServiceDiscovery(uint16_t connection, const ble_gatt_error *error,
                                   const ble_gatt_svc *service, void *arg) {
    int result{0};

    switch (error->status) {
        case 0: {
            if (service->uuid.u.type != BLE_UUID_TYPE_16) {
                break;
            }
            // const auto targetService{std::find(
            //     targetServices.begin(), targetServices.end(),
            //     service->uuid.u16.value)};
            // if (targetService == targetServices.end()) {
            //     break;
            // }

            ESP_LOGD(logTag, "Service found, UUID: 0x%02X", service->uuid.u16.value);
            peer.services.emplace_back(*service);
            break;
        }
        case BLE_HS_EDONE:
            ESP_LOGD(logTag, "Service discovery complete");
            discoverCharacteristics();
            break;
        default:
            ESP_LOGE(logTag, "Service discovery failed, status: %d", error->status);
            break;
    }

    return result;
}

void BleService::discoverCharacteristics() {
    for (const auto &service : peer.services) {
        ESP_LOGD(logTag, "Discovering characteristics for service 0x%02X",
                 service.definition.uuid.u16.value);
        int result{ble_gattc_disc_all_chrs(
            peer.connectionHandle, service.definition.start_handle,
            service.definition.end_handle, onCharacteristicDiscovery,
            (void *)(&service))};
        if (result != 0) {
            ESP_LOGE(logTag, "Characteristic discovery failed, result: %d", result);
        }
    }

    discoverDescriptors();
}

int BleService::onCharacteristicDiscovery(uint16_t connection,
                                          const ble_gatt_error *error,
                                          const ble_gatt_chr *characteristic,
                                          void *arg) {
    int result{0};
    auto service{reinterpret_cast<GattService *>(arg)};

    switch (error->status) {
        case 0:
            if (characteristic->uuid.u.type != BLE_UUID_TYPE_16) {
                break;
            }
            ESP_LOGD(logTag, "Characteristic found, UUID: 0x%02X in service 0x%02X",
                     characteristic->uuid.u16.value, service->definition.uuid.u16.value);
            service->characteristics.emplace_back(*characteristic);
            break;
        case BLE_HS_EDONE:
            ESP_LOGD(logTag, "Characteristic discovery complete");
            discoverDescriptors();
            break;
        default:
            ESP_LOGE(logTag, "Characteristic discovery failed, status: %d",
                     error->status);
            result = error->status;
            break;
    }

    if (result != 0) {
        // terminateConnection();
    }
    return result;
}

void BleService::discoverDescriptors() {
    for (auto &service : peer.services) {
        for (auto &characteristic : service.characteristics) {
            const auto startHandle{characteristic.definition.val_handle};
            const auto &nextCharacteristic{std::next(&characteristic)};
            const auto endHandle{(nextCharacteristic != nullptr)
                                     ? nextCharacteristic->definition.def_handle - 1
                                     : service.definition.end_handle};

            ESP_LOGD(logTag,
                     "Discovering descriptors for characteristic 0x%02X, start: %d, "
                     "end: %d",
                     characteristic.definition.uuid.u16.value, startHandle, endHandle);
            int result{ble_gattc_disc_all_dscs(peer.connectionHandle, startHandle,
                                               endHandle, onDescriptorDiscovery,
                                               (void *)(&characteristic))};
            if (result != 0) {
                ESP_LOGE(logTag, "Descriptor discovery failed, result: %d", result);
            }
        }
    }
}

int BleService::onDescriptorDiscovery(uint16_t connection, const ble_gatt_error *error,
                                      uint16_t chr_val_handle,
                                      const ble_gatt_dsc *descriptor, void *arg) {
    int result{0};
    auto characteristic{reinterpret_cast<GattCharacteristic *>(arg)};

    switch (error->status) {
        case 0:
            if (descriptor->uuid.u.type != BLE_UUID_TYPE_16) {
                break;
            }
            ESP_LOGD(logTag, "Descriptor found, UUID: 0x%02X for characteristic 0x%02X",
                     descriptor->uuid.u16.value,
                     characteristic->definition.uuid.u16.value);
            characteristic->descriptor = *descriptor;
            break;
        case BLE_HS_EDONE:
            ESP_LOGD(logTag, "Descriptor discovery complete");
            printPeer();
            testSubscribe();
            testWrite();
            break;
        default:
            ESP_LOGE(logTag, "Descriptor discovery failed, status: %d", error->status);
            result = error->status;
            break;
    }

    if (result != 0) {
        // terminateConnection();
        printPeer();
        testSubscribe();
        testWrite();
    }
    return result;
}

void BleService::terminateConnection() {
    ESP_LOGE(logTag, "Terminating connection");
    ble_gap_terminate(peer.connectionHandle, BLE_ERR_REM_USER_CONN_TERM);
}

std::optional<GattService> getService(uint16_t serviceUuid) {
    const auto service{
        std::ranges::find_if(peer.services, [serviceUuid](const auto &service) {
            return service.definition.uuid.u16.value == serviceUuid;
        })};

    if (service == peer.services.end()) {
        return std::nullopt;
    }

    return *service;
}

std::optional<GattCharacteristic> getCharacteristic(uint16_t serviceUuid,
                                                    uint16_t characteristicUuid) {
    const auto service{getService(serviceUuid)};
    if (!service) {
        return std::nullopt;
    }

    const auto characteristic{std::ranges::find_if(
        service->characteristics, [characteristicUuid](const auto &characteristic) {
            return characteristic.definition.uuid.u16.value == characteristicUuid;
        })};

    if (characteristic == service->characteristics.end()) {
        return std::nullopt;
    }

    return *characteristic;
}

void BleService::testSubscribe() {
    const auto characteristic{
        getCharacteristic(GATT_SVC_OBD_MODULE, GATT_CHR_ENGINE_SPEED)};
    if (!characteristic) {
        ESP_LOGE(logTag, "Characteristic not found");
        return;
    }
    const auto descriptor{characteristic->descriptor};
    ESP_LOGD(logTag, "Subscribing to characteristic 0x%02X using descriptor 0x%02X",
             characteristic->definition.uuid.u16.value, descriptor.uuid.u16.value);

    if (descriptor.uuid.u16.value != BLE_GATT_DSC_CLT_CFG_UUID16) {
        ESP_LOGE(logTag, "Descriptor is not a client characteristic configuration");
        return;
    }

    uint8_t value[2] = {0x01, 0x00};
    int result{ble_gattc_write_flat(peer.connectionHandle, descriptor.handle, value,
                                    sizeof(value), nullptr, nullptr)};

    if (result != 0) {
        ESP_LOGE(logTag, "Subscribe failed, result: %d", result);
    }
}

void BleService::testWrite() {
    const auto characteristic{
        getCharacteristic(GATT_SVC_OBD_MODULE, GATT_CHR_ENGINE_SPEED)};
    if (!characteristic) {
        ESP_LOGE(logTag, "Characteristic not found");
        return;
    }

    const uint8_t value{69};
    int result{ble_gattc_write_flat(peer.connectionHandle,
                                    characteristic->definition.val_handle, &value,
                                    sizeof(value), nullptr, nullptr)};
    if (result != 0) {
        ESP_LOGE(logTag, "Write failed, result: %d", result);
    }
}

}  // namespace ble
