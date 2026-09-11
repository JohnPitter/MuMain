# Contrato de cloth da Capa Zeus — estudo para a integração futura

Estudo prático de como uma **capa nova do Duel Master/MG** (a Capa do Herdeiro
de Zeus) se conecta ao runtime de cloth do cliente, tomando o Manto do
Imperador nativo (`Data/Item/DarkLordRobe02.bmd`, `MODEL_CAPE_OF_EMPEROR =
MODEL_WING + 40`, `_enum.h:1968`) como precedente auditado. Nada aqui altera
código C++, IDs, rede ou runtime: é o mapa do que a onda de integração
precisará reproduzir para a capa Zeus com ID próprio. O estudo espelha o do
Manto Poseidon (`art-source/poseidon/cape-cloth-contract.md`) **e anota a
diferença de classe**: o Poseidon mira o Dark Lord; a capa Zeus veste o
Duel Master — e o portão de cloth do cliente **não está aberto** para essa
classe (seção 4, a razão deste documento existir).

Fontes: árvore `src/source` deste repositório (arquivos e linhas citados,
verificados nesta lane), o BMD nativo congelado (`ultimate-base-downloaded.zip`,
SHA-256 em `native-reference-audit.json`, chave `emperor_cape`) e o esqueleto
`Data/Player/player.bmd`.

## 1. Estrutura do BMD nativo (ferragem rígida)

`native-reference-audit.json`, chave `emperor_cape`:

| Propriedade | Valor medido |
| --- | --- |
| Osso | 1 osso, índice 0, nome `collar`, pai `-1`, não-dummy |
| Pose | 1 ação, 1 quadro (`action_frames [1]`, `action_locks [false]`) |
| Bind do osso 0 | posição `(0, 1.032, 0)`, rotação `(0, 0, -π)` |
| Meshes | 3 meshes, 192 triângulos no total |
| Meshes 0/1/2 | `dl_redwings01.jpg` (156 tris), `dl_redwings03.tga` (20), `dl_redwings02.tga` (16) |

Leitura: o BMD carrega **apenas ferragem rígida** — gola/jugo e dois painéis
rígidos que descem pelas costas. Não há malha de tecido no arquivo. A capa
Zeus segue exatamente essa divisão: `prototype/models/Zeus_Cape.bmd` carrega
gola, brasão e caudas rígidas em **um osso** (`collar`, índice 0, 1 pose);
a massa azul de tecido NÃO vai no BMD.

## 2. O cloth NÃO vem do BMD — é gerado por código

O tecido é criado proceduralmente em `ZzzCharacter.cpp` no bloco
`if (bCloak)` (~linha 9840). As grades abaixo foram relidas e confirmadas
nesta lane (valores literais das chamadas `CPhysicsCloth::Create`):

### 2a. Ramo Dark Lord / Lord Emperor (`GetBaseClass == CLASS_DARK_LORD`, ~9844)

`numCloth = 6` quando `c->Wing.Type == MODEL_CAPE_OF_EMPEROR`, senão 4
(sem os painéis laterais e com a capa principal no slot de textura
`BITMAP_ROBE + 7`).

| Grade | Osso | Âncora local (x,y,z) | Grade | Tamanho | Textura | Flags |
| --- | --- | --- | --- | --- | --- | --- |
| `pCloth[0]` capa de ombro | 20 | (0, 0, 20) | 6×5 | 30×70 | `ROBE+6` | CURVED, RUBBER2, MASK_LIGHT, STRICTDISTANCE, SHORT_SHOULDER, NORMAL_THICKNESS, OPT_HAIR; vento 10–50 |
| `pCloth[1]` capa de ombro (blenda) | 20 | (0, 5, 18) | 5×5 | 30×70 | `ROBE+6` | idem + MASK_BLEND; vento 8–40 |
| `pCloth[2]` **capa principal** | 19 | (0, 8, 10) | 10×10 | 180×180 | `ROBE+9` | CURVED, SHORT_SHOULDER, HEAVY, MASK_ALPHA |
| `pCloth[3]` saia (só com skin DL/LE) | 18 | (0, 10, −5) | 5×5 | 50×90 | `BITMAP_DARK_LOAD_SKIRT` (skin DL) ou `BITMAP_DARKLOAD_SKIRT_3RD` (skin LE) | MASK_ALPHA, HEAVY, STICKED, SHORT_SHOULDER |
| `pCloth[4]` painel lateral direito | 19 | (30, 15, 10) | 2×5 | 12×200 | `ROBE+10` | FLAT, SHAPE_NORMAL, COTTON, MASK_ALPHA |
| `pCloth[5]` painel lateral esquerdo | 19 | (−30, 20, 10) | 2×5 | 12×200 | `ROBE+10` | idem |

