# Primeiro teste no jogo com Mod Engine 2

Configuracao separada: `config/modengine2_hook_test.toml`.
Ela carrega a DLL Release diretamente do projeto usando external_dlls.
Nao e preciso instalar EldenModLoader ou copiar a DLL para Game/mods neste fluxo.
O perfil nao carrega assets extras e mantem ScyllaHide desabilitado.
Arquivos e mods que ja estejam instalados no jogo nao sao removidos pelo perfil.

Atalho: abra `launch_elden_bridge.bat` na raiz do projeto. Ele chama o mesmo
script abaixo, que configura `MEB_SELECTION_JSON`, `MEB_LOG_DIRECTORY` e
`MEB_SELECTION_POLL_MS` somente no processo de lancamento. Nao exige digitar
essas variaveis manualmente. O `.bat` usa `RemoteSigned` apenas no processo
PowerShell iniciado; nao modifica a politica persistente de scripts.

1. Deixe a Steam aberta e feche qualquer instancia do Elden Ring.
2. Abra PowerShell na raiz do projeto e execute:

```powershell
Set-Location 'D:\Elden Ring Mod Project IA'
.\tools\launch_hook_test.ps1
```

3. Aguarde o menu principal. Confirme que a sessao esta offline; nao tente conectar
   aos servidores. Nao e necessario carregar um save para testar esta DLL.
4. Em outra janela PowerShell, leia o log:

```powershell
Get-Content 'D:\Elden Ring Mod Project IA\runtime-logs\game\MinecraftEldenBridge.log' -Tail 20
```

Procure um timestamp novo e as mensagens:

```text
MinecraftEldenBridge hook loaded.
minecraft_id=... dimension=... position=(x=..., y=..., z=...)
Selection polling active. interval_ms=333 note=20_frames_at_60fps_is_about_333ms
```

O teste confirma carregamento e leitura do JSON vivo em
`runtime-state/minecraft_selection.json`. Nada deve aparecer ou ser criado no
mundo do jogo neste milestone. Minecraft nao precisa estar aberto.
Feche o jogo antes de recompilar; a DLL permanece fixada ate o processo terminar.

## Teste de escrita atomica enquanto o jogo esta aberto

Com o Elden Ring aberto pelo script, execute em outra janela:

```powershell
Set-Location 'D:\Elden Ring Mod Project IA'
.\tools\write_selection_atomic.ps1 -MinecraftId 'minecraft:gold_block' -X 12 -Y 72 -Z -8
```

Depois confira o log:

```powershell
Get-Content 'D:\Elden Ring Mod Project IA\runtime-logs\game\MinecraftEldenBridge.log' -Tail 20
```

O hook deve registrar uma nova linha com `minecraft:gold_block`. O escritor usa
arquivo temporario e renomeacao para evitar JSON parcial.

## Diagnostico

`launch_hook_test.ps1 -CheckOnly` valida os caminhos sem abrir o jogo.
Se a DLL estiver ausente, execute `tools/build_hook.ps1`.
Se o launcher fechar sem abrir o jogo, execute pelo PowerShell e confira seu erro.
Os logs do Mod Engine ficam na pasta de dependencias, em
`mod wngine 2/modengine2/logs`. Fallback do log do hook:
`%TEMP%/MinecraftEldenBridge/MinecraftEldenBridge.log`.
Um retorno zero do launcher nao comprova carregamento do hook; confira o log.

O script usa o launcher fornecido pelo Mod Engine 2 para o fluxo offline/modded.
Nao usa `offline_CheatEngine.bat`, nao encerra processos de anti-cheat e nao
modifica `start_protected_game.exe`. Nao abra o jogo pelo botao Jogar da Steam
para este teste, pois isso nao seleciona este perfil.

Os caminhos reais estao em launch_hook_test.ps1. O TOML contem o caminho absoluto
da DLL; atualize-o tambem se mover o projeto. O launchmod_eldenring.bat original
continua usando o config_eldenring.toml original, que tem external_dlls vazio.

Referencias: README.txt distribuido com Mod Engine 2 e
https://github.com/soulsmods/ModEngine2/blob/main/launcher/launcher.cpp

O usuario confirmou o carregamento e a leitura de mudancas no jogo no milestone 1.
Para dados reais do Minecraft, veja [MINECRAFT_FABRIC_MVP](MINECRAFT_FABRIC_MVP.md).
