# Zeus — o acabamento azul medido no Legendary +15 Excellent

Estudo de 11/09/2026 sobre a base congelada `ultimate-base-downloaded.zip`
(SHA-256 `baedcdbe3b66dc2763a9a6538f8d0fd592bc7cccd76844952ec174ca323d3c80`).
Fonte autoritativa: código do cliente desta árvore e os atlas reais do ZIP,
não descrições genéricas de MU. Objetivo: orientar o **perfil de runtime do
acabamento azul do set Herdeiro de Zeus** (Azul Celeste principal + Branco
Platinado nos detalhes) na onda futura de integração, usando o set Legendary
nativo como referência de como um azul sobrevive ao pipeline +15/Excellent.
**Esta lane não altera código de render**; tudo abaixo é evidência e proposta
de perfil, não mudança implementada.

## O que é o Legendary nesta base

Família 3 do catálogo de armaduras (`docs/art/armor-reference/canonical-families.json`),
grupos 8–11, item número 3, usável por Dark Wizard **e Magic Gladiator**
(`C:/_wt-celestial-server/src/Persistence/Initialization/VersionSeasonSix/Items/Armors.cs`,
linhas 120/293). Modelos `Data/Player/{Helm,Armor,Pant,Glove,Boot}Male04.bmd`,
atlas `Data/Player/{head,upper,lower,boots,gloves}_13m.OZJ`.

Medição direta dos atlas extraídos do ZIP congelado (quantização 32 níveis,
pixels quase pretos excluídos da dominância):

| Atlas | Dimensão | Cores dominantes | Leitura |
| --- | --- | --- | --- |
| `upper_13m` | 128×128 | `#000020` 15,7%, `#202060` 6,3%, `#e0c0c0` 5,3%, `#e0c0e0` 4,1% | Azul-marinho profundo + highlights platinados |
| `lower_13m` | 128×128 | `#202020` 14,2%, `#200020` 11,5%, `#202040` 11,3% | Marinho menos saturado, quase grafite |
| `boots_13m` | 64×64 | `#e0c0c0` 21,1%, `#000020` 7,6%, `#202060` 6,1%, `#e0a040` 3,5% | Detalhe claro dominante sobre azul |
| `gloves_13m` | 64×64 | `#000020` 27,5%, `#202060` 9,5%, `#000040` 6,5% | O atlas mais azul da família (55,4% em clusters azuis) |
| `head_13m` | 64×64 | `#e08040` 8,4%, `#e0c0c0` 8,2%, `#202060` 8,0% | Rosto/cabelo quentes com elmo azul |

Conclusão de textura: o “azul Legendary” **não é uma textura azul viva**; é
marinho escuro (B>R em ~30–55% da área útil) com highlights quase brancos
pintados. A leitura azul do set no +15 Excellent vem majoritariamente dos
**passes e do tint**, não do difuso. Isso valida a estratégia do Zeus: base
Azul Celeste moderadamente escura nos masters + branco platina nos detalhes,
deixando brilho/especular para os passes de upgrade.

## Por que o Legendary “rende azul” no +15: a cadeia de passes

O upgrade de aparência chega ao renderer já convertido em faixas
(0–2→0 … 15→7; `ItemExtensions.cs:28` do servidor, `LevelConvert`
em `ZzzCharacter.cpp:12629`). Para nível visual ≥ 7 e qualidade de efeitos
ligada, `RenderPartObject` escolhe uma pipeline de passes
[ZzzObject.cpp:10506](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:10506):

| Nível | Pipeline | Escala da base | Passes aditivos |
| --- | --- | --- | --- |
| 7–8 | `TIER_CHROME_1` | ×0,8 | `CHROME\|BRIGHT` |
| 9–10 | `TIER_CHROME_METAL` | ×0,9 | `CHROME\|BRIGHT` + `METAL\|BRIGHT` |
| 11–12 | `TIER_FULL_SPECULAR_V1` | ×0,9 | `CHROME2\|BRIGHT` (via Color2) + `METAL` + `CHROME` |
| 13–15 | `TIER_FULL_SPECULAR_V2` | ×0,9 | `CHROME4\|BRIGHT` (via Color2) + `METAL` + `CHROME` |

