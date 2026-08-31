# Correção de acentos de `QuestWords_por.bmd`

## Processo

1. O original foi lido de `MuMain/src/bin/Data/Local/Por/QuestWords_por.bmd` registro a registro. Cada registro é `int32 index` + `int16 byte-length` little-endian + payload. O XOR cíclico `FC CF AB` foi reiniciado no cabeçalho e no payload de cada registro. O payload foi tentado como UTF-8 e, quando inválido, lido com fallback Latin-1; o terminador NUL foi removido do texto.
2. Os seis arquivos `questwords-fixed-*.tsv` foram lidos como UTF-8 e aplicados por índice. Todos os índices existiam na tabela de 2.288 registros. Foram aplicadas **1.853 correções únicas** (substituições; o total aproximado informado de linhas era 1.791).
3. O arquivo foi re-empacotado com texto UTF-8 puro, incluindo terminador NUL, e o mesmo formato/XOR. O resultado tem 2.288 registros e 286.583 bytes (o original tinha 282.500 bytes; o aumento decorre da conversão de payloads Latin-1 para UTF-8).

## Validação

A decodificação do resultado confirmou todos os 2.288 índices e igualdade textual após round-trip. Os registros não incluídos nos fixes permaneceram textualmente idênticos ao original; os registros corrigidos foram comparados com os TSVs.

Exemplos decodificados:

- índice 1: `... um herói como você para acabar ...`
- índice 83: `Feche a janela de Missão ...`
- índice 27: `... e o nível do personagem ...`

As três cópias instaladas do BMD foram verificadas byte a byte iguais:

- `MuMain/src/bin/Data/Local/Por/QuestWords_por.bmd`
- `MuMain/out/build/windows-x86/src/Release/Data/Local/Por/QuestWords_por.bmd`
- `teste-launcher/Game/Data/Local/Por/QuestWords_por.bmd`

O original foi preservado em `MuMain/src/bin/Data/Local/Por/backups/QuestWords_por-original.bmd`.

## Patch

A investigação confirmou que `site/patch/Data.zip` e `teste-launcher/Game/Data.zip` eram byte a byte iguais e continham a entrada `Data/Local/Por/QuestWords_por.bmd` (estrutura relativa `Data/...`, adequada ao destino `Game\Data.zip`). A entrada foi substituída nos dois ZIPs, mantendo a ordem e metadados das demais entradas.

MD5 novo do `Data.zip`: **`fc7e5127ddd1bd452831770b7d445297`**.

A linha 2 de `site/patch/patchlist.txt` foi atualizada para esse MD5.
