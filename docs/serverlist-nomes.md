# Nomes dos grupos do ServerList

O cliente lê `Data\Local\ServerList.bmd` ao iniciar. O arquivo usa `BuxConvert`, um XOR cíclico com os bytes `FC CF AB`; o ciclo reinicia para cada registro fixo e para o texto da descrição.

Cada grupo tem um registro fixo empacotado de 53 bytes: `WORD` índice, `char[32]` nome ANSI/ASCII terminado em NUL, `BYTE` posição, `BYTE` sequência, 15 flags de PvP e `short` tamanho da descrição. A descrição vem logo depois do registro. A cópia atual tem 165 bytes e três registros (cada descrição ocupa 2 bytes).

O cliente calcula o grupo por `ServerID / MAX_SERVER_PER_GROUP` e o número mostrado por `(ServerID % MAX_SERVER_PER_GROUP) + 1`. A constante em `src/source/Core/Globals/_define.h` é `MAX_SERVER_PER_GROUP = 20`.

## Configuração atual

| Canal | ServerID | Grupo no BMD | Nome exibido |
|---|---:|---:|---|
| S6 Easy | 0 | 0 | MuRaiz S6 Easy-1 |
| 99d Hard | 20 | 1 | MuRaiz 99d Hard-1 |
| S2 Medium | 40 | 2 | MuRaiz S2 Medium-1 |

Os `ServerID` estão em `config."GameServerDefinition"`. Não há chaves estrangeiras para colunas `ServerID` no banco no momento da alteração.

## Reeditar no futuro

1. Faça backup do banco com `pg_dump` e dos BMDs antes de alterar.
2. Edite os nomes no mapa `$Names` de `tools/edit-serverlist.ps1` (máximo de 31 bytes mais o NUL).
3. Execute o script para cada cópia usada pelo executável:

   ```powershell
   powershell -File MuMain/tools/edit-serverlist.ps1 -Path MuMain/src/bin/Data/Local/ServerList.bmd
   powershell -File MuMain/tools/edit-serverlist.ps1 -Path MuMain/out/build/windows-x86/src/Release/Data/Local/ServerList.bmd
   ```

4. Rode novamente com `-Verify`; ele decodifica os registros e imprime índice, nome e tamanho. O tamanho deve continuar 165 bytes. Reabra o cliente para reler o arquivo.