Distância > 1200 devolve material-base; `optionLvl`
(`g_pOption->GetRenderLevel()`, teto `MaxRenderLevel = 5` em
`GameConfigConstants.h:140`) limita o tier: com qualidade 3, o +15 cai no V1.
Qualidade 0 desliga tudo e devolve a textura pura
[ZzzObject.cpp:10640](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:10640).

### O tint azul: `PartObjectColor` por família

A cor característica do item entra nos passes por `PartObjectColor`
[ZzzObject.cpp:6889](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:6889).
Para grupos 7–11 (`MODEL_ITEM` + grupo×512), o índice é o número do item
[ZzzObject.cpp:6827](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:6827).
**Item 3 (Legendary) → Color 3 → RGB `(0,0; 0,5; 1,0)`**
[ZzzObject.cpp:6837](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:6837) e
[ZzzObject.cpp:6894](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:6894).
É este o `(0.35,0.35,0.6)`-like citado no comentário do shader: o
`specularTint` é obtido por dry-run de `PartObjectColor(Type, 1, 1, …)`
[ZzzObject.cpp:10595](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:10595).
Portanto, no Legendary +15, **METAL e CHROME são reflexos tingidos de azul
puro (0, .5, 1)** sobre a base marinha — o cliente “pinta” o azul do item na
camada especular, não no difuso.

`PartObjectColor2` (usado no passe CHROME4/CHROME2 `useColor2`) devolve
neutro `(1,1,1)×Luz` para a armadura Legendary
[ZzzObject.cpp:6979](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:6979),
mas dá **azul `(0, .5, 1)` ao Legendary Staff**
(`MODEL_LEGENDARY_STAFF = MODEL_STAFF + 5`,
[ZzzObject.cpp:6947](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:6947),
[ZzzObject.cpp:7015](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:7015)).
`PartObjectColor3` (ramo Ancient) torna o item 3 **dourado `(1, .7, .2)`**,
exceção de família; o padrão Ancient é azul `(0.1, 0.6, 1)`
[ZzzObject.cpp:7020](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:7052).

### A camada Excellent

Excellent é `else if` depois do upgrade e adiciona um **corpo inteiro
`RENDER_TEXTURE | RENDER_BRIGHT`** com cor oscilante
`L = sin(WorldTime × 0.002) × .5 + .5` → `(L, .3L, 1−L)`
[ZzzObject.cpp:10647](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:10653).
`RENDER_BRIGHT` é blend `GL_ONE, GL_ONE` sem escrita de profundidade
(`EnableAlphaBlend`, `ZzzOpenglUtil.cpp:402`; ver também
[golden-runtime.md](../../../art-source/celestial/research/golden-runtime.md)):
a força chega pelo RGB, não pelo alpha. A oscilação vai de azul puro `(0,0,1)`
a âmbar `(1,.3,0)` — no Legendary o marinho base faz o intervalo âmbar parecer
“brilho dourado momentâneo” sobre azul, e o intervalo azul reativa o marinho.
Exceções por mesh (Sacred/Storm Hard/Piercing/Phoenix Soul/Mistery e
`MODEL_ARMORINVEN_74` oscilam apenas malhas específicas para não tingir pele).

### O +15 por cima: sprites e cascas por parte

`NextGradeObjectRender`
[ZzzObject.cpp:9437](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:9437)
acrescenta, só no +15:

- **Armas:** sprites `BITMAP_LIGHT` laranja `(1, .6, 0)` nos ossos 27/28 (arma
  da direita) ou 27/36–37 (esquerda), `BITMAP_MAGIC` pulsando
  `0.7f+0.3f·|sin|`, `BITMAP_FLARE_RED` e `BITMAP_LIGHTNING_MEGA1..3`
  aleatórios. Nota: o brilho nativo de +15 de arma é laranja, **não azul**.
