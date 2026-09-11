# Equipamentos MU: composição, animação e efeitos no runtime

Auditoria de fonte local em 11/09/2026. Complementa o [catálogo de assets](catalog.md), sem alterar o jogo, o servidor, itens de jogadores ou a VPS. Abrange armaduras e também a ligação visual de armas, escudos, asas e acessórios.

**Como ler:** “verificado” abaixo significa comportamento identificado no código, não uma captura validada ingame de cada item. Formas/paletas dos arquivos, existência no carregador, definição no servidor e disponibilidade para jogadores são quatro evidências diferentes. Uma textura azul não garante resultado azul: iluminação, materiais e efeitos se somam a ela.

## Arquivos de referência preservados

- [equipment-effects-index.json](equipment-effects-index.json): índice lexical de todos os `.cpp`, `.h`, `.hpp` e `.inl` em `src/source`, com caminhos, SHA-256, linhas, trechos e referências cruzadas `MODEL_*`, `BITMAP_*` e `SOUND_*`. Inclui também monstros/NPCs: é uma base de busca abrangente, não uma lista filtrada de itens disponíveis.
- [build-runtime-index.mjs](build-runtime-index.mjs): extração reproduzível, sem dependências externas e sem executar código do jogo. Só grava o JSON desta documentação.
- `playerActions` no JSON: declarações de ações `PLAYER_*`, com linha e aliases explícitos. Não assume índices numéricos depois de condicionais de compilação.
- `armorMaterialTint` no JSON: extração do mapa de tintas de `PartObjectColor`, inclusive multiplicadores RGB e cor padrão. Não são cores amostradas das texturas.

O índice é lexical, não um compilador C++ nem um grafo completo de chamadas. Proximidade entre um `MODEL_*` e uma chamada de efeito **não prova** que o efeito pertence àquele item: é necessário ler o `if`/`switch` envolvente. Expressões como `MODEL_ARMOR + i` continuam expressões; o catálogo de assets resolve os casos de loader. Comentários inline, macros condicionais e referências não relacionadas a equipamentos podem aparecer. As contagens e hashes atuais estão no próprio JSON, evitando uma segunda fonte manual.

## 1. Identidade e composição de uma família

No servidor, a identidade é `(Group, Number)`. `Number` é único dentro de um grupo; `QualifiedCharacters`, `Requirements`, `MaximumSockets`, opções e grupos de conjunto pertencem à definição de backend. O nome da malha **não** concede uma classe, socket ou bônus. Fontes: [ItemDefinition.cs:20](C:/_wt-celestial-server/src/DataModel/Configuration/Items/ItemDefinition.cs:20), [ArmorInitializerBase.cs:368](C:/_wt-celestial-server/src/Persistence/Initialization/Items/ArmorInitializerBase.cs:368).

| Componente | Grupo do item | Modelo convencional no cliente |
|---|---:|---|
| Espada / machado / mace-scepter / lança / arco | 0 / 1 / 2 / 3 / 4 | `MODEL_ITEM + Group * 512 + Number` |
| Cajado / escudo | 5 / 6 | `MODEL_STAFF + Number` / `MODEL_SHIELD + Number` |
| Helm / armor / pants / gloves / boots | 7 / 8 / 9 / 10 / 11 | `MODEL_HELM`, `MODEL_ARMOR`, `MODEL_PANTS`, `MODEL_GLOVES`, `MODEL_BOOTS` + `Number` |
| Asas e outros itens do grupo | 12 | Não assumir que todo item do grupo é uma asa |
| Helpers, anéis, pendants e outros | 13 | Identificar também slot/definição; não basta o grupo |

`MAX_ITEM_INDEX` é 512 nesta fonte: [_define.h:354](C:/_wt-celestial-client/src/source/Core/Globals/_define.h:354). Os intervalos/símbolos vêm de [_enum.h:1457](C:/_wt-celestial-client/src/source/Core/Globals/_enum.h:1457). O parser de aparência usa a fórmula por grupo/número: [ZzzCharacter.cpp:13089](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:13089).

Armaduras com mesmo `Number` formam a família de cinco grupos, mas nem toda família tem cinco peças: há conjuntos de Magic Gladiator sem helm e Rage Fighter sem luva de armadura. O backend agrupa armaduras pelo número, com regras de conjunto separadas: [ArmorInitializerBase.cs:56](C:/_wt-celestial-server/src/Persistence/Initialization/Items/ArmorInitializerBase.cs:56). As tabelas Season 6 com nomes/requisitos estão em [Armors.cs:65](C:/_wt-celestial-server/src/Persistence/Initialization/VersionSeasonSix/Items/Armors.cs:65); as ocorrências do catálogo devem ser lidas por versão/origem, não misturadas como se fossem uma configuração instalada única.

