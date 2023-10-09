file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp")

set(DEPENDENCIES "")

idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS "include" REQUIRES ${DEPENDENCIES})
