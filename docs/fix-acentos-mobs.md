# Correção de acentos dos nomes de monstros

## Origem dos nomes

Os nomes não vêm do servidor nem de `Monster*.bmd`. O client chama
`OpenMonsterScript` em `src/source/Engine/Object/ZzzOpenData.cpp`, carregando:

`Data\\Local\\<idioma>\\NpcName_<idioma>.txt`

O parser de `ZzzInfomation.cpp` lê o terceiro token e passa `TokenString` para
`CMultiLanguage::ConvertFromUtf8`. Portanto, o arquivo português precisa ser
UTF-8. Os arquivos `.bmd` em `Data/Monster` são modelos/animações, não a tabela
de nomes.

## Diagnóstico e correção

`NpcName_Por.txt` estava em Windows-1252: os 156 registros com caracteres não
ASCII eram válidos em CP1252, mas o arquivo inteiro era inválido como UTF-8.
Consequentemente, os bytes de acentos eram convertidos para `?` ao passar pelo
parser UTF-8. Os comentários coreanos também continuam visualmente como `?`,
mas não são nomes exibidos no jogo.

O arquivo foi decodificado como CP1252, preservando os textos portugueses, e
regravado como UTF-8 sem BOM. Foram corrigidos **156 registros** (**98 nomes
únicos**); nenhum nome originalmente em inglês foi traduzido ou alterado.
Exemplos afetados:

- `Górgone` → `Górgone`
- `Tântalo dourado` → `Tântalo dourado`
- `Fênix da escuridão` → `Fênix da escuridão`
- `Caçador` → `Caçador`
- `Médico` → `Médico`
- `Ilusão de Kundun` → `Ilusão de Kundun`

A diferença nesses exemplos é a representação no arquivo (CP1252, que fazia o
client mostrar `?`, para UTF-8), não uma troca lexical do nome.

## Cópias atualizadas

As cópias instaladas têm o mesmo MD5 `1dc5b3d16629c1fdef7af0a1203ed523`:

- `MuMain/src/bin/Data/Local/Por/NpcName_Por.txt`
- `MuMain/out/build/windows-x86/src/Release/Data/Local/Por/NpcName_Por.txt`
- `teste-launcher/Game/Data/Local/Por/NpcName_Por.txt`

Os originais foram preservados em `backups/NpcName_Por-original.txt` junto às
cópias de `src/bin` e `out/build`.

## Patch

A entrada `Data/Local/Por/NpcName_Por.txt` foi atualizada em ambos os arquivos,
e os ZIPs continuam byte a byte iguais entre si:

- `site/patch/Data.zip`
- `teste-launcher/Game/Data.zip`

Novo MD5 de `Data.zip`: **`74b2a42759442bdf528d445145ba2f12`**.
A linha 2 de `site/patch/patchlist.txt` foi atualizada para esse valor.
