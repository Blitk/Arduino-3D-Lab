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
# ============================================================
# Dependencias gráficas
# ============================================================
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
# ============================================================
# Opcoes comuns de compilacao
# ============================================================
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
# ============================================================
# Circuito logico: independente do OpenGL
# ============================================================
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
arduino3d_configure_target(arduino3d_circuit)
# ============================================================
# Emulador AVR + GPIO
# ============================================================
add_library(arduino3d_emulator
    src/emulator/Memory.cpp
    src/emulator/CPU.cpp
    src/emulator/InstructionDecoder.cpp
    src/emulator/AVR.cpp
    src/emulator/GPIO.cpp
)
target_include_directories(arduino3d_emulator
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
# O GPIO conecta a CPU ao circuito logico, nunca ao renderizador.
target_link_libraries(arduino3d_emulator
    PUBLIC
        arduino3d_circuit
)
arduino3d_configure_target(arduino3d_emulator)
# ============================================================
# Aplicacao grafica
# ============================================================
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
target_compile_definitions(arduino3d_core PRIVATE
    ARDUINO3D_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
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
arduino3d_configure_target(arduino3d_core)
add_executable(arduino-3d-lab
    src/main.cpp
)
target_link_libraries(arduino-3d-lab
    PRIVATE
        arduino3d_core
)
# ============================================================
# Testes automatizados
# ============================================================
if(BUILD_TESTING)
    # Circuito logico
    add_executable(test_circuit
        tests/test_circuit.cpp
    )
    target_link_libraries(test_circuit
        PRIVATE arduino3d_circuit
    )
    add_test(
        NAME circuit
        COMMAND test_circuit
    )
    # Memoria AVR
    add_executable(test_memory
        tests/test_memory.cpp
    )
    target_link_libraries(test_memory
        PRIVATE arduino3d_emulator
    )
    add_test(
        NAME memory
        COMMAND test_memory
    )
    # Estado da CPU e registradores
    add_executable(test_cpu
        tests/test_cpu.cpp
    )
    target_link_libraries(test_cpu
        PRIVATE arduino3d_emulator
    )
    add_test(
        NAME cpu_registers
        COMMAND test_cpu
    )
    # Decodificacao e execucao de instrucoes AVR
    add_executable(test_avr
        tests/test_avr.cpp
    )
    target_link_libraries(test_avr
        PRIVATE arduino3d_emulator
    )
    add_test(
        NAME avr_instructions
        COMMAND test_avr
    )
    # Integracao GPIO -> circuito -> LED
    add_executable(test_gpio
        tests/test_gpio.cpp
    )
    target_link_libraries(test_gpio
        PRIVATE arduino3d_emulator
    )
    add_test(
        NAME gpio_circuit_integration
        COMMAND test_gpio
    )
    message(STATUS "CTest habilitado.")
endif()
