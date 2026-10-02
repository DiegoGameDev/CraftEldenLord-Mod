# Minecraft Fabric MVP

O mod client-side em `minecraft-fabric-raycast` publica o bloco mirado usando
o contrato v1 consumido pela DLL. Nenhum hook interno ou renderizacao no
Elden Ring foi adicionado nesta etapa.

## Versoes e instancia

Instancia encontrada: `EldenCraftLord Instance`, em
`C:\Users\DiegoMogger\AppData\Roaming\PrismLauncher\instances\EldenCraftLord Instance`.
O `mmc-pack.json` confirma Minecraft 26.3 e Fabric Loader 0.19.5.
O diretorio do jogo desta instancia e `minecraft`, nao `.minecraft`.

Prism configura Fabric Loader ao preparar a instancia. Fabric API e outro
componente: `fabric-api-0.161.0+26.3.jar` vai para `minecraft/mods`, junto com
nosso mod. Nao basta colocar um instalador generico `fabric.jar` ali.

Build fixado: Loom 1.17.21, Gradle 9.6.0, bytecode Java 25. Usa nomes oficiais
nao ofuscados do Minecraft 26.3, sem Yarn/remapeamento antigo.
O JDK 25 extraido em dependencias e Linux ARM64; nao executa no Windows.
O script usa `C:\Program Files\Java\jdk-26.0.2.1` com `--release 25`.
Prism tambem precisa de Java Windows 25+. Nao alteramos sua configuracao Java.

## Compilar e instalar

```powershell
Set-Location 'D:\Elden Ring Mod Project IA'
.\tools\build_fabric.ps1
.\tools\install_fabric_mod.ps1 -WhatIf
# Feche Minecraft nesta instancia antes de instalar/atualizar:
.\tools\install_fabric_mod.ps1
```

Jar: `minecraft-fabric-raycast/build/libs/minecraft-elden-bridge-raycast-0.2.0.jar`.
Nao instalar o `-sources.jar`. O primeiro build baixa dependencias oficiais;
depois use `build_fabric.ps1 -Offline` quando o cache estiver completo.
Cache Gradle: `.gradle-user-home`, ignorado pelo Git.
Outro JDK: `build_fabric.ps1 -JavaHome 'CAMINHO_DO_JDK_WINDOWS'`.

O instalador valida versoes, copia nosso mod como
`minecraft/mods/MinecraftEldenBridge-raycast.jar` e adiciona Fabric API local
somente se ausente. Nao substitui dependencias existentes; uma API diferente
ou segundo jar da ponte causa erro para revisao manual. Nao modifica
`mmc-pack.json`, contas, Java, mundos ou opcoes.
Outra instancia: `install_fabric_mod.ps1 -InstanceDirectory 'CAMINHO_DA_INSTANCIA'`.

## Publicacao

A cada `END_CLIENT_TICK`, usa o raycast vanilla 26.3
`LocalPlayer.raycastHitResult(1.0F, cameraEntity)`. Isso respeita alcance,
colisoes e alvos de entidade, atualizando o alvo sem depender do ultimo frame.
Nao altera `Minecraft.hitResult` nem o estado de renderizacao.
Captura ID pelo registro de blocos, dimensao e coordenadas inteiras do bloco.

Uma thread de I/O recebe snapshots imutaveis e publica a selecao mais recente
a cada 50 ms, somente se diferente da ultima gravada. Mudancas intermediarias
podem ser agrupadas: isto representa estado atual, nao uma fila de eventos.
O JSON UTF-8 sem BOM e fechado em `minecraft_selection.json.tmp` antes de
`Files.move(ATOMIC_MOVE, REPLACE_EXISTING)` no mesmo diretorio.
Nao ha fallback nao atomico. Handles de leitores no Windows podem impedir
a troca temporariamente; o worker faz ate 8 tentativas, separadas por 10 ms.
Falhas persistentes mantem o JSON anterior, geram aviso e tentam novamente
a cada 5 segundos. O log informa a recuperacao. Use disco local com suporte a
substituicao atomica. Atomicidade nao promete durabilidade contra queda de energia.

```json
{
  "schema_version": 1,
  "minecraft_id": "minecraft:stone",
  "dimension": "minecraft:overworld",
  "position": { "x": 128, "y": 64, "z": -32 }
}
```

Sem mundo/jogador, mirando ar ou entidade: nao publica nova selecao. O arquivo
continua com o ultimo bloco valido. Ainda nao existe alvo ausente, timestamp
ou heartbeat: arquivo presente nao comprova que Minecraft esta aberto.
Use uma instancia escritora por caminho. Nao execute simultaneamente
`write_selection_atomic.ps1`, pois ambos usam o mesmo temporario.

Destino padrao: `D:\Elden Ring Mod Project IA\runtime-state\minecraft_selection.json`.
Override opcional: propriedade Java `-Dmeb.selectionJson=CAMINHO_ABSOLUTO`,
depois variavel `MEB_SELECTION_JSON`. A propriedade tem prioridade; o padrao
permite iniciar pelo Prism sem herdar variaveis de PowerShell.
Caminho invalido desabilita a ponte com erro no log Minecraft.

