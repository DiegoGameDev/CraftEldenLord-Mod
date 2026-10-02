# MinecraftEldenBridge

Milestone 1: DLL Windows x64, logging e leitura unica de selecao Minecraft em JSON.
Projeto experimental offline/modded. Hooks e spawn permanecem TODO/research.

## Transparencia sobre IA

Este projeto foi desenvolvido por Diego Mogger com assistencia de IA via Codex.
A IA ajudou a estruturar o projeto, escrever a base C++/PowerShell, documentar
os limites de seguranca e preparar testes locais. A intencao e deixar explicito
que a experimentacao foi feita com apoio de uma ferramenta acessivel, de forma
honesta e auditavel.

Autor humano: Diego Mogger.
Assistente de IA: OpenAI Codex.

## Inicio rapido

```powershell
Set-Location 'D:\Elden Ring Mod Project IA'
.\tools\build_hook.ps1
Get-Content '.\elden-ring-hooker\build\test-output\valid\MinecraftEldenBridge.log'
```

DLL: `elden-ring-hooker/build/Release/MinecraftEldenBridge.dll`.
Guia de runtime: [hooker README](elden-ring-hooker/README.md).
Compilacao e caminhos: [SETUP_WINDOWS](docs/SETUP_WINDOWS.md).
Arquitetura e limites: [HOOKER_DEVELOPMENT](docs/HOOKER_DEVELOPMENT.md).
Primeiro teste offline no jogo: [FIRST_GAME_TEST](docs/FIRST_GAME_TEST.md).
MVP Minecraft/Fabric: [MINECRAFT_FABRIC_MVP](docs/MINECRAFT_FABRIC_MVP.md).

## Arquivos do projeto

```text
.gitignore
README.md
elden-ring-hooker/
  CMakeLists.txt
  README.md
  src/
    dllmain.cpp
    logger.h
    logger.cpp
    selection_reader.h
    selection_reader.cpp
  tests/
    core_tests.cpp
    smoke_host.cpp
  third_party/
    README.md
    nlohmann/json.hpp
    nlohmann/LICENSE.MIT
shared-formats/
  example_selection.json
  minecraft_selection.schema.json
docs/
  HOOKER_DEVELOPMENT.md
  FIRST_GAME_TEST.md
  MINECRAFT_FABRIC_MVP.md
  SETUP_WINDOWS.md
tools/
  build_hook.ps1
  copy_hook_to_game.ps1
  launch_hook_test.ps1
  write_selection_atomic.ps1
  tests/copy_hook_tests.ps1
```

Builds, binarios, fixtures e logs de verificacao ficam em `elden-ring-hooker/build`.
Nenhuma dependencia existente foi modificada. Nenhum arquivo foi instalado no jogo.

## Validacao local

Ambiente: MSVC x64 do Visual Studio Community 2026, Windows SDK 10.0.26100.0.
CTest cobre o leitor/logger, carregamento da DLL com exemplo valido e arquivo
ausente. `Test-Json` confirmou que o exemplo corresponde ao schema publicado.
O teste de copia usa uma pasta ficticia sob build/test-output e nao abre o jogo:

```powershell
.\tools\tests\copy_hook_tests.ps1 `
  -DllPath 'D:\Elden Ring Mod Project IA\elden-ring-hooker\build\Release\MinecraftEldenBridge.dll'
```

O script de copia usa por padrao a pasta informada do jogo:
`D:\SteamLibrary\steamapps\common\ELDEN RING\Game`.
Simule com `.\tools\copy_hook_to_game.ps1 -WhatIf`.
Use `-GameDirectory` somente para substituir esse destino em outra instalacao.
Para a ponte, ajustar `MEB_SELECTION_JSON` ou colocar `minecraft_selection.json`
ao lado da DLL. Opcionalmente definir `MEB_LOG_DIRECTORY`.
Compatibilidade dentro do jogo ainda nao foi verificada; Fabric e bridge server
sao proximos milestones.
