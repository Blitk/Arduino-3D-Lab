# Arduino 3D Lab

Simulador experimental de Arduino em 3D para Linux, desenvolvido
em C++20.

## Estado atual

A versão inicial contém:

- Estrutura de compilação com CMake e Ninja.
- Aplicação executável em C++20.
- Núcleo da aplicação separado do ponto de entrada.
- Testes automatizados com CTest.

## Requisitos

- CMake 3.20 ou superior.
- Compilador C++ com suporte a C++20.
- Ninja.

No Ubuntu ou Debian, instale os requisitos com:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build
```

## Compilação

Na raiz do projeto:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Execução

```bash
./build/arduino-3d-lab
```

## Testes

```bash
ctest --test-dir build --output-on-failure
```

## Próximas etapas

- Implementar o núcleo do emulador AVR.
- Mapear os registradores GPIO.
- Representar circuitos e componentes virtuais.
- Integrar a renderização OpenGL.

O projeto ainda não oferece emulação AVR nem renderização 3D.
