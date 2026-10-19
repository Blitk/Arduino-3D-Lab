# Arduino 3D Lab — formato JSON de projeto

- **Versão:** 1
- **Identificador:** `arduino-3d-lab-project`
- **Extensão sugerida:** `.a3dl.json`

## Objetivo

O formato armazena os dados necessários para reconstruir um circuito:
nome do projeto, configurações, componentes, transformações 3D, pinos,
propriedades e conexões.

Não armazena estado transitório da GPU, como IDs de objetos OpenGL,
VAOs, VBOs, texturas, ponteiros, buffers de renderização ou caches.

## Exemplo

~~~json
{
  "format": "arduino-3d-lab-project",
  "format_version": 1,
  "name": "Meu circuito",
  "settings": {
    "board": "arduino-uno",
    "cpu_frequency_hz": 16000000,
    "grid_size": 1,
    "snap_to_grid": true
  },
  "components": [
    {
      "id": "led-1",
      "type": "led",
      "name": "LED vermelho",
      "position": {"x": 1, "y": 0, "z": 2},
      "orientation": {"x": 0, "y": 0, "z": 0, "w": 1},
      "pins": ["anode", "cathode"],
      "properties": {
        "color": "#ff0000",
        "forward_voltage": 2.0
      }
    },
    {
      "id": "board-1",
      "type": "arduino-uno",
      "name": "Arduino Uno",
      "position": {"x": 0, "y": 0, "z": 0},
      "orientation": {"x": 0, "y": 0, "z": 0, "w": 1},
      "pins": ["D2", "GND"],
      "properties": {}
    }
  ],
  "connections": [
    {
      "from": {"component": "board-1", "pin": "D2"},
      "to": {"component": "led-1", "pin": "anode"}
    }
  ]
}
~~~

## Campos principais

- `format`: identificador fixo do formato.
- `format_version`: versão inteira do esquema.
- `name`: nome do projeto, com até 512 caracteres.
- `settings.board`: identificador da placa.
- `settings.cpu_frequency_hz`: frequência da CPU em hertz.
- `settings.grid_size`: tamanho da grade.
- `settings.snap_to_grid`: ativa ou desativa o encaixe na grade.
- `components`: lista de componentes.
- `components[].id`: identificador único no projeto.
- `components[].type`: tipo lógico do componente.
- `components[].name`: nome de exibição.
- `components[].position`: posição cartesiana x, y, z.
- `components[].orientation`: quaternion x, y, z, w.
- `components[].pins`: nomes lógicos dos pinos.
- `components[].properties`: objeto JSON com propriedades específicas.
- `connections`: conexões entre pinos de componentes.
- `connections[].from` e `connections[].to`: cada endpoint contém
  `component` e `pin`.

## Validação

A implementação rejeita:

- Formato ou versão não suportados.
- IDs de componente duplicados.
- Componentes ou pinos referenciados que não existem.
- Conexões duplicadas.
- Posições, orientações e configurações numéricas inválidas.
- Quaternions com norma zero.
- Propriedades que não sejam objetos JSON.
- Mais de 10.000 componentes.
- Mais de 50.000 conexões.
- Mais de 256 pinos por componente.
- Arquivos maiores que 10 MiB durante o carregamento.

O carregamento só substitui o projeto de destino depois que o JSON
é analisado e todos os dados são validados. Em caso de erro, a função
retorna `false` e pode preencher uma mensagem de erro.

## Evolução do formato

Ao alterar a estrutura de persistência, incremente `format_version`
e implemente uma migração explícita ou rejeite versões desconhecidas.

Não use a posição do componente no array como identificador permanente.
Use o campo `id` para reconstruir conexões.

Dados transitórios de renderização devem ser reconstruídos pela aplicação
e nunca tratados como parte obrigatória deste formato.