### O nome do arquivo não segue uma fórmula única

`OpenPlayers` é o mapa autoritativo de carregamento da armadura. Exemplos que impedem deduzir todos os arquivos como `ArmorMale(Number+1)`:

| Faixa/família | Convenção / exceção verificada |
|---|---|
| Famílias 0–9 | `*Male01..10` |
| Famílias 10–14 | `*Elf01..05` |
| Grand Soul, família 18 | Pants usa `t_PantMale19`; outras peças seguem o grupo correspondente |
| Divine, família 19 | Usa `*MaleTest20` |
| Famílias 29–33 | `HDK_*Male01..05`; Venom Mist é `HDK_*Male02` |
| Famílias 34–38 | `CW_*Male01..05`; há famílias sem helm |
| Família Phoenix Soul, 73 | `HelmMale74`, `ArmorMale74`, `PantMale74`, `BootMale74` |
| Celestial | Cinco `Celestial_*.bmd` explícitos, sem alias para um set antigo |

Fontes: [ZzzOpenData.cpp:212](C:/_wt-celestial-client/src/source/Engine/Object/ZzzOpenData.cpp:212), [ZzzOpenData.cpp:246](C:/_wt-celestial-client/src/source/Engine/Object/ZzzOpenData.cpp:246), [ZzzOpenData.cpp:297](C:/_wt-celestial-client/src/source/Engine/Object/ZzzOpenData.cpp:297), [ZzzOpenData.cpp:373](C:/_wt-celestial-client/src/source/Engine/Object/ZzzOpenData.cpp:373). O catálogo de assets, não esta tabela de exemplos, contém a enumeração das famílias.

## 2. Esqueleto, movimento e variantes corporais

**A armadura veste a animação do personagem.** `OpenPlayers` carrega `Data/Player/Player.bmd` como `MODEL_PLAYER`; o código considera erro esse arquivo ter malhas (`NumMeshs > 0`). A superfície visível vem das peças, enquanto o personagem fornece o esqueleto/ações. As peças são transformadas pelas matrizes de ossos do personagem em `RenderPartObject`. Fontes: [ZzzOpenData.cpp:179](C:/_wt-celestial-client/src/source/Engine/Object/ZzzOpenData.cpp:179), [ZzzObject.cpp:311](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:311), [ZzzObject.cpp:10912](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:10912).

Consequência para modelagem: copiar só o contorno ou uma textura não basta. Índices de bones, parentagem, pose-base e coordenadas locais das peças precisam corresponder ao rig utilizado. Malhas de helm, ombro, mãos e pés podem parecer corretas isoladas e falhar ao animar. Um BMD de armadura com uma única ação não significa “personagem sem animação”.

| Caso | Comportamento verificado / ponto de entrada |
|---|---|
| Corpo sem equipamento | `HelmClass`, `ArmorClass`, etc.; famílias `Class2` e `Class3` para evoluções, com exceções por classe em `OpenPlayers` |
| Summoner com Vine/Silk (10/11) | Troca para `MODEL_*2`, arquivos `*ElfC01/02`; não é simplesmente a mesma malha em outra escala |
| Rage Fighter com Leather/Scale/Brass/Plate (5/6/8/9) | Remapeia para `*Monk01..04`; cores retornam ao tipo original via `OrginalTypeCommonItemMonk` |
| Dark Lord com helm 0/5/6/8/9 | Troca para variante `MaskHelmMale*` |
| Rage Fighter Sacred/Storm Hard/Piercing/Phoenix Soul | Modelos de peito próprios para inventário, distintos do peito equipado |
| Calças Grand Soul / Divine / Dark Soul | `RenderPartObject` adiciona simulação de tecido com configurações, bones e colisores específicos |

Fontes: [ZzzCharacter.cpp:9728](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:9728), [MonkSystem.cpp:109](C:/_wt-celestial-client/src/source/GameLogic/Social/MonkSystem.cpp:109), [MonkSystem.cpp:173](C:/_wt-celestial-client/src/source/GameLogic/Social/MonkSystem.cpp:173), [ZzzObject.cpp:10924](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:10924).

