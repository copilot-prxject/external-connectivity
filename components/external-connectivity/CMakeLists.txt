file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp")

set(DEPENDENCIES
    "bt"
    "console"
    "copilot-configs"
    "esp_http_client"
    "esp_modem"
    "fmt"
    "nimble_central_utils"
    "nvs_flash"
    "ttn-esp32")

idf_component_register(
    SRCS         ${SOURCES}
    INCLUDE_DIRS "include"
    REQUIRES     ${DEPENDENCIES}
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
