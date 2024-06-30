file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp")

set(DEPENDENCIES
    "bt"
    "console"
    "esp_http_client"
    "esp_modem"
    "fmt"
    "nvs_flash"
    "ttn-esp32")

idf_component_register(
    SRCS         ${SOURCES}
    INCLUDE_DIRS "include"
    REQUIRES     ${DEPENDENCIES}
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
target_link_libraries(${COMPONENT_LIB} magic_enum)
