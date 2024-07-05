#include "BleService.hpp"

#include <copilot/BleConsts.h>
#include <sys/queue.h>

#include <InternalMappings.hpp>
#include <LoraService.hpp>
#include <algorithm>
#include <format>
#include <string>

#include "esp_central.h"
#include "esp_err.h"
#include "esp_log.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "sdkconfig.h"
#include "services/gap/ble_svc_gap.h"

#ifdef CONFIG_EXT_CON_DEBUG_LOGGING
#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif

namespace {

using extcon::Uuid;

constexpr auto logTag{"ble"};
constexpr auto deviceName{"ext-con"};
uint8_t addressType;

constexpr std::string to_string(const ble_addr_t &address) {
    return std::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", address.val[5],
                       address.val[4], address.val[3], address.val[2], address.val[1],
                       address.val[0]);
}

std::string to_upper(const std::string &str) {
    std::string upperStr{str};
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
    return upperStr;
}

peer_chr *getCharacteristic(const peer &peer, Uuid uuid) {
    peer_svc *service;
    SLIST_FOREACH(service, &peer.svcs, next) {
        peer_chr *characteristic;
        SLIST_FOREACH(characteristic, &service->chrs, next) {
            if (characteristic->chr.uuid.u16.value == uuid) {
                return characteristic;
            }
        }
    }
    return nullptr;
}

void debugPrint(const peer *peer) {
    peer_svc *service;
    peer_chr *characteristic;
    peer_dsc *descriptor;
    ESP_LOGD(logTag, "============================================================");
    ESP_LOGD(logTag, "Services:");
    SLIST_FOREACH(service, &peer->svcs, next) {
        ESP_LOGD(logTag, "------------------------------------------------------------");
        ESP_LOGD(logTag, "    UUID:  0x%02X", service->svc.uuid.u16.value);
        ESP_LOGD(logTag, "    start: %d", service->svc.start_handle);
        ESP_LOGD(logTag, "    end:   %d", service->svc.end_handle);
        SLIST_FOREACH(characteristic, &service->chrs, next) {
            ESP_LOGD(logTag, "    Characteristic:");
            ESP_LOGD(logTag, "        UUID:  0x%02X",
                     characteristic->chr.uuid.u16.value);
            ESP_LOGD(logTag, "        def_handle: %d", characteristic->chr.def_handle);
            ESP_LOGD(logTag, "        val_handle: %d", characteristic->chr.val_handle);
            SLIST_FOREACH(descriptor, &characteristic->dscs, next) {
                ESP_LOGD(logTag, "        Descriptor:");
                ESP_LOGD(logTag, "            UUID:  0x%02X",
                         descriptor->dsc.uuid.u16.value);
                ESP_LOGD(logTag, "            handle: %d", descriptor->dsc.handle);
            }
        }
    }
    ESP_LOGD(logTag, "============================================================");
}

}  // namespace

