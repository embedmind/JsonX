cmake_minimum_required(VERSION 3.16)

project(JsonX
    VERSION 2.0.0
    LANGUAGES C)

option(JSONX_BUILD_TESTS "Build JsonX desktop smoke tests" ON)

add_library(jsonx STATIC
    src/jx_native_backend.c
    src/jx_parser.c
    src/jx_static_allocator.c
    src/jx_version.c)

target_include_directories(jsonx
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private)

target_compile_features(jsonx PUBLIC c_std_99)

if(MSVC)
    target_compile_options(jsonx PRIVATE /W4)
else()
    target_compile_options(jsonx PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(JSONX_BUILD_TESTS)
    enable_testing()

    add_executable(jsonx_basic_mapping_test
        tests/basic_mapping_test.c)

    target_link_libraries(jsonx_basic_mapping_test PRIVATE jsonx)

    if(NOT CMAKE_CROSSCOMPILING)
        add_test(NAME jsonx_basic_mapping_test
            COMMAND jsonx_basic_mapping_test)
    endif()
endif()
