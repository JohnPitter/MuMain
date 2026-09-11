# Celestial — referência de metal dos monstros Golden

Auditoria local de 11/09/2026, base `b443487c`. Fonte autoritativa: código do cliente desta árvore, não uma descrição genérica de MU. Objetivo: usar reflexo metálico dourado forte, com cinza metálico de contraste, mantendo geometria, animações, atributos e aparência dos outros itens. Não houve execução do jogo nesta auditoria.

## O que os Golden realmente desenham

O caminho comum é **textura do monstro + reflexo METAL + reflexo CHROME**. A textura não é substituída por uma superfície uniformemente emissiva. `RenderCharacter` desenha primeiro `RenderObject` em [ZzzCharacter.cpp:8810](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:8810); depois acrescenta `RENDER_METAL | RENDER_BRIGHT` e `RENDER_CHROME | RENDER_BRIGHT` no [bloco Golden](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:8865).

| Grupo / ID de monstro | Modelo utilizado | Composição relevante |
| --- | --- | --- |
| Golden Budge Dragon, 43 | Budge Dragon, índice 2 | Base + METAL + CHROME |
| Golden Goblin, 78 | Goblin, índice 19 | Base + METAL + CHROME; machado próprio |
| Golden Derkon, 79 | Dragon, índice 31 | Base + METAL + CHROME |
| Golden Lizard King, 80 | Lizard, índice 36 | Base + METAL + CHROME; Chaos Lightning Staff Excellent |
| Golden Vepar, 81 | Vepar, índice 34 | Base + METAL + CHROME |
| Golden Tantallos, 82 | Tantallos, índice 42 | Base + METAL + CHROME; também efeitos de energia próprios |
| Golden Wheel, 83 | Golden Wheel, índice 41 | Base + METAL + CHROME; também efeitos de energia próprios |
| Golden Dark Knight / Devil / Stone Golem, 493–495 | Dark Knight 3 / Devil 26 / Golden Stone Golem 101 | Base + METAL + CHROME |
| Golden Crust / Satyros / Twin Tail, 496–498 | Crust 52 / Satyros 109 / Twin Tail 115 | Base + METAL + CHROME |
| Golden Iron Knight, 499 | Iron Knight 149, `Monster150.bmd` | Base + METAL + CHROME; efeitos específicos do Iron Knight dependem do caminho de mapa |
| Golden Napin / Great Dragon / Rabbit, 500–502 | Napin 142 / Dragon 31 / Rabbit 128 | Base + METAL + CHROME; Great Dragon também tem sprites/fogo próprios |
| Golden Titan / Soldier, 53–54 | Variantes próprias | Bloco separado: METAL com textura explícita `BITMAP_SHINY + 1`; não confundir com a dupla acima |

Criação dos modelos: [grupo 78–83](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:13718), [43](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:14057), [493–502](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:15130). Valores dos índices: [enum de modelos](C:/_wt-celestial-client/src/source/Core/Globals/_enum.h:4183). `OpenMonsterModel` soma o índice a `MODEL_MONSTER01` e abre `Monster` com sufixo `Type + 1`: [ZzzOpenData.cpp:2655](C:/_wt-celestial-client/src/source/Engine/Object/ZzzOpenData.cpp:2655). Exceção Titan/Soldier: [ZzzCharacter.cpp:9075](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:9075).

### Cor efetiva: não parar na primeira atribuição

O bloco Golden 493–502 escreve inicialmente `(1, 0.6, 0.3)`; Great Dragon escreve ainda `(1, 0, 0)`. Porém, as duas chamadas seguintes de `RenderPartObjectBodyColor` não passam `iMonsterIndex`. O argumento padrão é `-1`, em [ZzzObject.h:60](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.h:60), e o helper termina chamando `PartObjectColor` em [ZzzObject.cpp:9192](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:9192).

Para Iron Knight e os demais modelos sem exceção nesse helper, `Color = 0` produz **RGB `(1, 0.5, 0)` com Bright 1**, ou seja, âmbar puro sem componente azul. Confira a inicialização em [6649](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:6649) e a tabela final em [6889](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:6889). As cores condicionais por IDs 493–502 dentro de `RenderPartObjectBodyColor` só se aplicam quando esse parâmetro é fornecido; não são a cor efetiva dessas chamadas. Com alpha diferente de 1 há ainda o tratamento legado de fade em `PartObjectColor` e `RenderMesh`.

