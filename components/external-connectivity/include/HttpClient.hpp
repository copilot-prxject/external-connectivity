#pragma once

#include <esp_http_client.h>

#include <map>
#include <string>

namespace extcon::http {

class HttpClient {
public:
    HttpClient() = default;

    esp_err_t get(std::string url);
    esp_err_t post(std::string url, std::map<std::string, std::string> data);

private:
};

}  // namespace extcon::http
