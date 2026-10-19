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
find_package(glm CONFIG REQUIRED)

if(TARGET glfw)
    set(ARDUINO3D_GLFW_TARGET glfw)
elseif(TARGET glfw3::glfw)
    set(ARDUINO3D_GLFW_TARGET glfw3::glfw)
else()
    message(FATAL_ERROR "Alvo CMake do GLFW nao encontrado.")
endif()

foreach(required_target glad::glad glm::glm)
    if(NOT TARGET ${required_target})
        message(FATAL_ERROR
            "Dependencia sem o alvo CMake esperado: ${required_target}"
        )
    endif()
endforeach()

add_library(arduino3d_core
    src/app/Application.cpp
    src/graphics/Camera.cpp
    src/graphics/Scene.cpp
    src/graphics/Renderer.cpp
)

target_include_directories(arduino3d_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(arduino3d_core PUBLIC cxx_std_20)

target_compile_definitions(arduino3d_core PRIVATE
    ARDUINO3D_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

target_link_libraries(arduino3d_core
    PUBLIC
        ${ARDUINO3D_GLFW_TARGET}
        glad::glad
        glm::glm
        OpenGL::GL
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(arduino3d_core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

add_executable(arduino-3d-lab src/main.cpp)
target_link_libraries(arduino-3d-lab PRIVATE arduino3d_core)

if(BUILD_TESTING)
    message(STATUS "CTest habilitado.")
endif()
