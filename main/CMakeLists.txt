set(SOURCES "main.cpp")

set(DEPENDENCIES "external-connectivity")

idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS "" REQUIRES ${DEPENDENCIES})
