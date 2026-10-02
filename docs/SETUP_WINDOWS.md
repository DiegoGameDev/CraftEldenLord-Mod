# Setup Windows

## Ambiente encontrado

Projeto: `D:\Elden Ring Mod Project IA` (inicialmente vazio).
Dependencias: `C:\Users\DiegoMogger\Downloads\install mods\elden e mine mod dependences`.

- EldenModLoader-117-3-2-1662569069: dinput8.dll e mod_loader_config.ini.
- MinHook_134_bin: header, DLLs e import libraries x86/x64.
- ghidra_12.1.4_PUBLIC, JDK 25, Smithbox e fabric-api-0.161.0+26.3.jar.
- PrismLauncher em `%APPDATA%\PrismLauncher`; configuracoes e contas nao alteradas.
- Visual Studio Community 2026, MSVC 14.51 x64 e CMake 4.3.1-msvc1 encontrados.
- MinGW 6.3 no PATH e x86; nao usar para esta DLL x64.

## Build

Requisitos: Windows x64, Visual Studio 2022/2026 com workload Desktop development
with C++, Windows SDK e CMake. Use o script que localiza o CMake incluido no VS:

```powershell
Set-Location 'D:\Elden Ring Mod Project IA'
.\tools\build_hook.ps1
```

Alternativa em Developer PowerShell com cmake/ctest no PATH:

```powershell
cmake -S elden-ring-hooker -B elden-ring-hooker/build -G 'Visual Studio 18 2026' -A x64
cmake --build elden-ring-hooker/build --config Release --parallel 1
ctest --test-dir elden-ring-hooker/build -C Release --output-on-failure
```

Para VS 2022, use `Visual Studio 17 2022` e outro diretorio de build se ja houver
cache do VS 2026. A compilacao usa CRT estatico e nlohmann/json local; MinHook e
ferramentas de pesquisa nao sao dependencias de build neste milestone.
O script usa um processo de build por vez. Em PowerShell 7, tambem normaliza
o ambiente dos subprocessos para evitar o erro MSBuild de chaves PATH/Path duplicadas.

## Testar o log sem jogo

O script build_hook ja executa os testes. Abra:

```powershell
Get-Content '.\elden-ring-hooker\build\test-output\valid\MinecraftEldenBridge.log'
```

O log deve conter:

```text
MinecraftEldenBridge hook loaded.
minecraft_id=minecraft:stone dimension=minecraft:overworld position=(x=128, y=64, z=-32)
Initialization complete.
```

O teste `missing` deve registrar `Selection unavailable:` sem derrubar o host.
Consulte o README do hooker para executar o host manualmente.

## Copia futura para o jogo

Com Mod Engine 2, use o [primeiro teste no jogo](FIRST_GAME_TEST.md), que carrega
a DLL direto do projeto. A copia abaixo e uma alternativa para EldenModLoader;
nao e uma etapa necessaria do teste com Mod Engine 2.

A pasta informada para `eldenring.exe` e
`D:\SteamLibrary\steamapps\common\ELDEN RING\Game`.
Esse e o destino padrao do script. Com o jogo fechado, primeiro simule:

```powershell
.\tools\copy_hook_to_game.ps1 -WhatIf
```

Para outra instalacao, informe `-GameDirectory 'CAMINHO_ABSOLUTO_DA_PASTA_GAME'`.
Remova `-WhatIf` quando o destino estiver correto. A copia vai para
`Game\mods\MinecraftEldenBridge.dll`; um arquivo existente exige `-Overwrite`.
O script copia somente esta DLL, nao instala o loader e nao modifica configuracoes.
Falhas de permissao sao reportadas; o script nao tenta elevar privilegios sozinho.
Se Codex precisar executar essa copia em pasta protegida, deve pedir aprovacao
com justificativa via require_escalated.

O EldenModLoader carrega as DLLs da pasta `mods`, conforme seu README oficial:
https://github.com/techiew/EldenRingModLoader
Verifique a configuracao do loader para sua instalacao offline/modded existente.
Este projeto nao instala, desativa nem burla Easy Anti-Cheat. Nao execute o
experimento em uma sessao protegida/online.

Para o JSON, escolha um dos caminhos:

- Coloque uma copia de `shared-formats\example_selection.json` ao lado da DLL,
  com nome `minecraft_selection.json`.
- Defina `MEB_SELECTION_JSON` com o caminho absoluto do arquivo compartilhado,
  no ambiente que o processo do jogo realmente herda.

Para o segundo caso, exemplo de configuracao da sessao PowerShell:

```powershell
$env:MEB_SELECTION_JSON = 'D:\Elden Ring Mod Project IA\shared-formats\example_selection.json'
$env:MEB_LOG_DIRECTORY = 'D:\Elden Ring Mod Project IA\runtime-logs'
```

Launchers ja abertos podem nao herdar essas variaveis. O padrao ao lado da DLL
evita essa dependencia. Log padrao: `Game\mods\MinecraftEldenBridge.log`;
fallback: `%TEMP%\MinecraftEldenBridge\MinecraftEldenBridge.log`.
A DLL permanece carregada ate encerrar o processo. Sem `MEB_SELECTION_POLL_MS`,
le uma vez; o launcher do projeto define 333 ms para acompanhar mudancas.
Reinicie para trocar o binario. Nenhum arquivo foi copiado para o jogo durante
a preparacao desta etapa.

## Mod Fabric

Use `tools/build_fabric.ps1` e `tools/install_fabric_mod.ps1`.
O JDK 25 extraido nas dependencias e Linux ARM64; para compilar no Windows o
script usa `C:\Program Files\Java\jdk-26.0.2.1`, gerando bytecode Java 25.
Outro JDK Windows 25+ pode ser informado com `-JavaHome`.
Detalhes: [MINECRAFT_FABRIC_MVP](MINECRAFT_FABRIC_MVP.md).

Se PowerShell bloquear scripts locais, examine `Get-ExecutionPolicy -List`.
Uma sessao pode usar `Set-ExecutionPolicy -Scope Process -ExecutionPolicy RemoteSigned`;
isso termina ao fechar PowerShell. O `.bat` ja passa `-ExecutionPolicy RemoteSigned`
ao seu processo PowerShell para executar estes scripts locais. Nao modifica
CurrentUser ou LocalMachine. Politicas impostas por administrador prevalecem.