namespace extcon::ble {

void BleService::setValue(uint8_t index, const std::string &value) {
    ESP_LOGI(logTag, "Setting value for characteristic %d: %s", index, value.c_str());
}

bool BleService::init() {
    esp_log_level_set(logTag, ESP_LOG_DEBUG);

    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.gatts_register_cb = nullptr;
    ble_hs_cfg.reset_cb = onReset;
    ble_hs_cfg.sync_cb = onSync;

    ESP_ERROR_CHECK(ble_svc_gap_device_name_set(deviceName));

    constexpr int maxPeers{1};
    constexpr int maxDefinitions{64};
    ESP_ERROR_CHECK(peer_init(maxPeers, maxDefinitions, maxDefinitions, maxDefinitions));

    return true;
}

void BleService::start() {
    ESP_LOGI(logTag, "Starting BleService.");
    nimble_port_freertos_init(loop);
}

void loop(void *) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void scanDevices() {
    constexpr ble_gap_disc_params discoveryParams{
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

void onReset(int reason) {
    ESP_LOGD(logTag, "Resetting, reason: %d", reason);
}

void onSync() {
    ESP_LOGD(logTag, "Synchronized");
    ESP_ERROR_CHECK(ble_hs_util_ensure_addr(0));
    ESP_ERROR_CHECK(ble_hs_id_infer_auto(0, &addressType));
    scanDevices();
}

int onEvent(ble_gap_event *event, void *arg) {
    assert(event != nullptr);
    int result{0};

    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            result = handleEventDiscovery(*event);
            break;
        case BLE_GAP_EVENT_DISC_COMPLETE:
            ESP_LOGD(logTag, "Discovery complete");
            break;
        case BLE_GAP_EVENT_CONNECT:
            result = handleEventConnect(*event);
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            result = handleEventDisconnect(*event);
            break;
        case BLE_GAP_EVENT_NOTIFY_RX:
            result = handleEventNotifyDownlink(*event);
            break;
        default:
            ESP_LOGW(logTag, "Unknown event %d", event->type);
            break;
    }

    return result;
}

int handleEventDiscovery(const ble_gap_event &event) {
    ESP_LOGD(logTag, "Found address: %s", to_string(event.disc.addr).c_str());
    tryConnecting(event);
    return 0;
}

int handleEventConnect(const ble_gap_event &event) {
    if (event.connect.status == 0) {
        int result{peer_add(event.connect.conn_handle)};
        if (result != 0) {
            ESP_LOGE(logTag, "Failed to add peer, result: %d", result);
            return 0;
        }
        result = peer_disc_all(event.connect.conn_handle, onDiscoveryComplete, nullptr);
    } else {
        ESP_LOGW(logTag, "Connection failed, status: %d", event.connect.status);
        scanDevices();
    }
    return 0;
}

int handleEventDisconnect(const ble_gap_event &event) {
    ESP_LOGI(logTag, "Disconnected");
    peer_delete(event.disconnect.conn.conn_handle);
    scanDevices();
    return 0;
}

int handleEventNotifyDownlink(const ble_gap_event &event) {
    ESP_LOGD(logTag,
             "Notification received %s, conn_handle=%d attr_handle=%d attr_len=%d",
             event.notify_rx.indication ? "indication" : "notification",
             event.notify_rx.conn_handle, event.notify_rx.attr_handle,
             OS_MBUF_PKTLEN(event.notify_rx.om));
    char buffer[OS_MBUF_PKTLEN(event.notify_rx.om) + 1];
    os_mbuf_copydata(event.notify_rx.om, 0, sizeof(buffer), buffer);
    buffer[sizeof(buffer)] = '\0';
    ESP_LOGD(logTag, "Data: %s", buffer);
    return 0;
}

void tryConnecting(const ble_gap_event &event) {
    const auto &advertisingReport{event.disc};

    if (!shouldConnect(event)) {
        return;
    }

    ESP_LOGD(logTag, "Connecting");
    const int32_t maxDurationMs{30000};
    ESP_ERROR_CHECK(ble_gap_disc_cancel());
    ESP_ERROR_CHECK(ble_gap_connect(addressType, &advertisingReport.addr, maxDurationMs,
                                    nullptr, onEvent, nullptr));
}

bool shouldConnect(const ble_gap_event &event) {
    const auto &advertisingReport{event.disc};

    if (advertisingReport.event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND &&
        advertisingReport.event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
        return false;
    }

    ble_hs_adv_fields fields;
    int result{ble_hs_adv_parse_fields(&fields, advertisingReport.data,
                                       advertisingReport.length_data)};
    if (result != 0) {
        ESP_LOGW(logTag, "Advertisement parse failed, result: %d", result);
        return false;
    }

    const auto targetAddress{to_upper(CONFIG_EXT_CON_PERIPHERAL_ADDRESS)};
    const auto address{to_string(advertisingReport.addr)};
    if (address != targetAddress) {
        return false;
    }

    return true;
}

void onDiscoveryComplete(const peer *peer, int status, void *arg) {
    if (status != 0) {
        ESP_LOGE(logTag, "Discovery failed, status: %d", status);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }

    ESP_LOGI(logTag, "Service discovery complete");
    debugPrint(peer);

    subscribeToNotifications(*peer);
}

void subscribeToNotifications(const peer &peer) {
    for (const auto &uuid : subscribableCharacteristics) {
        const auto characteristic{getCharacteristic(peer, uuid)};
        if (characteristic == nullptr) {
            ESP_LOGW(logTag, "Subscribable characteristic not found: 0x%02X", uuid);
            continue;
        }
        subscribe(peer, *characteristic);
    }
}

void subscribe(const peer &peer, const peer_chr &characteristic) {
    const auto ccdDescriptor{characteristic.dscs.slh_first->dsc.handle};
    constexpr uint8_t subscribeValue[]{0x01, 0x00};
    auto result{ble_gattc_write_flat(peer.conn_handle, ccdDescriptor, subscribeValue,
                                     sizeof(subscribeValue), nullptr, nullptr)};

    if (result != 0) {
        ESP_LOGE(logTag, "Failed to subscribe to characteristic 0x%02X, result: %d",
                 characteristic.chr.uuid.u16.value, result);
    }
}

}  // namespace extcon::ble
