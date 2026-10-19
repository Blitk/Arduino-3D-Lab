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


# Núcleo lógico: sem dependências gráficas.
add_library(arduino3d_circuit
    src/circuit/Circuit.cpp
    src/circuit/Pin.cpp
    src/circuit/Wire.cpp
    src/circuit/Led.cpp
)

target_include_directories(arduino3d_circuit
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(arduino3d_circuit PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(arduino3d_circuit PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# A aplicação gráfica pode consumir a biblioteca lógica.
target_link_libraries(arduino3d_core PUBLIC arduino3d_circuit)

# Teste independente do OpenGL.
if(BUILD_TESTING)
    add_executable(test_circuit tests/test_circuit.cpp)
    target_link_libraries(test_circuit PRIVATE arduino3d_circuit)

    add_test(NAME circuit COMMAND test_circuit)
endif()

add_library(arduino3d_emulator
    src/emulator/Memory.cpp
    src/emulator/CPU.cpp
)

target_include_directories(arduino3d_emulator
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(arduino3d_emulator PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(arduino3d_emulator PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

if(BUILD_TESTING)
    add_executable(test_memory tests/test_memory.cpp)
    target_link_libraries(test_memory PRIVATE arduino3d_emulator)
    add_test(NAME memory COMMAND test_memory)

    add_executable(test_cpu tests/test_cpu.cpp)
    target_link_libraries(test_cpu PRIVATE arduino3d_emulator)
    add_test(NAME cpu_registers COMMAND test_cpu)
endif()

add_executable(arduino-3d-lab src/main.cpp)
target_link_libraries(arduino-3d-lab PRIVATE arduino3d_core)

if(BUILD_TESTING)
    message(STATUS "CTest habilitado.")
endif()