A escala também muda: no mundo, `SetCharacterScale` usa Wizard/Knight 0,90 ou 0,93; Elf 0,88 ou 0,86; MG 0,95; DL 0,92; Summoner 0,90; RF 1,03, conforme o ramo de skin. Na seleção, RF 1,35 e demais 1,20. Não comparar altura de screenshots sem igualar cena, classe e câmera. [ZzzCharacter.cpp:12307](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:12307).

### Ações, velocidade e repetição

- Idle, caminhada e corrida são escolhidos por classe, arma de uma/duas mãos, cajado, arco/crossbow, montaria, voo/nado e estado do personagem — não pelo nome da armadura. Pontos de entrada: [SetPlayerStop:301](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:301), [SetPlayerWalk:516](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:516).
- Existem ações distintas de ataque, magia, defesa, dano, morte, descanso/sentar, montaria e habilidades RF. A enumeração está preservada em `playerActions` do índice; o arquivo BMD determina keyframes realmente presentes. [_enum.h:2928](C:/_wt-celestial-client/src/source/Core/Globals/_enum.h:2928).
- `OpenPlayers` atribui velocidades iniciais; `SetPlayerWalk`/`SetAttackSpeed` as alteram em runtime. Stun/sleep interrompem avanço; buffs e certas habilidades alteram a cadência. [ZzzOpenData.cpp:392](C:/_wt-celestial-client/src/source/Engine/Object/ZzzOpenData.cpp:392), [SetAttackSpeed:875](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:875), [CharacterAnimation:2575](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:2575).
- `PlayAnimation` avança `AnimationFrame += Speed * FPS_ANIMATION_FACTOR`. Não converter `PlaySpeed=0.3` diretamente para “0,3 fps”. A duração depende também de quantidade de keys, fator temporal, estado e overrides. Particularidade: neste código `Actions[action].Loop == true` **prende no último quadro**; o ramo `false` faz retorno por módulo. Verificar a semântica, não traduzir literalmente o nome do campo. [ZzzBMD.cpp:709](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:709).

## 3. Armas, escudos, asas e acessórios

Armas/escudos normalmente carregam de `Data/Item`. Também há exceções: Rune Blade usa `Sword32`, Kundun Staff usa `Staff12`, Divine Scepter usa `Saint`, além de aliases e modelos separados para RF. Conferir **ID + loader + hash**, não só o rótulo comercial. [OpenItems:693](C:/_wt-celestial-client/src/source/Engine/Object/ZzzOpenData.cpp:693).

`RenderLinkObject` liga o modelo do item ao `PART_t.LinkBone`, compõe a matriz do dono e permite animação própria do item. Armas podem ir às costas em zona segura. Asas normalmente usam bone 47; Cape of Emperor/Overrule usa 19; a decisão e os offsets devem acompanhar a malha. [RenderLinkObject:6745](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:6745), [composição de matriz:7067](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:7067), [RenderCharacterBackItem:15506](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:15506).

**Asas têm movimento próprio além da pose de voo do personagem.** O ramo comum aplica `PlaySpeed=1` ao voar (Wing of Storm: 0,5), e 0,25 fora dessas ações; o BMD da asa é animado depois de receber a transformação de ligação. Há exceções de action/segurança/mapa. Uma asa estática no Blender não comprova falta de animação no jogo, e uma animação no BMD não comprova que o loader seleciona a action correta. [ZzzCharacter.cpp:7100](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:7100), [ZzzCharacter.cpp:15665](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:15665).

Anéis e pendants precisam de modelo/textura/visualização de inventário, mas não são automaticamente desenhados no corpo como roupa: o contrato comum de aparência remota transmite mãos, cinco peças, asas e pet — não slots de ring/pendant. Visual no corpo requer regra de apresentação própria; transformation rings/pets/efeitos de conjunto não devem ser confundidos com renderizar uma joia no dedo. Fonte do contrato: [AppearanceSerializerExtended.cs:94](C:/_wt-celestial-server/src/GameServer/RemoteView/AppearanceSerializerExtended.cs:94).

### Trails e som não pertencem apenas ao BMD

`CreateWeaponBlur` escolhe mão, região temporal do ataque, tipo de blur e cores/mapeamentos conforme arma e skill. Para reproduzir um trail, preservar também os pontos de origem/destino e a janela da animação, não apenas a textura. [ZzzCharacter.cpp:3880](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:3880), [ZzzEffectBlurSpark.cpp:26](C:/_wt-celestial-client/src/source/Render/Effects/ZzzEffectBlurSpark.cpp:26).

