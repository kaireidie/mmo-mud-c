cmake_minimum_required(VERSION 3.16)

project(serverc LANGUAGES C)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Manually get MySQL flags using mysql_config
execute_process(
    COMMAND mysql_config --cflags
    OUTPUT_VARIABLE MYSQL_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND mysql_config --libs
    OUTPUT_VARIABLE MYSQL_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Add the executable
add_executable(serverc main.c
    Opcodes.h
    Packet.h)

# Link OpenSSL and MySQL to the executable
target_link_libraries(serverc OpenSSL::SSL OpenSSL::Crypto ${MYSQL_LIBS})

# Include MySQL headers
target_compile_options(serverc PRIVATE ${MYSQL_CFLAGS})

# Installation settings
include(GNUInstallDirs)
install(TARGETS serverc
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
