file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp")

set(DEPENDENCIES "console" "esp_modem" "fmt")

idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS "include" REQUIRES ${DEPENDENCIES})

target_link_libraries(${COMPONENT_LIB} magic_enum)
