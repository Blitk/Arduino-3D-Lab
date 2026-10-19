Arduino 3D Lab

Um simulador experimental de Arduino em 3D, desenvolvido em C++, com emulação AVR e simulação de circuitos eletrônicos.

O Arduino 3D Lab é um projeto open source em desenvolvimento que busca unir renderização 3D, programação embarcada e emulação de hardware em uma aplicação desktop para Linux.

A proposta é permitir que o usuário monte circuitos virtuais, carregue firmware compilado para microcontroladores AVR e observe os efeitos da execução do programa nos componentes simulados.

Objetivos

* Criar um ambiente 3D para visualizar placas, componentes e conexões eletrônicas.
* Desenvolver um emulador AVR próprio, começando pelo ATmega328P.
* Executar firmware compilado para AVR, em vez de apenas interpretar o código-fonte Arduino.
* Simular pinos digitais e propagar seus estados pelo circuito.
* Representar visualmente o comportamento de componentes, começando por LEDs.
* Salvar e carregar projetos em um formato estruturado.
* Construir uma arquitetura modular, testável e extensível.

Status: projeto experimental. A arquitetura e os recursos descritos representam o planejamento de desenvolvimento; a compatibilidade com instruções AVR, periféricos e bibliotecas Arduino dependerá da implementação de cada componente.

Tecnologias planejadas

Tecnologia	Finalidade
C++20	Linguagem principal
CMake	Configuração e geração do build
Ninja	Sistema de build
OpenGL	Renderização 3D
GLFW	Janela e entrada do usuário
GLAD	Carregamento de funções OpenGL
GLM	Matemática gráfica e transformações
Dear ImGui	Interface de usuário e ferramentas
nlohmann/json	Serialização de projetos
avr-g++ / avr-libc	Compilação do firmware para AVR
CTest	Integração dos testes automatizados

As dependências definitivas serão ajustadas conforme a implementação evoluir.

Estrutura do projeto

arduino-3d-lab/
├── CMakeLists.txt
├── README.md
├── assets/
│   ├── models/
│   └── shaders/
├── firmware/
│   ├── blink.cpp
│   └── include/
│       └── Arduino.h
├── src/
│   ├── main.cpp
│   ├── app/
│   │   ├── Application.cpp
│   │   └── EditorUI.cpp
│   ├── graphics/
│   │   ├── Renderer.cpp
│   │   ├── Camera.cpp
│   │   └── Scene.cpp
│   ├── circuit/
│   │   ├── Circuit.cpp
│   │   ├── Pin.cpp
│   │   ├── Wire.cpp
│   │   └── Led.cpp
│   ├── emulator/
│   │   ├── AVR.cpp
│   │   ├── Memory.cpp
│   │   ├── InstructionDecoder.cpp
│   │   └── GPIO.cpp
│   └── project/
│       ├── Project.cpp
│       └── Serializer.cpp
└── tests/
    ├── test_memory.cpp
    ├── test_gpio.cpp
    └── test_circuit.cpp

Responsabilidade dos módulos

app/ — Aplicação e interface

* Application.cpp: inicialização, ciclo principal, atualização dos módulos e encerramento.
* EditorUI.cpp: interface do simulador, controles de execução e ferramentas de edição.

graphics/ — Renderização 3D

* Renderer.cpp: inicialização do OpenGL e renderização dos objetos.
* Camera.cpp: posição, orientação e movimentação da câmera.
* Scene.cpp: gerenciamento dos objetos presentes na cena.

Os recursos gráficos, como modelos e shaders, ficam no diretório assets/.

circuit/ — Simulação eletrônica

* Circuit.cpp: gerenciamento do circuito e das conexões.
* Pin.cpp: representação dos pinos dos componentes.
* Wire.cpp: conexões elétricas entre pinos.
* Led.cpp: estado lógico e representação do LED.

Este módulo deve determinar como as alterações nos pinos afetam os componentes conectados.

emulator/ — Emulação AVR

* AVR.cpp: estado do microcontrolador e execução das instruções.
* Memory.cpp: memória de programa, registradores e espaços de memória necessários.
* InstructionDecoder.cpp: decodificação e execução das instruções AVR implementadas.
* GPIO.cpp: controle dos registradores e estados de entrada e saída digital.

A implementação começará com um subconjunto de instruções necessário para executar um firmware simples. A compatibilidade será ampliada progressivamente, sem presumir suporte completo ao conjunto AVR ou ao ecossistema Arduino.

project/ — Gerenciamento de projetos

* Project.cpp: representação dos dados de um projeto.
* Serializer.cpp: serialização e desserialização, inicialmente utilizando JSON.

O objetivo é preservar os componentes, suas propriedades, posições e conexões para permitir que um circuito seja salvo e restaurado.

firmware/ — Firmware de exemplo

* blink.cpp: programa inicial para testar a execução do firmware.
* include/Arduino.h: interface mínima de compatibilidade com funções Arduino que forem implementadas.

Esse cabeçalho não representa, por si só, uma implementação completa da API Arduino. As funções utilizadas pelo firmware deverão ter comportamento definido e compatível com os recursos existentes no simulador.

tests/ — Testes automatizados