Há sons selecionados pelo ataque: bow/crossbow, algumas armas mágicas, Light Saber/Spear e brandish comum; skills e pets possuem caminhos adicionais. O índice guarda os `SOUND_*` e chamadas de áudio, mas não os atribui indevidamente ao set inteiro. [ZzzCharacter.cpp:1365](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:1365).

## 4. O que determina cor, brilho e transparência

O resultado combina **geometria e UV → textura base → máscara/estado por mesh → luz do personagem/mapa → passes por upgrade/categoria → partículas/sprites/trails**. Uma amostra de paleta do arquivo é apenas a camada de textura base.

### Materiais e scripts no nome da textura

`RenderBody` usa `BlendMesh`, brilho, UV scrolling, mesh oculto e scripts por textura. `TextureScriptParsing` interpreta caracteres após o primeiro `_`: `R` bright, `H` oculto, `S` stream e `N` noneBlend. O parser tem janela curta e retorna falso ao encontrar certos caracteres não reconhecidos; não classificar todo arquivo com underscore como script. **Preservar nome e caixa exatos da textura** durante uma remodelagem. Fontes: [TextureScript.cpp:13](C:/_wt-celestial-client/src/source/Render/Sprites/TextureScript.cpp:13), [ZzzBMD.cpp:3344](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:3344), [ZzzBMD.cpp:2216](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:2216).

Os passes podem usar `RENDER_TEXTURE`, `RENDER_CHROME/2/3/4`, `RENDER_METAL`, `RENDER_BRIGHT`, lightmap, cor e sombra. Não equivalem a materiais PBR convencionais com metallic/roughness. Há texturas compartilhadas: `Effect/Chrome01.jpg`, `Chrome02.jpg`, `Chrome03.jpg`, `Shiny01..03.jpg`, `ring.jpg`, etc. A fonte dos loaders está em [ZzzOpenData.cpp:5513](C:/_wt-celestial-client/src/source/Engine/Object/ZzzOpenData.cpp:5513), e o índice permite consultar `BITMAP_CHROME` / `BITMAP_SHINY` para chegar a todos os usos.

### Upgrade normal: caminho genérico

| Nível visual recebido pelo renderer | Comportamento-base verificado |
|---|---|
| 0–2 | Textura/iluminação base |
| 3–4 | Modulação quente, aproximadamente `(L, 0,6L, 0,6L)` |
| 5–6 | Modulação fria, aproximadamente `(0,5L, 0,7L, L)` |
| 7–8 | Pass chrome + bright |
| 9–10 | Chrome + metal + bright |
| 11–12 | Chrome2 + metal + chrome, com variação de tint |
| 13–15 | Variante chrome4 + metal + chrome |

São ramos **genéricos**, não regras universais para itens com handler específico. O shader pode condensar os passes; sem suporte usa fallback multipass. O nível de detalhe limita a quantidade de passes e distância acima de 1200 unidades retorna ao material-base nesse trecho. [ZzzObject.cpp:10441](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:10441), [pipeline:10505](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:10505).

O servidor compacta o upgrade de aparência em faixas: 0–2→0, 3–4→1, 5–6→2, 7–8→3, 9–10→4, 11–12→5, 13–14→6, 15→7. O cliente reconstitui níveis 0/3/5/7/9/11/13/15. Portanto o visual remoto pode não distinguir todos os níveis consecutivos, mesmo que tooltip/atributos sejam diferentes. [ItemExtensions.cs:28](C:/_wt-celestial-server/src/GameServer/RemoteView/ItemExtensions.cs:28), [LevelConvert:12629](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:12629).

### Excellent, Ancient, socket e +15

- **Excellent:** o overlay clássico pulsa com `sin(WorldTime*0.002)`, alterando o RGB `(L,0.3L,1-L)` e usando textura + bright. Algumas peças RF/Summoner restringem o overlay a meshes específicos para não tingir a pele. [ZzzObject.cpp:10647](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:10647).
- **Ancient:** nesse ramo usa alpha oscilante e `CHROME3 | BRIGHT`; `PartObjectColor3` fornece azul `(0.1,0.6,1)` ou dourado `(1,0.7,0.2)` para exceções de família. É `else if` depois de Excellent: prioridade de apresentação não remove opções de backend. [ZzzObject.cpp:10707](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:10707), [PartObjectColor3:7020](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:7020).
- **Socket:** `MaximumSockets` é propriedade de definição; sementes/opções são dados do item. O contrato comum de aparência e `RenderPartObjectEffect` não recebem IDs das sementes: não foi encontrado ali “uma cor/partícula por socket”. Famílias socket podem ter malhas/tintas próprias pelo ID, o que é diferente de cada seed produzir um efeito visual. Isso não nega outros handlers/UI; consulte `item-option` no índice. [ItemDefinition.cs:124](C:/_wt-celestial-server/src/DataModel/Configuration/Items/ItemDefinition.cs:124), [AppearanceSerializerExtended.cs:134](C:/_wt-celestial-server/src/GameServer/RemoteView/AppearanceSerializerExtended.cs:134).
- **+15:** `NextGradeObjectRender` adiciona efeitos em armas e modelos auxiliares `MODEL_15GRADE_ARMOR_OBJ_*` por parte/bone. Não é apenas aumentar saturação. A chamada tem restrições de Chaos Castle/Cursed Temple. [ZzzObject.cpp:9437](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:9437), [ZzzCharacter.cpp:10165](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:10165).