### Metal não é aura branca

| Passe | Textura nativa | Coordenadas de amostragem | Resultado |
| --- | --- | --- | --- |
| `RENDER_METAL` | `BITMAP_SHINY` → `Effect/Shiny01.jpg` | `u = normal.z × .5 + .2`, `v = normal.y × .5 + .5` | Reflexo aderente à orientação da face; muda quando a peça gira ou anima |
| `RENDER_CHROME` | `BITMAP_CHROME` → `Effect/Chrome01.jpg` | `u = normal.z × .5 + wave`, `v = normal.y × .5 + wave × 2` | Reflexo que percorre a superfície com o tempo |
| `RENDER_TEXTURE` emissivo | Textura/UV próprios da malha | UV do BMD, preservando deslocamentos do objeto quando habilitados | Pedras, linhas mágicas e máscaras; não é a camada metálica |

`wave = (WorldTime inteiro % 10000) × .0001`. As fórmulas estão em [ZzzBMD.cpp:1631](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:1631); seleção das texturas em [1477](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:1477); carregamento de Shiny01 e Chrome01 em [ZzzOpenData.cpp:5513](C:/_wt-celestial-client/src/source/Engine/Object/ZzzOpenData.cpp:5513). Não foi encontrado um `RENDER_GOLD`: o dourado resulta de textura refletida × RGB âmbar.

`RENDER_BRIGHT` chama blend **GL_ONE, GL_ONE**, não transparência `SRC_ALPHA`. Ele desliga escrita de profundidade e culling, mantendo o teste de profundidade. Fonte: [EnableAlphaBlend](C:/_wt-celestial-client/src/source/Render/Textures/ZzzOpenglUtil.cpp:402). Portanto a força/fade precisa chegar ao RGB; só reduzir o alpha não é um controle uniforme de brilho.

## Particularidade Iron Knight / Dark Iron Knight

Golden Iron Knight 499 reutiliza o modelo Iron Knight 149 (`Monster150.bmd`); Dark Iron Knight 565 é outro modelo, 208 (`Monster209.bmd`). Os atlas e amostras originais extraídos estão na pasta [dark-iron-knight](C:/_wt-celestial-client/art-source/celestial/research/dark-iron-knight). A referência de cor escura/prata não deve ser confundida com uma textura de Golden pintada em amarelo: o efeito Golden vem dos passes do cliente.

Em [GM_Raklion.cpp:1477](C:/_wt-celestial-client/src/source/World/GameMaps/GM_Raklion.cpp:1477), o Iron Knight vivo desenha base TEXTURE, máscara `BITMAP_IRONKNIGHT_BODY_BRIGHT` pulsando com `(sin(WorldTime × .01) + 1) × .4 + .2`, outra malha translúcida e brilho/chrome próprios. Dark Iron Knight possui bloco equivalente em [1673](C:/_wt-celestial-client/src/source/World/GameMaps/GM_Raklion.cpp:1673). A máscara carregada é `Monster/icenightlight.jpg`, em [ZzzOpenData.cpp:5492](C:/_wt-celestial-client/src/source/Engine/Object/ZzzOpenData.cpp:5492). Esses tratamentos passam pelo despacho de mapa: [w_MapProcess.cpp:273](C:/_wt-celestial-client/src/source/World/MapInfra/w_MapProcess.cpp:273) verifica `IsCurrentMap`. Não atribuir todos esses efeitos automaticamente a qualquer Golden fora desse contexto.

## Diferença CPU/GPU encontrada nesta revisão

Há um risco preexistente no renderer compartilhado: `RenderMesh` resolve o ponteiro `texture` a partir da malha **antes** de escolher Chrome/Shiny pelos flags. O caminho legado chama `BindTexture(BITMAP_CHROME/SHINY)`, mas os caminhos GPU e VBO dinâmico entregam ao shader o `TextureNumber` do ponteiro resolvido inicialmente. Isso pode fazer um passe sem índice explícito amostrar o atlas difuso usando coordenadas de chrome, em vez da textura de reflexo pretendida.

Evidências: resolução inicial em [ZzzBMD.cpp:1316](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:1316), binding por flags em [1477](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:1477), sampler GPU em [1734](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:1734), sampler do VBO dinâmico em [1883](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:1883).

