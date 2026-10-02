# Desenvolvimento do hooker

## Escopo do milestone 1

`DllMain -> CreateThread -> logger -> selection_reader -> log -> fim da thread`.
O carregamento e compativel com o mecanismo de DLLs do EldenModLoader, mas
compatibilidade dentro da versao instalada do jogo precisa de teste separado.
Uso previsto: sessao offline/modded, sem qualquer implementacao de bypass de EAC.

DllMain faz somente fixacao do modulo e criacao da thread. Nao espera,
nao le arquivos e nao instancia logger/parser. A fixacao com GetModuleHandleExW
PIN antes do despacho evita descarregamento enquanto a thread esta enfileirada.
Hot unload nao e suportado. O handle de thread permanece valido para consultas
ate o processo terminar; o Windows o libera no encerramento.
O CRT e estatico; notificacoes de thread permanecem habilitadas.

Logger vive dentro da thread, usa mutex por instancia, arquivo em append,
timestamp UTC, PID/TID, escape de caracteres de controle e flush por mensagem.
Seu destrutor fecha o arquivo ao fim da leitura. Nao ha logger global nem
join/flush em DLL_PROCESS_DETACH. Falhas de escrita retornam false; falhas
criticas de log encerram a inicializacao com codigo 2. Fallback so ocorre na abertura.
O mutex coordena threads desta instancia, nao processos independentes. Para testes
simultaneos, use diretorios de log distintos. Sem rotacao neste milestone de leitura unica.

## Contrato e leitor

`selection_reader.h` nao expoe tipos de nlohmann/json. A implementacao isolada
usa o header local 3.11.3, validado por SHA-256 durante configure.
Retorna SelectionResult com selecao ou mensagem de erro, nunca um estado parcial.

Contrato v1 em `shared-formats/minecraft_selection.schema.json`:

- `schema_version` igual a 1, `minecraft_id`, `dimension` e `position` obrigatorios.
- IDs namespaced em minusculas, ate 256 caracteres; objetos sem campos extras.
- x/y/z numericos finitos entre -1e9 e +1e9; coordenadas ainda sao do Minecraft.
- UTF-8, limite de 64 KiB, chaves duplicadas rejeitadas, profundidade maxima 32.

O leitor valida o contrato em C++; nao carrega o schema em runtime. Limites de
bytes, profundidade e duplicidade sao regras de transporte adicionais ao schema.
O futuro produtor deve gravar um temporario no mesmo diretorio, fechar o arquivo
e substituir atomicamente o destino. Escrita parcial pode gerar WARN; ainda nao
ha polling, retry, protocolo de confirmacao, TTL nem deteccao de selecao antiga.

`MEB_WaitForInitialization(timeout_ms)` e uma exportacao para o host de teste,
chamada apenas fora de DllMain. Retornos: 0 sucesso; 1 selecao indisponivel/invalida;
2 falha de log; 3 excecao de inicializacao; 1460 timeout; outros erros Win32 podem ocorrer.
O retorno LoadLibrary apenas confirma carregamento, nao sucesso do leitor.

## Pesquisa futura

`FutureMinHookInitialization` em dllmain.cpp marca a integracao futura.
Antes de acrescentar hooks: identificar build do jogo, alvos, ABI, thread de
execucao, ownership dos objetos e estrategia de desligamento. Tudo e TODO/research.
Nao existem enderecos, assinaturas nem chamadas MH_* neste milestone.
Os binarios MinHook x64 encontrados nao sao copiados nem carregados agora.

Proximos milestones: produtor Fabric com escrita atomica do contrato; leitura
continua com cancelamento; mapeamento explicito de coordenadas; somente depois,
pesquisa de funcoes internas. Bridge server e opcional e ainda nao implementado.
A versao do Minecraft/Fabric/JDK sera escolhida no milestone Fabric.

## Verificacao

CTest inclui validacao de entrada, numeros, IDs, arquivos ausentes/grandes,
caminho Unicode, 400 escritas concorrentes, fallback de log e dois testes de DLL.
O smoke host tambem chama FreeLibrary imediatamente para verificar a fixacao.
Os testes nao comprovam compatibilidade com Elden Ring nem com outros mods.

Referencias primarias:

- https://github.com/techiew/EldenRingModLoader
- https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain
- https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandleexw
- https://github.com/nlohmann/json/releases/tag/v3.11.3
