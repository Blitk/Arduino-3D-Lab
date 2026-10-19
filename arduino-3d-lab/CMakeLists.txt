
cmake_minimum_required(VERSION 3.20)

project(Arduino3DLab
    VERSION 0.1.0
    DESCRIPTION "Simulador experimental de Arduino em 3D"
    LANGUAGES C CXX
)

# ============================================================
# Opções e configuração global
# ============================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

option(ARDUINO3D_BUILD_TESTS "Compilar testes automatizados" ${BUILD_TESTING})
option(ARDUINO3D_ENABLE_IMGUI "Habilitar Dear ImGui" OFF)

# ============================================================
# Dependências do sistema
# ============================================================

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

if(TARGET glfw)
    set(ARDUINO3D_GLFW_TARGET glfw)
elseif(TARGET glfw3::glfw)
    set(ARDUINO3D_GLFW_TARGET glfw3::glfw)
else()
    message(FATAL_ERROR "Não foi encontrado um alvo CMake do GLFW.")
endif()

# ============================================================
# GLAD 2 — integrado ao repositório
# Requer: external/glad
# ============================================================

set(ARDUINO3D_GLAD_DIR "${PROJECT_SOURCE_DIR}/external/glad")

if(NOT EXISTS "${ARDUINO3D_GLAD_DIR}/cmake/CMakeLists.txt")
    message(FATAL_ERROR
        "GLAD 2 não encontrado em external/glad.\n"
        "Execute na raiz do projeto:\n"
        "git clone https://github.com/Dav1dde/glad.git external/glad"
    )
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_subdirectory("${ARDUINO3D_GLAD_DIR}/cmake" glad-cmake)

glad_add_library(glad_gl_core_33
    REPRODUCIBLE
    API gl:core=3.3
)

# ============================================================
# Funções auxiliares
# ============================================================

function(arduino3d_configure_target target)
    target_compile_features(${target} PUBLIC cxx_std_20)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
    endif()
endfunction()

function(arduino3d_set_includes target)
    target_include_directories(${target}
        PUBLIC "${PROJECT_SOURCE_DIR}/src"
    )
endfunction()

# ============================================================
# Circuito lógico
# ============================================================

add_library(arduino3d_circuit
    src/circuit/Circuit.cpp
    src/circuit/Pin.cpp
    src/circuit/Wire.cpp
    src/circuit/Led.cpp
)

arduino3d_set_includes(arduino3d_circuit)
arduino3d_configure_target(arduino3d_circuit)

# ============================================================
# Emulador AVR
# ============================================================

add_library(arduino3d_emulator
    src/emulator/Memory.cpp
    src/emulator/CPU.cpp
    src/emulator/InstructionDecoder.cpp
    src/emulator/AVR.cpp
    src/emulator/GPIO.cpp
    src/emulator/FirmwareLoader.cpp
)

arduino3d_set_includes(arduino3d_emulator)
arduino3d_configure_target(arduino3d_emulator)

target_link_libraries(arduino3d_emulator
    PUBLIC arduino3d_circuit
)

# ============================================================
# Persistência de projetos
# ============================================================

add_library(arduino3d_project
    src/project/Project.cpp
    src/project/Serializer.cpp
)

arduino3d_set_includes(arduino3d_project)
arduino3d_configure_target(arduino3d_project)

target_link_libraries(arduino3d_project
    PUBLIC nlohmann_json::nlohmann_json
)

# ============================================================
# Dear ImGui — opcional
# Requer: external/imgui
# ============================================================

if(ARDUINO3D_ENABLE_IMGUI)
    set(ARDUINO3D_IMGUI_DIR "${PROJECT_SOURCE_DIR}/external/imgui")

    set(ARDUINO3D_IMGUI_SOURCES
        "${ARDUINO3D_IMGUI_DIR}/imgui.cpp"
        "${ARDUINO3D_IMGUI_DIR}/imgui_draw.cpp"
        "${ARDUINO3D_IMGUI_DIR}/imgui_tables.cpp"
        "${ARDUINO3D_IMGUI_DIR}/imgui_widgets.cpp"
        "${ARDUINO3D_IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
        "${ARDUINO3D_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
    )

    foreach(source IN LISTS ARDUINO3D_IMGUI_SOURCES)
        if(NOT EXISTS "${source}")
            message(FATAL_ERROR
                "Arquivo do Dear ImGui não encontrado: ${source}\n"
                "Clone https://github.com/ocornut/imgui.git em external/imgui."
            )
        endif()
    endforeach()

    add_library(arduino3d_imgui STATIC ${ARDUINO3D_IMGUI_SOURCES})

    target_include_directories(arduino3d_imgui
        PUBLIC "${ARDUINO3D_IMGUI_DIR}"
        PRIVATE "${ARDUINO3D_IMGUI_DIR}/backends"
    )

    target_link_libraries(arduino3d_imgui
        PUBLIC
            ${ARDUINO3D_GLFW_TARGET}
            OpenGL::GL
    )

    arduino3d_configure_target(arduino3d_imgui)

    message(STATUS "Dear ImGui habilitado.")
