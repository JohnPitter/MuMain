# Contrato de cloth do Manto — estudo para a integração futura

Estudo prático de como o `Data/Item/DarkLordRobe02.bmd` (Manto do Imperador,
`MODEL_CAPE_OF_EMPEROR = MODEL_WING + 40`) se conecta ao runtime de cloth do
cliente Celestial. Nada aqui altera código C++, IDs, rede ou runtime: é o mapa
do que a onda de integração precisará reproduzir para um manto Poseidon com ID
próprio. Fontes: árvore `src/source` deste repositório (arquivos e linhas
citados), o BMD nativo congelado (`ultimate-base-downloaded.zip`, SHA-256 em
`native-reference-audit.json`) e o esqueleto `Data/Player/player.bmd`.

## 1. Estrutura do BMD nativo (ferragem rígida)

`native-reference-audit.json`, chave `emperor_cape`:

| Propriedade | Valor medido |
| --- | --- |
| Osso | 1 osso, índice 0, nome `collar`, pai `-1`, não-dummy |
| Pose | 1 ação, 1 quadro (`action_frames [1]`, `action_locks [false]`) |
| Bind do osso 0 | posição `(0, 1.032, 0)`, rotação `(0, 0, -π)` |
| Meshes | 3 meshes, 192 triângulos no total |
| Mesh 0 | `dl_redwings01.jpg`, 156 tris, bbox local x ±35.8, y −25.7..20.1, z 46.8..81.2 |
| Mesh 1 | `dl_redwings03.tga`, 20 tris, bbox local x ±69.8, y 5.1..57.0, z −66.5..55.9 |
| Mesh 2 | `dl_redwings02.tga`, 16 tris, bbox local x ±66.8, y 13.3..32.8, z −96.5..50.5 |

Leitura: o BMD carrega **apenas ferragem rígida** — gola/jugo e dois painéis
rígidos de "asas" que descem pelas costas. Não há malha de tecido no arquivo.
UVs dos meshes são mapeamento comum 0..1 sobre as texturas listadas.

## 2. O cloth NÃO vem do BMD — é gerado por código

O tecido é criado proceduralmente em `ZzzCharacter.cpp` (bloco `bCloak`,
`GetBaseClass(c->Class) == CLASS_DARK_LORD`, logo após ~linha 9838). O runtime
monta `numCloth = 6` grades de `CPhysicsCloth` **quando e somente quando**
`c->Wing.Type == MODEL_CAPE_OF_EMPEROR`. Nenhuma dessas grades lê vértice,
UV ou osso do BMD do manto; elas são grades retangulares
`(iNumHor × iNumVer)` construídas no espaço local de um osso do player:

| Grade | Osso | Âncora local (x,y,z) | Grade | Tamanho (L×A) | Textura (`BITMAP_ROBE`) | Flags principais |
| --- | --- | --- | --- | --- | --- | --- |
| `pCloth[0]` capa de ombro | 20 (cabeça/pescoço) | (0, 0, 20) | 6×5 | 30×70 | `ROBE+6` = `Player/iui06.jpg` | CURVED, RUBBER2, LIGHT, SHORT_SHOULDER, OPT_HAIR |
| `pCloth[1]` capa de ombro (blenda) | 20 | (0, 5, 18) | 5×5 | 30×70 | `ROBE+6` | idem, MASK_BLEND |
| `pCloth[2]` **capa principal** | 19 (base do pescoço) | (0, 8, 10) | 10×10 | 180×180 | `ROBE+9` = `Item/dl_redwings02.tga` | CURVED, SHORT_SHOULDER, HEAVY, MASK_ALPHA |
| `pCloth[3]` saia (só com armadura DL/LE) | 18 (peito alto) | (0, 10, −5) | 5×5 | 50×90 | `BITMAP_DARKLOAD_SKIRT_3RD` = `Player/dark3chima3.tga` | ALPHA, HEAVY, STICKED, SHORT_SHOULDER (+CURVED na recriação) |
| `pCloth[4]` painel lateral direito | 19 | (30, 15, 10) | 2×5 | 12×200 | `ROBE+10` = `Item/dl_redwings03.tga` | FLAT, COTTON, ALPHA |
| `pCloth[5]` painel lateral esquerdo | 19 | (−30, 20, 10) | 2×5 | 12×200 | `ROBE+10` | idem |

Colisores (`AddCollisionSphere`, esferas rigidadas a ossos do player):
`pCloth[0]/[1]`: duas esferas r=27 no osso 17 (peito), centros (∓10, 20, 20);
`pCloth[2]`: quatro esferas no osso 17, r=25 em (∓10, −10, −10) e r=27 em
(∓10, −10, 20); `pCloth[3]`: r=30 no osso 2 (cintura); `pCloth[4]/[5]`: r=30
no osso 2 e r=35 no osso 17.