Colisores (`AddCollisionSphere`): `pCloth[0]/[1]`: duas esferas r=27 no osso
17, centros (∓10, 20, 20); `pCloth[2]`: quatro esferas no osso 17, r=25 em
(∓10, −10, −10) e r=27 em (∓10, −10, 20); `pCloth[3]`: r=30 no osso 2 em
(0, −15, −20); `pCloth[4]/[5]`: r=30 no osso 2 e r=35 no osso 17.

**A saia depende de skin DL/LE, confirmado em código** (~9903–9917): a grade
`pCloth[3]` só é criada quando
`c->BodyPart[BODYPART_ARMOR].Type == MODEL_BODY_ARMOR + SKIN_CLASS_DARK_LORD`
ou `+ SKIN_CLASS_LORDEMPEROR`, e é destruída/recriada dinamicamente quando a
armadura muda (~9986–10125). A armadura do set Zeus é a família Class304
(`SKIN_CLASS_DARK`) — **não existe saia nativa para o MG/Duel Master e a capa
Zeus não pede saia**: o faldão do conceito já é geometria da peça Calça
(protótipo commitado), e a capa se compõe de capa principal + caudas rígidas.

### 2b. Ramo Ragefighter (`GetBaseClass == CLASS_RAGEFIGHTER`, ~9936)

1 grade (capa de ombro 180×170 `BITMAP_MANTOE`/`BITMAP_NCCAPE` no osso 19,
âncora (0, 15, 5)) ou 3 com `MODEL_CAPE_OF_OVERRULE` (painéis laterais
(±25, 15, 2) 2×5 12×180 `BITMAP_MANTO01` com elastes `PCT_ELASTIC_RAGE_L/R`).

### 2c. Ramo genérico (qualquer outra classe com `bCloak`, ~9964)

1 única grade no osso 19, âncora (0, 10, 0), **75×120**, `BITMAP_ROBE`
(ou 120×120 `ROBE+8` com `MODEL_WING_OF_RUIN`). É o único cloth que um
personagem da família CLASS_DARK pode receber hoje (seção 4).

### 2d. Como a grade é construída (`PhysicsManager.cpp`, `CPhysicsCloth::Create`)

1. Vértices gerados no espaço do osso-âncora: largura em x centrada, y fixo
   (20; 0 se STICKED), profundação em −z linha a linha.
2. `PCT_SHORT_SHOULDER`: a largura de cada linha escala de
   `RATE_SHORT_SHOULDER = 0.6` (topo) até 1.0 (base) — silhueta que abre para
   baixo. `PCT_CURVED` recurva as linhas para trás.
3. A linha do topo (`SetFixedVertices`) é **presa** ao osso-âncora: primeira
   linha marcada `PVS_FIXEDPOS`, posicionada com largura `0.6 × fWidth`. É a
   única ligação física com o esqueleto; o resto é simulação (molas + vento +
   gravidade + colisores).
4. UVs são **procedurais**: a textura inteira é esticada sobre a grade
   (`uv = (i/(hor−1), j/(ver−1))`). As UVs do BMD não participam.

Consequência de design: **silhueta de tecido = retângulo de grade com borda
inferior reta**. As caudas em ponta de raio do conceito não podem ser cloth —
no nativo as pontas são os meshes rígidos do BMD, e na capa Zeus são as seis
caudas + brasão autorais em `Zeus_Cape.bmd` (mesma regra do Manto Poseidon:
ponta pontiaguda é rígida, grade retangular é cloth).

## 3. Ligação da ferragem ao corpo (`RenderLinkObject`)

Em `ZzzCharacter.cpp` (~15690), quando o item rende equipado:

```cpp
case MODEL_CAPE_OF_EMPEROR:
case MODEL_CAPE_OF_OVERRULE:
    w->LinkBone = 19;
    RenderLinkObject(0.f, 0.f, 15.f, c, w, ..., true, bTranslate);
```

Dentro de `RenderLinkObject`, para `Type >= MODEL_CAPE_OF_EMPEROR` (~6845),
o produto de matrizes reconstruído pelo Poseidon e reutilizado aqui:

```
M1 = AngleMatrix(0, 90, 0) + T(-47, -7, 0)      # ramo de capas (~6845)
M2 = AngleMatrix(145, 0, 275) + T(0, 10, -30)   # concat de não-mão-direita
P_ferragem = BoneTransform[19] · M1 · M2 · M_osso0
```