**Correção restrita ao Celestial:** cada passe Metal/Chrome agora fornece explicitamente `BITMAP_SHINY`/`BITMAP_CHROME`. Assim o ponteiro inicial e o binding concordam em todos os caminhos. O renderer compartilhado e os monstros existentes não foram alterados. A equivalência alegada é com a composição e os mapas de reflexão nativos pretendidos, não com cada possível artefato do caminho gráfico antigo.

As coordenadas METAL existem no shader como `u_ChromeVariant == 6`, em [BMDMeshShader.cpp:272](C:/_wt-celestial-client/src/source/Render/Shaders/BMDMeshShader.cpp:272), escolhido em [ZzzBMD.cpp:1807](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:1807). Chrome comum usa variante 0. O caminho CPU calcula as mesmas projeções em `g_chrome`. `RENDER_BRIGHT` usa `BodyLight` sem escurecimento direcional nos shaders e no VBO dinâmico: [1817](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:1817), [2780](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:2780). Não é necessário um novo shader ou novo mapa de iluminação para o Celestial.

## Perfil implementado no Celestial

Arquitetura: descrição pura de até dois passes em `CelestialShimmer`, execução OpenGL no adaptador `CelestialAppearance`. Arrays fixos, sem alocação/I/O por malha. Fontes: [perfil](C:/_wt-celestial-client/src/source/Render/Models/CelestialShimmer.cpp:10), [bindings explícitos](C:/_wt-celestial-client/src/source/Render/Models/CelestialAppearance.cpp:27), [execução e preservação do scroll emissivo](C:/_wt-celestial-client/src/source/Render/Models/CelestialAppearance.cpp:49).

| Material Celestial | METAL em +0 → +15 | CHROME em +0 → +15 | Cor e observações |
| --- | --- | --- | --- |
| `Celestial_Gold.jpg` | .90 → 1.00 | .70 → 1.00 | RGB Golden `(1, .5, 0)`, independente de Excellent; em +15 ambos atingem a força nativa |
| `Celestial_Ivory.jpg` | .30 → .36 | .12 → .16 | Nome de arquivo preservado por compatibilidade; agora o acabamento é cinza metálico, RGB `(.60, .62, .64)` |
| `Celestial_Sapphire.jpg` | — | — | Emissão original própria .08–.13, sem aumento |
| `Celestial_Emissive.jpg` | — | — | Emissão original própria .12–.20, sem aumento |
| Outros materiais | — | — | Nenhum passe novo |

Qualidade 0 desativa efeitos; qualidade 1 mantém somente METAL dourado; qualidade 2+ habilita os dois reflexos e as emissões. Distância acima de 1200, alpha não visível, cloaking e malhas ocultas não ganham reflexos. O `BodyLight` original é restaurado. Metal e Chrome usam projeção normal nativa; emissivos conservam `BlendMeshTexCoordU/V` do objeto, além de seus UVs originais.

A base TEXTURE continua sendo desenhada uma única vez antes dos reflexos, exatamente como a estrutura Golden. No +15, a energia teórica somada dos dois reflexos dourados é `(2, 1, 0)` antes da amostragem; os padrões Shiny/Chrome modulam isso espacialmente. Isso **pode** saturar pontos de brilho, assim como o Golden; não é prova de imagem sem clipping. O cinza tem orçamento total máximo por canal abaixo de .333 e a auréola não recebeu aumento. Atlas com sulcos escuros e bordas claras são necessários para manter a leitura das placas entre os highlights.

## Validação e limites

Onze testes de perfil cobrem dupla METAL/CHROME, RGB Golden, força nativa no +0/+15, ausência de pulsação branca, limites/qualidade, nomes não Celestial, fade RGB aplicado uma vez, orçamento do cinza, preservação de gems/halo e entradas não finitas. Fonte: [test_celestial_shimmer.cpp](C:/_wt-celestial-client/tests/celestial/test_celestial_shimmer.cpp).

Esta subtask não compilou nem iniciou o cliente; build/testes integrados ficam com a tarefa principal. Inspeção visual em jogo ainda precisa confirmar brilho durante idle/caminhada/cast, as superfícies traseiras, contraste ouro/cinza sob luz de mapa, equipamento no inventário, qualidade reduzida e custo de dois passes nas asas. Nenhum teste puro demonstra aprovação estética ou FPS real. Nenhuma regra Grand Master/400, Excellent, sockets ou fases do conjunto foi alterada.