### Tintas por família e efeitos de conjunto

A tabela completa derivada da função está em `armorMaterialTint` do JSON (grupos 7–11). Exemplos de multiplicadores RGB do pass especular, **não** a cor final/textura:

| Família `Number` | Nome de referência | Multiplicador RGB |
|---:|---|---|
| 1 | Dragon | 1,00 / 0,20 / 0,00 |
| 3 | Legendary | 0,00 / 0,50 / 1,00 |
| 13 | Spirit | 0,00 / 0,80 / 0,40 |
| 18 | Grand Soul | 1,00 / 1,00 / 1,00 |
| 29 | Dragon Knight | 0,50 / 0,40 / 0,30 |
| 30 | Venom Mist | 0,37 / 0,37 / 1,00 |
| 31 | Sylphid Ray | 0,30 / 0,70 / 0,30 |
| 32 | Volcano | 0,50 / 0,40 / 1,00 |
| 33 | Sunlight | 0,45 / 0,45 / 0,23 |
| 73 | Phoenix Soul | 0,50 / 0,80 / 0,90 |

Fonte: [PartObjectColor:6649](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:6649). Os meshes Monk retornam ao item original para aplicar essa tabela. IDs sem caso explícito caem na cor padrão da função; isso não autoriza criar itens só atribuindo um ID livre.

`CheckFullSet` verifica identidade das peças e nível mínimo visual, com exceções para MG/RF. `RenderCharacter` acrescenta sprites nas mãos/braços e partículas de conjunto conforme detalhe gráfico. Para Dragon Knight/Venom Mist/Sylphid Ray/Volcano/Sunlight/Aura, há uma camada `BITMAP_WATERFALL_2` quando `EquipmentLevelSet > 9`, com RGB próprios. **Venom Mist é explicitamente `(0.1,0.1,0.9)` nessa camada**, diferente da tinta especular acima. [CheckFullSet:5505](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:5505), [efeitos de conjunto:11139](C:/_wt-celestial-client/src/source/Engine/Object/ZzzCharacter.cpp:11139).

### Celestial Ultimate: caminho autoral separado

O Celestial preserva materiais por nomes `Celestial_Gold.jpg`, `Celestial_Ivory.jpg`, `Celestial_Sapphire.jpg` e `Celestial_Emissive.jpg`; aplica chrome suave em ouro/marfim e adição emissiva em safira/luz. Intensidade varia com +0..+15, pulso e detalhe. `CanRenderLegacyGradeTint` exclui esses modelos do overlay Excellent/Ancient clássico, preservando a identidade dourada/marfim em vez da alternância vermelho/azul — **as opções do item continuam no backend**. Halo usa bone 47 da asa autoral, sprite, rotação e pulso com limites de distância/visibilidade. [CelestialShimmer.cpp:18](C:/_wt-celestial-client/src/source/Render/Models/CelestialShimmer.cpp:18), [CelestialAppearance.cpp:29](C:/_wt-celestial-client/src/source/Render/Models/CelestialAppearance.cpp:29), [ZzzObject.cpp:54](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:54).

## 5. Onde procurar cada componente de um futuro equipamento