Com o player no bind, o osso 19 fica em `(0, 0, 156.3)` e o produto final
posiciona a raiz da ferragem em ≈ `(0.84, 1.74, 79.9)` — atrás do tronco,
~77 unidades abaixo da base do pescoço (prova numérica e vista equipada nos
relatórios `cape-build-report.json` / `cape-equipped-review-report.json`).
A capa Zeus **reusa este link sem alterações** (mesma faixa, mesmo osso 19);
a diferença do Duel Master está no portão de cloth, não na ferragem.

## 4. A diferença de classe: o portão `bCloak` não abre para o Duel Master

O gate que cria qualquer cloth (~9313–9322) é:

```cpp
bool bCloak = false;
if ((c->Class == CLASS_DARK || gCharacterManager.GetBaseClass(c->Class) == CLASS_DARK_LORD
    || gCharacterManager.GetBaseClass(c->Class) == CLASS_RAGEFIGHTER) && o->Type == MODEL_PLAYER)
{
    bCloak = true;
}
```

Classes (`_enum.h:3240-3267`, `CharacterManager.cpp:165-190`):
`CLASS_DARK = 3` é a base **Magic Gladiator**; `CLASS_DUELMASTER` (14) mapeia
para `CLASS_DARK` no `GetBaseClass`; `CLASS_LORDEMPEROR` mapeia para
`CLASS_DARK_LORD`. Portanto, hoje:

| Personagem | `bCloak`? | Cloth recebido |
| --- | --- | --- |
| Dark Lord / Lord Emperor | sim (base DARK_LORD) | 4 ou 6 grades do Manto do Imperador |
| Ragefighter / Temple Night | sim (base RAGEFIGHTER) | 1 ou 3 grades próprias |
| Magic Gladiator (base, `c->Class == CLASS_DARK`) | sim | **ramo genérico**: 1 grade 75×120 `ROBE` (sem relação com capas de imperador) |
| **Duel Master (`CLASS_DUELMASTER`)** | **NÃO** — não é `CLASS_DARK` literal e o `GetBaseClass` é `CLASS_DARK`, que não casa com `DARK_LORD` nem `RAGEFIGHTER` | **nenhum** — sem cloth, sem saia, nada |

Ou seja: **nenhum membro da família CLASS_DARK recebe cloth de capa hoje**, e
um Duel Master com a capa Zeus renderizaria apenas a ferragem rígida
(BMD no osso 19 pelo link da seção 3), sem o tecido. Para a capa funcionar no
Duel Master, a onda de integração precisará:

1. **Abrir o gate** (~9317): incluir
   `gCharacterManager.GetBaseClass(c->Class) == CLASS_DARK` — cobre MG e
   Duel Master de uma vez (o MG base já passa por `c->Class == CLASS_DARK`,
   mas hoje cai no ramo genérico).
2. **Case próprio no bloco de cloth** (novo `else if
   (GetBaseClass(c->Class) == CLASS_DARK)` em ~9844/9936/9964): grades
   `CPhysicsCloth::Create` com parâmetros próprios da capa Zeus — o
   precedent é o `pCloth[2]` do imperador (osso 19, âncora (0, 8, 10),
   10×10, 180×180, CURVED | SHORT_SHOULDER | HEAVY | MASK_ALPHA) e,
   opcionalmente, dois painéis laterais no molde `pCloth[4]/[5]`. **Sem
   grade de saia**: o portão de saia compara skins DL/LE literalmente
   (~9903) e o design do set não tem saia na capa.
3. **Recriação por troca de equipamento**: o ciclo de destruição/recriação
   (~9986–10125) e os `DeleteCloth` de troca de asa/capa
   (`WSclient.cpp:2825/2841`) comparam `MODEL_CAPE_OF_EMPEROR` etc.
   literalmente — o ID novo precisa entrar nessas listas (ou num case novo
   equivalente) para o cloth ser destruído/recriado ao desequipar.
4. **Renderização do cloth** (~10081–10134): o skip da grade `i == 2` quando
   a asa não é capa compara tipos literais — o case novo deve tratar o ID
   Zeus na mesma família.

## 5. Portões e gatilhos que um ID novo precisa abrir

Renomear/reempacotar o BMD **não ativa nada** — cada trecho compara IDs
exatos:

1. **Carga**: `ZzzOpenData.cpp:1117`
   `gLoadData.AccessModel(MODEL_CAPE_OF_EMPEROR, L"Data\\Item\\", L"DarkLordRobe02")`
   + `:1368` `OpenTexture(MODEL_CAPE_OF_EMPEROR, L"Item\\")` — um ID novo
   precisa das duas entradas (modelo + pasta de textura).