### Como a grade é construída (`PhysicsManager.cpp`, `CPhysicsCloth::Create`)

1. Vértices da grade gerados no espaço do osso-âncora: largura em x centrada
   (`−0.5·fWidth .. +0.5·fWidth`), y fixo (20; 0 se STICKED), profundação em
   `−z` linha a linha.
2. `PCT_SHORT_SHOULDER`: a largura de cada linha escala de
   `RATE_SHORT_SHOULDER = 0.6` (topo) até 1.0 (base) — silhueta de manto que
   abre para baixo. `PCT_CURVED` recurva as linhas para trás (−10·fMove²).
3. A linha do topo (`SetFixedVertices`) é **presa** ao osso-âncora: primeira
   linha de vértices marcada `PVS_FIXEDPOS`, posicionada em `(fx±, fy, fz)`
   com largura `0.6·fWidth`. É a única ligação física com o esqueleto; o resto
   é simulação (molas + vento + gravidade + colisores).
4. UVs são **procedurais**: `RenderFace` emite `uv = (i/(hor−1), j/(ver−1))`,
   esticando a textura inteira sobre a grade. As UVs do BMD não participam.
5. Recriação dinâmica: a saia (`pCloth[3]`) é reconstruída em runtime quando a
   armadura equipada muda para a skin Lord Emperor (~linha 10060).

Consequência de design: **silhueta de tecido = retângulo de grade com borda
inferior reta**. Painéis pontiagudos e "asas" não podem ser cloth — no nativo
eles são os meshes rígidos 1 e 2 do BMD (que rendem atrás/da quadratic da
grade), e no Manto Poseidon precisam seguir o mesmo princípio: pontas em
geometria rígida do BMD, massa de tecido reservada às grades.

## 3. Ligação da ferragem ao corpo (`RenderLinkObject`)

Em `ZzzCharacter.cpp` (~15690), quando o manto rende equipado:

```cpp
case MODEL_CAPE_OF_EMPEROR:
case MODEL_CAPE_OF_OVERRULE:
    w->LinkBone = 19;
    RenderLinkObject(0.f, 0.f, 15.f, c, w, ..., true, bTranslate);
```

Dentro de `RenderLinkObject` (ramo `Link == true`), para
`Type >= MODEL_CAPE_OF_EMPEROR` (~6841):

```cpp
Vector(0.f, 90.f, 0.f, Angle); AngleMatrix(Angle, Matrix);
Matrix[0][3] = -47.f; Matrix[1][3] = -7.f; Matrix[2][3] = 0.f;
// + concatenação para itens não-mão-direita (~6855):
Vector(145.f, 0.f, 275.f, vNewAngle); AngleMatrix(vNewAngle, mNewRot);
mNewRot[0][3] = 0.f; mNewRot[1][3] = 10.f; mNewRot[2][3] = -30.f;
R_ConcatTransforms(Matrix, mNewRot, Matrix);
// e por fim (~7075):
R_ConcatTransforms(o->BoneTransform[f->LinkBone], Matrix, ParentMatrix);
```

O modelo do item é animado com `b->Animation(..., true, true, ParentMatrix)`
(~7120), que concatena a matriz do osso raiz do BMD após `ParentMatrix`
(`ZzzBMD.cpp`, ~281). Em resumo, a pose final da ferragem é:

```
P_ferragem = BoneTransform[19] · [T(−47,−7,0) · R_y(90°)] · [T(0,10,−30) · R(145°,0°,275°)] · M_osso0
```

Com o player no bind (`world_matrices` do `player.bmd`), o osso 19 fica em
`(0, 0, 156.3)` e sua base local aponta **X local para cima (+Z mundo)**, **Y
local para a frente (−Y mundo é frente)**, **Z local para +X mundo**. A
translação `(−47,−7,0)` portanto desce ~77 unidades ao longo da coluna e o
modelo é aplicado atrás do tronco. Verificação numérica desta reconstrução
com o BMD nativo: os meshes 1/2 caem em x −94..34, y −52..83, z 26..148
(painéis traseiros descendo até próximo dos joelhos), coerente com o manto
em jogo. Este mesmo produto de matrizes é a "bind" usada pelo protótipo
Poseidon (`poseidon_cape_rig.cape_root`), provada nos relatórios de roundtrip
e na vista equipada.

## 4. Portões e gatilhos que um ID novo precisa abrir

Renomear/reempacotar o BMD **não ativa nada** — cada trecho abaixo compara
IDs/modelos exatos:

1. **Carga**: `ZzzOpenData.cpp` `AccessModel(MODEL_CAPE_OF_EMPEROR, ...)` +
   `OpenTexture(MODEL_CAPE_OF_EMPEROR, L"Item\\")` — um ID novo precisa das
   duas entradas (modelo + pasta de textura).
