# MinecraftEldenBridge Raycast

Mod Fabric client-side para Minecraft 26.3. Publica o bloco mirado pelo jogador
no JSON compartilhado consumido pelo hooker C++.

Na raiz do repositorio:

```powershell
.\tools\build_fabric.ps1
.\tools\install_fabric_mod.ps1 -WhatIf
.\tools\install_fabric_mod.ps1
```

Guia de instalacao, segundo plano, contrato e teste integrado:
[MINECRAFT_FABRIC_MVP](../docs/MINECRAFT_FABRIC_MVP.md).

`src/client`: captura no tick e worker de I/O; `src/main`: snapshot e escritor
atomico independentes do jogo; `src/test`: testes do contrato e filesystem.
Fabric declara `environment: client`; nao instalar em servidor dedicado.

Gradle Wrapper 9.6.0 obtido do repositorio oficial Gradle (Apache-2.0).
SHA-256 verificado de `gradle-wrapper.jar`:
`497c8c2a7e5031f6aa847f88104aa80a93532ec32ee17bdb8d1d2f67a194a9c7`.
A distribuicao tambem tem SHA-256 fixado em `gradle-wrapper.properties`.
Minecraft, Fabric API e bibliotecas de terceiros nao sao redistribuidos no jar
da ponte. Gson e SLF4J sao fornecidos pelo ambiente Minecraft/Fabric.