2. **Resolução do slot de asas**: o equipamento de grupo 7 chega por
   `WSclient.cpp` (case 7, `c->Wing.Type = MODEL_ITEM + Type`, ~2837) e pelo
   switch de asas de `ZzzCharacter.cpp` (~12740-12780, onde
   `MODEL_CAPE_OF_LORD/FIGHTER/OVERRULE` têm cases e o imperador entra pelo
   braço default `MODEL_WING + Type - 1`). O ID Zeus precisa de braço próprio
   ou de faixa auditada nesses switches.
3. **Cloth**: seções 2 e 4 — case próprio com `Create`, `SetWindMinMax`,
   `AddCollisionSphere` e `DeleteCloth` escolhidos para a nova silhueta.
4. **Link da ferragem**: o ramo `Type >= MODEL_CAPE_OF_EMPEROR` dentro de
   `RenderLinkObject` (~6845) captura por faixa de ID — qualquer ID novo
   acima desse valor herda a matriz do manto do imperador; IDs abaixo caem em
   outros ramos. A integração decide: reusar a faixa (mesma matriz, caminho
   da capa Zeus) ou abrir `case` com matriz própria.
5. **bCloak/wiring**: os ramos de render (`ZzzCharacter.cpp` ~9840 e ~15690)
   e de inventário (`ZzzInventory.cpp:8387`) comparam `MODEL_CAPE_OF_EMPEROR`
   literalmente; NPCs da cidade recebem o manto por ID fixo em
   `GMNewTown.cpp:547` (apenas referência de uso).
6. **Slots de textura do cloth**: `BITMAP_ROBE + 6/+9/+10`,
   `BITMAP_DARK_LOAD_SKIRT`, `BITMAP_DARKLOAD_SKIRT_3RD`, `BITMAP_MANTOE`,
   `BITMAP_NCCAPE`, `BITMAP_MANTO01` são globais carregadas em
   `ZzzOpenData.cpp`. Uma capa com atlas próprio (`Zeus_Blue.jpg` para o
   tecido) ou re-aponta esses `LoadBitmap` (afetando todo dono do slot) ou
   recebe índices novos no case próprio do item 3.

## 6. O que a integração precisará provar (checklist)

- [ ] Malha: ferragem rígida em **um osso** (`collar`, índice 0, 1 pose) —
      nenhum skinning além do osso 0; caudas e pontas de raio rígidas
      (contrato já cumprido pelo protótipo: 1 osso, roundtrip < 1e-6).
- [ ] Portão de classe: `bCloak` estendido à família `CLASS_DARK` + case
      próprio de cloth para a capa Zeus no Duel Master (seção 4) — sem isso
      a capa sai "sem tecido" para a classe-alvo.
- [ ] Sem saia: confirmar que nenhum grade liga ao osso 18 para o set Zeus
      (saia é exclusiva das skins DL/LE; o faldão do Zeus vive na Calça).
- [ ] Região de cloth reservada: a massa de tecido NÃO vai no BMD; a gola do
      protótipo já cobre/contorna a linha presa da grade principal (linha
      reta de largura 0.6 × 180 = 108 centrada na âncora do osso 19, 8
      unidades atrás do pescoço) e os painéis rígidos ficam em y ≥ 33,
      fora do varrido do cloth e das esferas r=25..30 dos ossos 17 e 2.
- [ ] Orçamento: ferragem nativa tem 192 tris; o protótipo Zeus usa 7.986
      (gola estruturada + brasão + 6 caudas) — dentro do limite por mesh do
      cliente (≤2200 tris/mesh, ≤10000 vértices, ≤50 meshes).
- [ ] Textura: o tecido é uma imagem inteira esticada na grade (UV
      procedural); desenhar o atlas `Zeus_Blue.jpg` pensando nisso (frente/
      verso/hem coerentes), além das UVs 0..1 da ferragem.
- [ ] Poses: provar parado/andar (a prova equipada usa `PLAYER_STOP_SWORD` e
      `PLAYER_WALK_SWORD` do próprio `player.bmd`); montado/cavalo e skill
      ficam para a onda de integração, junto de vento/colisão reais, que o
      preview Blender não simula.

## 7. Lacunas que este estudo não fecha

- Simulação física real (vento, molas, colisão) só existe no runtime; o
  protótipo valida matemática de encaixe e limites de malha, não drapeado.
- O `case`/faixa de `RenderLinkObject`, os switches do slot de asas e os
  índices de textura são decisões de integração (reusar vs. abrir caso
  novo) — a escolha muda a matriz de ligação e os slots globais.
- IDs de item, requisitos e categoria são backend (**update 249 a reservar**
  na coordenação de ondas); nada desta lane cria ID ou rede.
- Género/variantes (ramos irmãos CLASS_RAGEFIGHTER etc.) fora do escopo.
