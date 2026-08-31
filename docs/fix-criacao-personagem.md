# Correção da criação de personagem

## Sintoma

Na tela de criação de personagem, o modelo 3D era renderizado, mas os painéis,
botões e textos da interface não apareciam.

## Causa

`RenderControls()` renderiza o personagem com `RenderCreateCharacter()`, que
chama `BeginOpengl()` e troca o viewport e as matrizes para a projeção 3D.
`EndOpengl()` não restaura esse estado. Assim, os controles desenhados depois
da prévia 3D permaneciam sob a projeção 3D e ficavam invisíveis.

## Correção

Depois de `RenderCreateCharacter()`, `RenderControls()` chama `BeginBitmap()` para
restaurar o viewport e a projeção 2D antes de desenhar os sprites, botões e
textos da tela.

`BeginBitmap()` é adequado para essa restauração: ele redefine diretamente o
viewport completo e as matrizes ortográficas 2D, sem contador de aninhamento.
A chamada adicional é segura quando a tela já iniciou um bloco 2D externo; o
bloco externo continua responsável pelo `EndBitmap()` correspondente.