- **Armadura:** cascas `MODEL_15GRADE_ARMOR_OBJ_{HEAD, BODYLEFT/RIGHT,
  PANTLEFT/RIGHT, ARMLEFT/RIGHT, BOOTLEFT/RIGHT}` ligadas aos ossos 20 (cabeça),
  35/26 (ombros), 3/10 (coxas), 36/27 (antebraços), 4/11 (joelhos/pés)
  [ZzzObject.cpp:9512](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:9512).

### CPU vs GPU

Com `CItemSpecularShader` suportado, os quatro desenhos (base + 3 passes)
colapsam em **um** render pelo modo 6/7 do `BMDMeshShader`
[BMDMeshShader.cpp:368](C:/_wt-zeus-foundation/src/source/Render/Shaders/BMDMeshShader.cpp:368):
`(base + chrome2) × luzDaBase + (metal + chrome1) × specularTint`, com
`chrome2` amostrando `BITMAP_CHROME2` em UV animada (CHROME2 varredura;
CHROME4 projeção pela normal) e `metal/chrome1` em MatCap estático
(`ItemSpecularShader.h:24` documenta a migração DXP-02). Sem shader, o caminho
legado repete o corpo por passe com projeções CPU (`ZzzBMD.cpp:1631/1648`).
Mesmo número de draw calls não prova mesmo custo: CHROME4 isolado força o
caminho de normais/UV por CPU quando não há shader — ver
[kundun-platinum.md](../../../art-source/celestial/research/kundun-platinum.md).

## Perfil recomendado para o Zeus (proposta, não implementação)

1. **Base:** masters próprios com Azul Celeste médio-escuro (evitar marinho
   puro: o Zeus deve ler azul já no +0) e detalhes Branco Platinado usando
   obrigatoriamente os masters aprovados
   `C:/_wt-platina-client/art-source/celestial/textures/masters/white-platina`
   (`Gold.png` SHA-256 `a85d8f37d420296a709664a6c2a6b4ee1b0b88872432bdddb2e5b3f377d21b35`,
   `Ivory.png` `9105bc8aa9c475b4a21e519a9954c43d8ac04ee397754823b5795cdd08c79bfa`,
   âncoras `#d7d3d4`/`#545156`/`#faf8f6` de `generate_white_platina.py`).
   Highlights claros pintados nos atlas, como o Legendary faz — os passes
   saturam áreas claras em highlights metálicos.
2. **Especular azul:** um único `specularTint` azul celeste por material Zeus
   (proposta inicial `RGB (0.10, 0.45, 1.00)`, mais claro que o `(0,.5,1)`
   nativo para diferenciar do Legendary sem sair do vocabulary do loader),
   aplicado só nos materiais azul; os detalhes branco-platina ficam com tint
   neutro para não amarelar nem azular o branco aprovado.
3. **Excellent:** adotar o overlay clássico via integração (o overlay só exige
   que o modelo não seja excluído por `CanRenderLegacyGradeTint`,
   [ZzzObject.cpp:54](C:/_wt-zeus-foundation/src/source/Engine/Object/ZzzObject.cpp:56));
   a oscilação `(L,.3L,1−L)` sobre base azul celeste recria o efeito
   “tempestade que também ilumina” do conceito sem novo shader. Se o dono
   quiser matizar a fase âmbar, isso é decisão de integração futura, não aqui.
4. **+15:** reusar `NextGradeObjectRender` como está (sprites/cascas nativas);
   customizar cor dos sprites é escopo de onda posterior com testes visuais.
5. **Orçamento:** como o Kundun/Golden — limitar a dois passes os materiais
   Zeus quando não houver shader; quality 0 desliga; distância > 1200 cai para
   base; nada de emissão branca uniforme (o branco platina não pode virar
   auréola). Ícones não recebem passe em tempo real.

## Limites deste estudo

A paleta medida é do atlas (pixels), não da imagem final pós-UV/iluminação; os
percentuais são dominância relativa de pixels úteis. Não houve execução do
jogo: a confirmação visual (Legendary +15 Excellent e o futuro Zeus na mesma
câmera/mapa) permanece pendente para a onda de integração. Linhas citadas
refletem esta árvore; hashes do audit congelam a evidência.