* test_memory.cpp: testes das operações de memória.
* test_gpio.cpp: testes dos registradores e estados dos pinos.
* test_circuit.cpp: testes das conexões e da propagação de estados pelo circuito.

Os testes deverão validar os módulos independentemente da interface gráfica sempre que possível.

Como os módulos se conectam

O fluxo principal planejado é:

firmware/blink.cpp
        |
        v
Compilador AVR
        |
        v
Firmware compilado (ELF)
        |
        v
Emulador AVR
        |
        v
Registradores GPIO
        |
        v
Pinos do circuito
        |
        v
Conexões e componentes
        |
        v
Estado visual do LED em 3D

O compilador transforma o firmware em código de máquina para a arquitetura AVR. O emulador executa as instruções suportadas, e as alterações nos registradores GPIO são refletidas no circuito virtual.

A renderização 3D apresenta o estado resultante. Ela não deve ser responsável por determinar a lógica elétrica ou a execução das instruções.

Primeiro objetivo: Blink

O primeiro marco funcional será executar um firmware equivalente ao exemplo abaixo e observar um LED virtual alternando entre ligado e desligado.

#include <Arduino.h>
constexpr int LED_PIN = 13;
int main() {
    pinMode(LED_PIN, OUTPUT);
    while (true) {
        digitalWrite(LED_PIN, HIGH);
        delay(1000);
        digitalWrite(LED_PIN, LOW);
        delay(1000);
    }
    return 0;
}

O exemplo ilustra o comportamento desejado. Para executá-lo, o projeto precisará implementar ou fornecer as funções utilizadas, além das instruções AVR e dos mecanismos de temporização necessários.

O primeiro marco não exige compatibilidade integral com o Arduino. Exige uma cadeia funcional mínima, com limitações documentadas e testes reproduzíveis.

Plano de desenvolvimento

1. Estrutura inicial: configurar CMake, dependências, diretórios e aplicação mínima.
2. Janela 3D: criar a janela OpenGL, a câmera e a renderização básica.
3. Componentes visuais: representar placa, protoboard, fios e LED.
4. Circuito lógico: implementar pinos, conexões e propagação de estados.
5. GPIO AVR: modelar os registradores de entrada, saída e direção dos pinos.
6. Emulador AVR: implementar memória, registradores, flags e instruções necessárias.
7. Carregamento de firmware: integrar o compilador AVR e carregar os artefatos compilados.
8. Temporização: estabelecer a relação entre ciclos de CPU, periféricos e tempo simulado.
9. Integração: conectar o emulador ao circuito e atualizar os componentes 3D.
10. Editor e projetos: adicionar edição de circuitos e persistência JSON.
11. Testes e depuração: validar os módulos e criar ferramentas para inspecionar a execução.

Cada etapa deve produzir uma versão compilável e verificável antes da introdução de novas funcionalidades.

Compilação

Em sistemas baseados em Debian ou Ubuntu, as ferramentas iniciais podem ser instaladas com:

sudo apt update
sudo apt install \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config \
    libglfw3-dev \
    libgl1-mesa-dev \
    libglm-dev \
    gcc-avr \
    avr-libc \
    binutils-avr

O GLAD, o Dear ImGui e o nlohmann/json também precisarão estar disponíveis conforme a estratégia de dependências adotada pelo CMakeLists.txt.

Depois de configurar o projeto e suas dependências:

cmake -S . -B build -G Ninja
cmake --build build

Para executar os testes registrados no CTest:

ctest --test-dir build --output-on-failure

Observação: os comandos de compilação só funcionarão quando o CMakeLists.txt e os arquivos necessários tiverem sido implementados. A instalação das ferramentas, isoladamente, não torna o simulador funcional.

Princípios de desenvolvimento

* C++ moderno: usar recursos do C++20 quando forem úteis e manter responsabilidades claras.
* Modularidade: separar renderização, emulação, circuito e persistência.
* Testabilidade: manter as regras essenciais independentes da interface gráfica.
* Precisão documentada: diferenciar recursos implementados, limitações conhecidas e funcionalidades planejadas.
* Sem dependência de Docker: permitir desenvolvimento e compilação diretamente no Linux.
* Evolução incremental: priorizar uma cadeia funcional pequena antes de expandir o suporte a hardware.
* Reprodutibilidade: documentar as ferramentas, os comandos e os resultados esperados.

Limitações previstas

A primeira versão não deve prometer:

* Compatibilidade completa com todas as placas Arduino.
* Suporte a todas as instruções e periféricos AVR.
* Simulação elétrica analógica precisa.
* Compatibilidade automática com qualquer biblioteca Arduino.
* Precisão temporal de hardware real.

Esses recursos poderão ser considerados conforme a arquitetura e os testes amadurecerem.

Contribuições

Contribuições podem incluir melhorias no emulador, componentes virtuais, renderização, ferramentas de depuração, testes e documentação.

Antes de contribuir, consulte as instruções de compilação e verifique quais funcionalidades estão efetivamente implementadas.

Licença

A licença ainda precisa ser definida. Uma licença deverá ser escolhida antes da distribuição formal do projeto, levando em consideração o código próprio e as dependências utilizadas.