endif()

# ============================================================
# Núcleo gráfico e aplicação
# ============================================================

add_library(arduino3d_core
    src/app/Application.cpp
    src/graphics/Camera.cpp
    src/graphics/Scene.cpp
    src/graphics/Renderer.cpp
)

if(ARDUINO3D_ENABLE_IMGUI)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/src/app/EditorUI.cpp")
        message(FATAL_ERROR
            "ARDUINO3D_ENABLE_IMGUI está ativo, mas "
            "src/app/EditorUI.cpp não existe."
        )
    endif()

    target_sources(arduino3d_core PRIVATE src/app/EditorUI.cpp)

    target_compile_definitions(arduino3d_core
        PUBLIC 
            ARDUINO3D_ENABLE_IMGUI=1
            GLFW_INCLUDE_NONE
    )

    target_link_libraries(arduino3d_core PUBLIC arduino3d_imgui)
endif()

arduino3d_set_includes(arduino3d_core)
arduino3d_configure_target(arduino3d_core)

target_compile_definitions(arduino3d_core
    PRIVATE ARDUINO3D_ASSET_DIR="${PROJECT_SOURCE_DIR}/assets"
)

target_link_libraries(arduino3d_core
    PUBLIC
        ${ARDUINO3D_GLFW_TARGET}
        glad_gl_core_33
        glm::glm
        OpenGL::GL
        arduino3d_circuit
        arduino3d_emulator
        arduino3d_project
)

# ============================================================
# Executável
# ============================================================

add_executable(arduino-3d-lab src/main.cpp)

target_link_libraries(arduino-3d-lab PRIVATE arduino3d_core)
arduino3d_configure_target(arduino-3d-lab)

# ============================================================
# Testes automatizados
# ============================================================

if(ARDUINO3D_BUILD_TESTS)
    function(arduino3d_add_test name source label library)
        if(NOT EXISTS "${PROJECT_SOURCE_DIR}/${source}")
            message(WARNING "Teste ignorado: ${source} não encontrado.")
            return()
        endif()

        add_executable(${name} "${source}")
        target_link_libraries(${name} PRIVATE ${library})
        arduino3d_configure_target(${name})

        add_test(NAME ${label} COMMAND ${name})
    endfunction()

    arduino3d_add_test(
        test_circuit tests/test_circuit.cpp circuit arduino3d_circuit
    )

    arduino3d_add_test(
        test_memory tests/test_memory.cpp memory arduino3d_emulator
    )

    arduino3d_add_test(
        test_cpu tests/test_cpu.cpp cpu_registers arduino3d_emulator
    )

    arduino3d_add_test(
        test_avr tests/test_avr.cpp avr_instructions arduino3d_emulator
    )

    arduino3d_add_test(
        test_gpio tests/test_gpio.cpp gpio_circuit_integration arduino3d_emulator
    )

    arduino3d_add_test(
        test_project_serializer
        tests/test_project_serializer.cpp
        project_serializer
        arduino3d_project
    )

    message(STATUS "Testes automatizados habilitados.")
else()
    message(STATUS "Testes automatizados desabilitados.")
endif()

# ============================================================
# Resumo da configuração
# ============================================================

message(STATUS "========================================")
message(STATUS "Arduino 3D Lab ${PROJECT_VERSION}")
message(STATUS "Dear ImGui: ${ARDUINO3D_ENABLE_IMGUI}")
message(STATUS "Testes: ${ARDUINO3D_BUILD_TESTS}")
message(STATUS "OpenGL: ${OPENGL_FOUND}")
message(STATUS "========================================")
