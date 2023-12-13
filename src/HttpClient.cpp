#include "HttpClient.hpp"

#include <esp_log.h>

#include <magic_enum.hpp>

namespace http {
constexpr auto logTag = "http";

esp_err_t handleHttpEvent(esp_http_client_event_t *event) {
    switch (event->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(logTag, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(logTag, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(logTag, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(logTag, "HTTP_EVENT_ON_HEADER");
            ESP_LOGD(logTag, "%s: %s", event->header_key, event->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(logTag, "HTTP_EVENT_ON_DATA");
            ESP_LOG_BUFFER_HEXDUMP(logTag, event->data, event->data_len, ESP_LOG_INFO);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(logTag, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(logTag, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            ESP_LOGD(logTag, "Unhandled event %s", magic_enum::enum_name(event->event_id).data());
            break;
    }
    return ESP_OK;
}

HttpClient::HttpClient() {
}

esp_err_t HttpClient::get(std::string url) {
    esp_http_client_config_t config{
        .url = url.c_str(),
        .event_handler = handleHttpEvent,
    };
    auto client = esp_http_client_init(&config);
    auto result = esp_http_client_perform(client);
    if (result != ESP_OK) {
        ESP_LOGE(logTag, "Failed to perform HTTP request: %s", esp_err_to_name(result));
        return result;
    }

    auto contentLength = esp_http_client_get_content_length(client);
    auto statusCode = esp_http_client_get_status_code(client);
    ESP_LOGI(logTag, "HTTP request completed, status code: %d, content length: %lld", statusCode, contentLength);

    return ESP_OK;
}

esp_err_t HttpClient::post(std::string url, std::string data) {
    return ESP_OK;
}
}  // namespace http