## Segundo plano

O mod nao verifica foco, visibilidade ou eventos de desenho para publicar.
Dentro do mundo, use `F3+P` para desativar a pausa ao perder foco; confirme a
mensagem do proprio Minecraft. Isso permite alternar para Elden Ring sem
pausa automatica do single-player. `Esc` pode pausar explicitamente o mundo.

Minecraft continua sendo um cliente grafico. A janela pode ficar atras do jogo,
mas nao implementamos headless, janela oculta, controle remoto de camera nem
execucao exclusiva de fisica/chunks. Limitadores de FPS, minimizacao, mods de
economia de recursos e pausas podem reduzir a frequencia dos ticks.
O mod nao promete 20 ticks/s quando o cliente estiver suspenso.
Verifique o comportamento real em segundo plano no teste abaixo.

## Teste ponta a ponta

1. Abra a instancia Fabric no Prism e entre num mundo de teste.
2. Mire um bloco ao alcance. No PowerShell, execute:

```powershell
Get-Content 'D:\Elden Ring Mod Project IA\runtime-state\minecraft_selection.json' -Raw
```

3. Mire outro bloco ou outro lugar do mesmo tipo. Repita a leitura e confira
   `minecraft_id`, `dimension` e `position`. Com o mesmo alvo, o timestamp do
   arquivo deve ficar estavel. Mirando ar, o ultimo JSON permanece.
4. Desative a pausa com `F3+P`. Para testar sem foco, observe um bloco alterado
   por um pistao ou mecanismo do mundo enquanto alterna para PowerShell.
   Um alvo estatico nao muda o arquivo, por projeto.
5. Com Steam aberta, inicie Elden Ring pelo `launch_elden_bridge.bat` da raiz,
   no fluxo offline/modded existente. O perfil
   `config/modengine2_hook_test.toml` inclui nossa DLL em `external_dlls`.
6. Confira timestamps novos e os dados do bloco:

```powershell
Get-Content 'D:\Elden Ring Mod Project IA\runtime-logs\game\MinecraftEldenBridge.log' -Tail 20 -Wait
```

Polling da DLL: 333 ms, aproximadamente 20 frames a 60 FPS, e nao um contador
real de frames. Nenhum bloco aparece visualmente no Elden nesta etapa.
O teste do mundo, foco/minimizacao e os dois jogos juntos exige verificacao
interativa; build e testes automatizados nao substituem essa confirmacao.

## Diagnostico e testes locais

Log Minecraft: `minecraft/logs/latest.log` dentro da instancia. Procure
`MinecraftEldenBridge raycast loaded. Selection file:` ou avisos de publicacao.
O jar deve aparecer como `minecraft_elden_bridge` nos mods carregados.
Se a instancia nunca foi iniciada, Prism pode precisar baixar assets,
bibliotecas e Java; isso e independente do build do mod.

`build_fabric.ps1` executa testes JUnit de contrato, deduplicacao, substituicao,
preservacao/retry apos falha e leitura concorrente de snapshots completos.
Relatorio: `minecraft-fabric-raycast/build/reports/tests/test/index.html`.
Uma fixture produzida pelo mesmo escritor Java permite testar a DLL sem jogos:

```powershell
Set-Location 'D:\Elden Ring Mod Project IA'
$previousPoll = $env:MEB_SELECTION_POLL_MS
try {
    $env:MEB_SELECTION_POLL_MS = '0'
    .\elden-ring-hooker\build\Release\bridge_smoke.exe `
      '.\elden-ring-hooker\build\Release\MinecraftEldenBridge.dll' `
      '.\minecraft-fabric-raycast\build\test-output\minecraft_selection.json' `
      '.\runtime-logs\fabric-smoke' 0
} finally {
    $env:MEB_SELECTION_POLL_MS = $previousPoll
}
```

Este smoke valida JSON Java -> parser/DLL C++; nao exercita raycast num mundo.

Validacao desta etapa (2026-10-02): build offline completo com JDK Windows 26,
6 testes JUnit aprovados, fixture Java aceita pelo schema e por `bridge_smoke`
com status 0, e `launch_elden_bridge.bat -CheckOnly` aprovado sem abrir jogos.
Jar 0.2.0 e Fabric API 0.161.0+26.3 instalados na instancia indicada com hashes
SHA-256 conferidos. Nenhuma dependencia preexistente foi substituida.
O teste interativo Minecraft -> JSON -> Elden Ring ainda deve ser executado.

## Referencias e proxima etapa

- [Fabric para Minecraft 26.3](https://www.fabricmc.net/2026/09/15/263.html).
- [Exemplo Fabric 26.3](https://github.com/FabricMC/fabric-example-mod/tree/26.3).
- [Fabric Loom](https://docs.fabricmc.net/develop/loom/).

O metodo de raycast e os nomes usados foram conferidos tambem no jar Minecraft
26.3 do cache local do Prism. Pesquisa de renderizacao, atualizacao do jogador
e integracao interna do Elden Ring permanece TODO/research para a proxima etapa.
Nenhum offset, assinatura ou endereco foi adicionado.
