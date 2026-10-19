cmake_minimum_required(VERSION 3.20)

project(Arduino3DLab
VERSION 0.1.0
DESCRIPTION “Simulador experimental de Arduino em 3D”
LANGUAGES C CXX
)

============================================================

Configuração global

============================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

option(ARDUINO3D_BUILD_TESTS “Compilar os testes automatizados” ${BUILD_TESTING})
option(ARDUINO3D_ENABLE_IMGUI “Habilitar a interface Dear ImGui” OFF)

============================================================

Dependências gráficas

============================================================

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

if(TARGET glfw)
set(ARDUINO3D_GLFW_TARGET glfw)
elseif(TARGET glfw3::glfw)
set(ARDUINO3D_GLFW_TARGET glfw3::glfw)
else()
message(FATAL_ERROR “Alvo CMake do GLFW não encontrado.”)
endif()

foreach(required_target IN ITEMS glad::glad glm::glm)
if(NOT TARGET ${required_target})
message(FATAL_ERROR
“Dependência sem o alvo CMake esperado: ${required_target}”
)
endif()
endforeach()

============================================================

Configuração comum dos alvos

============================================================

function(arduino3d_configure_target target_name)
target_compile_features(${target_name} PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target_name} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

endfunction()

function(arduino3d_set_project_includes target_name)
target_include_directories(${target_name}
PUBLIC
${PROJECT_SOURCE_DIR}/src
)
endfunction()

============================================================

Circuito lógico

Independente de OpenGL e do emulador AVR

============================================================

add_library(arduino3d_circuit
src/circuit/Circuit.cpp
src/circuit/Pin.cpp
src/circuit/Wire.cpp
src/circuit/Led.cpp
)

arduino3d_set_project_includes(arduino3d_circuit)
arduino3d_configure_target(arduino3d_circuit)

============================================================

Emulador AVR e GPIO

============================================================

add_library(arduino3d_emulator
src/emulator/Memory.cpp
src/emulator/CPU.cpp
src/emulator/InstructionDecoder.cpp
src/emulator/AVR.cpp
src/emulator/GPIO.cpp
src/emulator/FirmwareLoader.cpp
)

arduino3d_set_project_includes(arduino3d_emulator)
arduino3d_configure_target(arduino3d_emulator)

target_link_libraries(arduino3d_emulator
PUBLIC
arduino3d_circuit
)

============================================================

Interface gráfica

============================================================

set(ARDUINO3D_GRAPHICS_SOURCES
src/graphics/Camera.cpp
src/graphics/Scene.cpp
src/graphics/Renderer.cpp
)

set(ARDUINO3D_APP_SOURCES
src/app/Application.cpp
${ARDUINO3D_GRAPHICS_SOURCES}
)

if(ARDUINO3D_ENABLE_IMGUI)
# O pacote precisa fornecer o alvo imgui::imgui.
# Os backends GLFW/OpenGL3 também precisam estar compilados
# e disponíveis para o linker.
find_package(imgui CONFIG REQUIRED)

if(NOT TARGET imgui::imgui)
    message(FATAL_ERROR
        "ARDUINO3D_ENABLE_IMGUI está ativo, mas "
        "o alvo imgui::imgui não foi encontrado."
    )
endif()
list(APPEND ARDUINO3D_APP_SOURCES
    src/app/EditorUI.cpp
)

endif()

add_library(arduino3d_core
${ARDUINO3D_APP_SOURCES}
)

arduino3d_set_project_includes(arduino3d_core)
arduino3d_configure_target(arduino3d_core)

target_compile_definitions(arduino3d_core
PRIVATE
ARDUINO3D_ASSET_DIR=”${PROJECT_SOURCE_DIR}/assets”
)

target_link_libraries(arduino3d_core
PUBLIC
${ARDUINO3D_GLFW_TARGET}
glad::glad
glm::glm
OpenGL::GL
arduino3d_circuit
arduino3d_emulator
)

if(ARDUINO3D_ENABLE_IMGUI)
target_link_libraries(arduino3d_core
PUBLIC
imgui::imgui
)
endif()

============================================================

Executável

============================================================

add_executable(arduino-3d-lab
src/main.cpp
)

target_link_libraries(arduino-3d-lab
PRIVATE
arduino3d_core
)

============================================================

Testes automatizados

============================================================

if(ARDUINO3D_BUILD_TESTS)
enable_testing()

function(arduino3d_add_test test_name source_file test_label)
    add_executable(${test_name}
        ${source_file}
    )
    target_link_libraries(${test_name}
        PRIVATE
            arduino3d_emulator
    )
    arduino3d_configure_target(${test_name})
    add_test(
        NAME ${test_label}
        COMMAND ${test_name}
    )
endfunction()
add_executable(test_circuit
    tests/test_circuit.cpp
)
target_link_libraries(test_circuit
    PRIVATE
        arduino3d_circuit
)
arduino3d_configure_target(test_circuit)
add_test(
    NAME circuit
    COMMAND test_circuit
)
arduino3d_add_test(
    test_memory
    tests/test_memory.cpp
    memory
)
arduino3d_add_test(
    test_cpu
    tests/test_cpu.cpp
    cpu_registers
)
arduino3d_add_test(
    test_avr
    tests/test_avr.cpp
    avr_instructions
)
arduino3d_add_test(
    test_gpio
    tests/test_gpio.cpp
    gpio_circuit_integration
)
message(STATUS "Testes automatizados habilitados.")

endif()
