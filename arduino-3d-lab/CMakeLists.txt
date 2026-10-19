cmake_minimum_required(VERSION 3.20)

project(Arduino3DLab
    VERSION 0.1.0
    DESCRIPTION "Simulador experimental de Arduino em 3D"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_library(arduino3d_core
    src/app/Application.cpp
)

target_include_directories(arduino3d_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(arduino3d_core PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(arduino3d_core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

add_executable(arduino-3d-lab
    src/main.cpp
)

target_link_libraries(arduino-3d-lab
    PRIVATE arduino3d_core
)

if(BUILD_TESTING)
    add_executable(test_application
        tests/test_application.cpp
    )

    target_link_libraries(test_application
        PRIVATE arduino3d_core
    )

    add_test(
        NAME application_starts
        COMMAND test_application
    )
endif()
