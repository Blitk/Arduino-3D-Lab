cmake_minimum_required(VERSION 3.20)

project(Arduino3DLab
    VERSION 0.1.0
    DESCRIPTION "Simulador experimental de Arduino em 3D"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

if(TARGET glfw)
    set(ARDUINO3D_GLFW_TARGET glfw)
elseif(TARGET glfw3::glfw)
    set(ARDUINO3D_GLFW_TARGET glfw3::glfw)
else()
    message(FATAL_ERROR "O pacote GLFW nao exportou um alvo conhecido.")
endif()

if(NOT TARGET glad::glad)
    message(FATAL_ERROR
        "O pacote GLAD deve disponibilizar o alvo glad::glad (GLAD 2)."
    )
endif()

add_library(arduino3d_core
    src/app/Application.cpp
)

target_include_directories(arduino3d_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(arduino3d_core
    PUBLIC
        ${ARDUINO3D_GLFW_TARGET}
        glad::glad
        OpenGL::GL
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
    message(STATUS
        "CTest habilitado. Testes que abrem janelas devem ser executados "
        "em ambiente grafico apropriado."
    )
endif()
