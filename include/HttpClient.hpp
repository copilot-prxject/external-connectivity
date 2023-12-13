#pragma once

#include <esp_http_client.h>

#include <string>

namespace http {
class HttpClient {
public:
    HttpClient();

    esp_err_t get(std::string url);
    esp_err_t post(std::string url, std::string data);

private:
};
}  // namespace http