| Componente / responsabilidade | Fonte de entrada verificada |
|---|---|
| Identidade e classificação | Backend `ItemDefinition`, inicializadores da versão; cliente `_enum.h` |
| Arquivo do modelo e textura | `ZzzOpenData.cpp`: `OpenPlayers`, `OpenItems`, `OpenPlayerTextures`, `OpenItemTextures` |
| Variante equipada por classe | `ZzzCharacter.cpp`: laço BodyPart em `RenderCharacter`; `MonkSystem.cpp` |
| Pose-base e animação do personagem | `Player.bmd`; `SetPlayerStop`, `SetPlayerWalk`, `SetPlayerAttack`, `SetPlayerMagic`, `SetAttackSpeed` |
| Animação de asas/arma e attachment | `RenderLinkObject`, `RenderCharacterBackItem`, `BMD::Animation`, `BMD::PlayAnimation` |
| Superfície / meshes especiais | `RenderPartObjectBody`, `RenderPartObjectBodyColor`, `RenderPartObjectBodyColor2`, `BMD::RenderBody/RenderMesh` |
| Glow por upgrade/categoria | `RenderPartObjectEffect`, `PartObjectColor/2/3`, `NextGradeObjectRender` |
| Aura do conjunto | `CheckFullSet`, ramo `fullset` em `RenderCharacter`; Celestial tem apresentação adicional própria |
| Partículas/sprites/joints | `Render/Effects/ZzzEffectParticle.cpp`, `zzzeffectsprite.cpp`, `ZzzEffectJoint.cpp` e chamadas de criação no índice |
| Trails | `CreateWeaponBlur`, `ZzzEffectBlurSpark.cpp` |
| Movimento/vida de efeitos complexos | `ZzzEffect.cpp`, `EffectRegistry.cpp`, `Render/Effects/Behaviors/*` |
| Tecidos / partes extras | `RenderPartObject` + física; `Character/CSParts.cpp` |
| Sons | `SOUND_*`, chamadas `PlayBuffer` e loaders de áudio no índice |
| Inventário, chão e personagem | Handlers diferentes; preservar todas as variantes e testar as três apresentações |

Para cada VFX, salvar **tipo + subtipo, sprite/mesh/texture, cor, escala, alpha, emissor/bone, offset, frequência, lifetime, rotação, limites e condição de ativação**. Esses valores podem estar distribuídos entre o local que cria e o handler que move/renderiza o efeito. Uma chamada `CreateParticle(..., 3)` não descreve sozinha o comportamento do subtipo 3.

## 6. Consulta e atualização reproduzíveis

Na raiz do cliente:

```powershell
node docs/art/armor-reference/build-runtime-index.mjs
$runtime = Get-Content docs/art/armor-reference/equipment-effects-index.json -Raw | ConvertFrom-Json
$runtime.counts
$runtime.playerActions.actions | Select-Object name, explicitExpression, line
$runtime.armorMaterialTint.mappings | Where-Object itemNumber -eq 30
```

Exemplo: localizar todas as ocorrências explícitas do Venom Mist Boots e, separadamente, todos os usos/carregamentos da família de partículas encontrada:

```powershell
foreach ($reference in $runtime.byModelSymbol.MODEL_VENOM_MIST_BOOTS) {
    $source = $runtime.files[$reference[0]]
    $event = $source.events[$reference[1]]
    [pscustomobject]@{ Path = $source.path; Line = $event.line; Code = $event.code }
}
foreach ($reference in $runtime.byResourceSymbol.BITMAP_WATERFALL_2) {
    $source = $runtime.files[$reference[0]]
    $event = $source.events[$reference[1]]
    [pscustomobject]@{ Path = $source.path; Line = $event.line; Code = $event.code }
}
```

Ler o bloco de controle na fonte antes de associar o efeito a uma peça. Para calcular a transformação real, seguir `LinkBone`/`TransformPosition` até o BMD correto; bone 47 do personagem não significa automaticamente bone 47 da asa. Usar SHA-256 por arquivo para detectar fonte modificada depois da extração; linhas podem se deslocar entre revisões.

## Limites e checklist visual

Esta auditoria não fez login nem equipou todos os itens, não mediu FPS/lifetime na máquina do jogador e não confirmou drops, lojas ou configuração persistida. Não afirmar “todo set disponível no servidor” a partir de arquivos encontrados. A enumeração é de fontes/assets auditados, com cobertura indicada no catálogo.

Antes de reutilizar um estilo, validar frente/costas/laterais, silhueta sem glow, vista isométrica e inventário; depois idle, walk, run, ataques, cast, hit, death, wings/mount. Repetir com +0, +7, +9, +11, +13 e +15, Excellent/Ancient quando aplicáveis, detalhes gráficos reduzidos, outra iluminação/mapa, distância e cloak. Em RF/Summoner/DL, validar também a variante corporal. Essa é a etapa que distingue “referência preservada no código” de “aparência e animação aprovadas ingame”.