2. **Cloth**: o bloco `bCloak` só cria as 6 grades no ramo
   `GetBaseClass == CLASS_DARK_LORD` **e** `c->Wing.Type == MODEL_CAPE_OF_EMPEROR`;
   um manto novo exige um `case` próprio com os parâmetros `Create(...)`,
   `SetWindMinMax` e `AddCollisionSphere` escolhidos para a nova silhueta.
3. **Link da ferragem**: o ramo `Type >= MODEL_CAPE_OF_EMPEROR` dentro de
   `RenderLinkObject` captura por faixa de ID — qualquer ID novo acima desse
   valor herda a matriz do manto do imperador; IDs abaixo caem em ramos de
   outros itens. A integração deve decidir: reusar a faixa (aceitando a mesma
   matriz de link) ou abrir um `case` com matriz própria.
4. **bCloak/wiring**: a flag de capa (`bCloak`) e os ramos de render
   (`ZzzCharacter.cpp` ~10081, ~15690) e inventário (`ZzzInventory.cpp` 8387)
   comparam `MODEL_CAPE_OF_EMPEROR` literalmente.
5. **Slots de textura do cloth**: os bitmaps `ROBE+6/+9/+10` e
   `BITMAP_DARKLOAD_SKIRT_3RD` são globais, carregados em `ZzzOpenData.cpp`
   517–522/5440–5443. Um manto com atlas próprio ou re-aponta esses
   `LoadBitmap` (afetando todo dono do slot) ou recebe `case` próprio no bloco
   de cloth com índices novos.

## 5. O que a integração precisará provar (checklist)

- [ ] Malha: ferragem rígida em **um osso** (`collar`, índice 0, 1 pose) —
      nenhum skinning além do osso 0; painéis pontiagudos rígidos.
- [ ] Região de cloth reservada: a massa de tecido NÃO vai no BMD; o BMD deve
      cobrir/contornar a linha presa da grade principal (linha reta de largura
      `0.6 × 180 = 108` centrada na âncora do osso 19, 8 unidades atrás do
      pescoço) — a gola do protótipo já nasce dessa medida.
- [ ] Colisão: gola e fechos dentro das esferas r=25..30 dos ossos 17/2;
      painéis rígidos fora do volume varrido das grades (y traseiro ≳ 30).
- [ ] Orçamento: ferragem nativa tem só 192 tris; o protótipo Poseidon assume
      8–12 mil tris (gola estruturada + painéis "asa") — dentro do limite por
      mesh do cliente (≤2200 tris/mesh, ≤10000 vértices, ≤50 meshes).
- [x] Textura: fabric de cloth é uma imagem inteira esticada na grade (UV
      procedural); desenhar `Poseidon_*` pensando nisso, além das UVs 0..1 da
      ferragem. **Declarado e atendido na integração (2026-09-11):** os masters
      `Poseidon_Black` (massa do manto) e `Poseidon_Pearl.jpg` (brilho
      perolado, acabamento branco platina aprovado) são acabamentos de
      **campo cheio**, autorados de propósito para funcionar **esticados na
      grade 0..1** do cloth procedural — o mesmo papel de `dl_redwings02.tga`
      no manto nativo (`RenderFace` emite `uv = (i/(hor−1), j/(ver−1))`; as
      UVs do BMD não participam). Na onda 3, as grades `pCloth[2]`/`[4]`/`[5]`
      de um ID Poseidon devem apontar o slot `BITMAP_ROBE` para essas texturas
      (ou derivadas delas), sem charts nem costuras de tile.
- [ ] Poses: provar parada/andar (a prova equipada usa `PLAYER_STOP_SWORD` e
      `PLAYER_WALK_SWORD` do próprio `player.bmd`); montado/cavalo e skill
      ficam para a onda de integração, junto de vento/colisão reais, que o
      preview Blender não simula.

## 6. Lacunas que este estudo não fecha

- Simulação física real (vento, molas, colisão) só existe no runtime; o
  protótipo valida matemática de encaixe e limites de malha, não drapeado.
- O ramo de `RenderLinkObject` por faixa de ID (`Type >= MODEL_CAPE_OF_EMPEROR`)
  precisa de decisão de integração (reusar vs. caso novo) — a escolha muda a
  matriz de ligação e, portanto, o ajuste fino da ferragem.
- Os slots globais de textura do cloth são compartilhados com os mantos
  nativos; sem um plano de slots dedicados, atlas próprio do manto vaza para
  os mantos DL originais.
- Género/variantes (capas de outras classes usam ramos irmãos
  CLASS_RAGEFIGHTER etc.) fora do escopo deste conjunto.
