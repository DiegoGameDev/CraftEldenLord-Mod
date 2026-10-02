# Minecraft Fabric MVP

Objetivo do proximo lado Minecraft: publicar o bloco visto pelo raycast em
`runtime-state/minecraft_selection.json` usando a mesma escrita atomica ja
testada pelo simulador PowerShell.

## Estado atual

- O hooker do Elden Ring le `runtime-state/minecraft_selection.json`.
- O launcher do teste configura `MEB_SELECTION_POLL_MS=333`, equivalente a cerca
  de 20 frames em 60 FPS.
- `tools/write_selection_atomic.ps1` simula o futuro escritor Minecraft.
- A DLL apenas loga mudancas. Ela ainda nao renderiza nada dentro do Elden Ring.

## Contrato do JSON

```json
{
  "schema_version": 1,
  "minecraft_id": "minecraft:stone",
  "dimension": "minecraft:overworld",
  "position": { "x": 128, "y": 64, "z": -32 }
}
```

O escritor deve criar um arquivo temporario no mesmo diretorio e renomear para
`minecraft_selection.json` depois da escrita completa.

## Prism e Fabric

O jar disponivel em dependencias e `fabric-api-0.161.0+26.3.jar`. Seu metadata
declara:

- `minecraft`: `~26.3-`
- `fabricloader`: `>=0.19.3`
- `java`: `>=25`

Antes de automatizar uma instancia do Prism, precisamos confirmar que existe
localmente uma instancia Fabric 26.3 funcional ou que os metadados/loader estao
no cache do Prism. A pasta `PrismLauncher/instances` nao listou instancia nesta
inspecao.

## Escopo do primeiro mod Fabric

O primeiro mod Fabric deve:

1. Rodar no client.
2. A cada tick, obter o raycast/crosshair target do jogador.
3. Se o alvo for bloco, pegar `minecraft_id`, dimensao e posicao.
4. Escrever o JSON atomicamente somente quando o alvo mudar.
5. Nao tentar falar com o Elden Ring diretamente.

## Escopo que ainda exige pesquisa Elden Ring

Mostrar visualmente o bloco do Minecraft dentro do Elden Ring nao deve ser
implementado ate termos uma estrategia real de renderizacao/injecao segura:

- ponto de renderizacao ou overlay;
- ABI e thread corretas;
- coordenadas e escala;
- ciclo de vida de recursos graficos;
- compatibilidade offline/modded sem Easy Anti-Cheat.

Enquanto isso, a confirmacao do MVP e pelo log da DLL lendo mudancas reais do
Minecraft.
