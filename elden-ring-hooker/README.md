# MinecraftEldenBridge: hooker, milestone 1

DLL Windows x64 / C++17 para uso experimental offline/modded.
Ao carregar, uma thread grava `MinecraftEldenBridge hook loaded.`, le uma
selecao JSON e registra `minecraft_id`, `dimension` e `position`.
Nao existe spawn, acesso a memoria do jogo, offset, assinatura ou hook ativo.

## Compilar e testar

No PowerShell:

```powershell
Set-Location 'D:\Elden Ring Mod Project IA'
.\tools\build_hook.ps1
```

O script localiza Visual Studio 2022/2026 com C++ x64 e CMake, compila Release
e executa CTest. Saida: `build\Release\MinecraftEldenBridge.dll` nesta pasta.
`nlohmann/json` 3.11.3 esta incluido com licenca MIT; build nao precisa de rede.

Teste manual, na raiz do projeto, depois do build:

```powershell
& '.\elden-ring-hooker\build\Release\bridge_smoke.exe' `
  '.\elden-ring-hooker\build\Release\MinecraftEldenBridge.dll' `
  '.\shared-formats\example_selection.json' `
  '.\elden-ring-hooker\build\manual-test' 0
Get-Content '.\elden-ring-hooker\build\manual-test\MinecraftEldenBridge.log'
```

O host de teste usa LoadLibrary no proprio processo e aguarda a thread.
Ele recria apenas o log no diretorio de teste indicado. Nao abre o jogo.

## Configuracao em runtime

| Variavel de ambiente | Padrao |
| --- | --- |
| `MEB_SELECTION_JSON` | `minecraft_selection.json` ao lado da DLL |
| `MEB_LOG_DIRECTORY` | Pasta da DLL |

Overrides precisam ser caminhos absolutos. Configure-os no ambiente herdado
pelo processo que carrega a DLL. Um launcher ja aberto pode nao herdar mudancas.
O log chama-se sempre `MinecraftEldenBridge.log`; se o destino inicial falhar,
o fallback e `%TEMP%\MinecraftEldenBridge\MinecraftEldenBridge.log`.
Se ambos falharem, a DLL emite diagnostico com OutputDebugString e encerra a inicializacao.

O JSON e lido uma vez. Arquivo ausente/invalido produz WARN e encerra a thread.
Para outra leitura, reinicie o host/jogo. A DLL fica fixada ate o processo terminar;
FreeLibrary nao a descarrega. Feche o processo antes de substituir a DLL.

Leia [SETUP_WINDOWS](../docs/SETUP_WINDOWS.md) para deploy manual e
[HOOKER_DEVELOPMENT](../docs/HOOKER_DEVELOPMENT.md) para limites e proximos passos.
