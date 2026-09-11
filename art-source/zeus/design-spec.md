# Herdeiro de Zeus — versão mágica azul (Duel Master)

## Estado e autorização

Segundo conjunto autoral do projeto, irmão do Manto do Poseidon, para a evolução
**Duel Master** do Magic Gladiator. Referência visual autoritativa:
`scratchpad/concept-herdeiro-de-zeus-mg-20260911.png` (SHA-256
`7d5bd6e0370f792f06faa4048987433e163916ff3992e94762399c76b249d573`), prancha
“HERDEIRO DE ZEUS, versão mágica azul”. A prancha é referência visual; os
**atributos de exemplo impressos na arte não são regras** — nada de dano,
defesa, nível, bônus, slots, raridade ou efeitos funcionais foi adotado desta
lane. Requisitos/bônus reais serão escopo aprovado de backend em onda posterior.

Entregue nesta lane fundacional: especificação completa do set, auditoria das
bases nativas da evolução MG/Duel Master (`native-reference-audit.json`), estudo
do acabamento azul Legendary +15 Excellent
(`research/legendary-excellent-blue.md`) e os **protótipos das duas armas**
(Espada de Zeus e Bastão Celestial) com BMD de diagnóstico, renders e testes.
Na onda seguinte, as **4 peças de corpo** (Armadura com ombreiras incorporadas,
Calça, Luvas e Botas) foram modeladas como protótipos autorais sobre o rig
Class304, no molde Poseidon (build/verify/equipped + renders + testes). A onda
seguinte entregou **asas, pendant e anéis** como protótipos autorais: as Asas
Celestiais sobre o esqueleto Wing44 congelado (47 ossos, flap de 9 quadros
preservado e provado em ≥2 poses) e as três joias nos contratos Necklace02/
Ring02 (1 osso estático cada, nome nativo preservado). A onda atual completou
o set com a **capa**: ferragem rígida autoral no osso `collar` do precedente
`DarkLordRobe02` (link do osso 19 reconstruído), tecido reservado ao cloth
procedural do runtime e estudo do contrato em `cape-cloth-contract.md` —
incluindo a descoberta de que o portão `bCloak` do cliente **não abre para o
Duel Master** e precisará de case próprio na integração. Com isso as **11
definições do set estão prototipadas**.

## Classe e requisitos de enquadramento

- **Classe alvo:** Duel Master, evolução do Magic Gladiator (client
  `CLASS_DUELMASTER`, cujo `GetBaseClass` é `CLASS_DARK`; servidor
  `DuelMaster = 13`). Nível **400**, padrão Celestial/Poseidon.
- **Contrato visual nativo:** o cliente usa a família `Class3` com sufixo
  **304** (`{Helm,Armor,Pant,Glove,Boot}Class304.bmd`, auditados; 51 ossos,
  1 pose por peça, textura compartilhada `SkinClass304`).
- Flag de item “pode ser equipado por” no cliente: `require_class == 3`
  exibe “DualMaster” (typo do cliente; `_enum.h` e inventário auditados).
  Decisão de item real permanece no backend.

## Linguagem visual

Paleta do conceito: **Azul Celeste (principal)** e **Branco Platinado
(detalhes)**. Regra do dono: o branco dos detalhes usa **obrigatoriamente** o
branco platina aprovado — masters
`C:/_wt-platina-client/art-source/celestial/textures/masters/white-platina`
(`Gold.png` SHA-256 `a85d8f37…77d21b35`, `Ivory.png` SHA-256 `9105bc8a…08c79bfa`,
âncoras `#d7d3d4` sombra `#545156` highlight `#faf8f6`). O **acabamento do
azul** tem como referência o +15 Excellent do set **Legendary nativo**
(estudo em `research/legendary-excellent-blue.md`): base azul moderadamente
escura com highlights claros pintados, azul reativado pelos passes especulares
tingidos e overlay Excellent oscilante — nenhum azul vivo pintado que o
pipeline +15 apagaria. O **design** (silhueta/geometria) é o do conceito.

Motivos de família Zeus: pontas e cunhos de raio angular, estrela de múltiplas
pontas como ornamento axial, placas facetadas em “V”, energia elétrica azul
em canaletas. NÃO reutilizar curvas de onda/tridente do Poseidon nem auréola
e asas angelicais do Celestial. Preservar a estética clássica de MU: silhueta
forte em câmera isométrica, massa legível no inventário, efeitos discretos que
não escondam mãos/pés. Frente, costas e lados com detalhe real.

Materiais de prévia são procedurais PBR provisionais; os atlases finais
(`Zeus_Blue.jpg`, `Zeus_Platina.jpg`, `Zeus_Emissive.jpg`) **não existem ainda**
e são declarados como dependência ausente. BMDs desta pasta são diagnósticos:
**não copiar para um cliente** sem os atlases e a integração.

## Composição do set

**Decisão ombreira:** a prancha mostra painéis “Peitoral” e “Ombreira”
separados, mas o slot system de MU não tem peça de ombro independente. As
ombreiras são **incorporadas ao peitoral** (Armadura), skinadas nos ossos de
clavícula/ombro nativos (25/26/35/36 do rig Class304). Não existe peça
“Ombreira” na lista de definições.

**Decisão elmo/cabeça:** o painel “Rosto/Cabelo” **não é item** — o personagem
mantém cabeça e cabelo nativos do Duel Master. O conceito não mostra elmo; o
set **NÃO tem elmo**. Impacto: o conjunto veste **4 peças de corpo + capa**
(Armadura, Calça, Luvas, Botas + Capa) em vez do layout clássico de 5;
o painel de rosto/cabelo é direção de arte de apresentação da prancha. No +15
nativo a casca de cabeça (`MODEL_15GRADE_ARMOR_OBJ_HEAD`, osso 20) continua
anexando pelo sistema padrão — é efeito de upgrade, não peça do set.

| Definição | Silhueta e detalhes a construir | Vinculação / aceitação | Estado |
| --- | --- | --- | --- |
| Armadura (peitoral + ombreiras) | Peitoral facetado em “V” azul com canaletas de energia, ombreiras altas angulares incorporadas com fan de placas em ponta de raio, platina nos cunhos e esterno, escápulas em estrela | Ossos nativos 2/3/10/17/18/25/26/27/34/35/36 auditados no ArmorClass304; 8.470 triângulos | Protótipo autoral BMD/Blender |
| Calça | Lamelas angulares azul/platina, quadril e traseira completos, faldão frontal curto em pontas duplas (visual da prancha) no osso raiz 0, como no PantClass304 nativo | Ossos 0/2/3/4/10/11/17 do PantClass304; nunca soldar faldão às pernas; 5.476 triângulos | Protótipo autoral BMD/Blender |
| Luvas | Dedos segmentados, guarda dorsal com cristal azul, canaletas elétricas platina; palma usável | Ossos 27/28/29/36/37/38 do GloveClass304; empunhadura de espada/bastão; 2.708 triângulos | Protótipo autoral BMD/Blender |
| Botas | Caneleira alta facetada, joelheira em ponta de raio, solado definido, platina nos recortes | Ossos 4/5/11/12 do BootClass304; flexão de pé/estribo; 3.904 triângulos | Protótipo autoral BMD/Blender |
| Capa | Capa longa azul (tecido = cloth do runtime) com gola alta facetada platinada de 12 gomos, cristas em ponta de raio, brasão central de estrela de 8 pontas com raio emissivo e seis caudas em ponta de raio com canaletas-runas e gemas celestiais; fechos estrelados à frente; “Capa (visão traseira)” da prancha | Precedente cloth `DarkLordRobe02` (auditado como `emperor_cape`): ferragem rígida no osso `collar` via link do osso 19 (`RenderLinkObject`), tecido é grade procedural do runtime; `bCloak`/saia dependem de skin DL/LE — capa no Duel Master exige case próprio (ver `cape-cloth-contract.md`); 7.986 triângulos, 1 osso | Protótipo autoral BMD/Blender |
| Espada de Zeus (arma principal) | Espada longa, lâmina azul celeste energética com núcleo emissivo e gume branco-platina, guarda em “V” de raio, punho envolto azul, pommel com estrela | Mão direita, osso 33 (`knife_gdf`); âncoras `Grip/Core/Tip`; **protótipo BMD/Blender nesta lane** | Protótipo autoral BMD/Blender |
| Bastão Celestial (arma alternativa) | Cajado azul alto com ornamento estelar no topo, núcleo emissivo, anéis platina, ponta inferior acabada | Mão esquerda, osso 42 (`hand_bofdgne01`); âncoras `Grip/Core/Tip`; não é a espada reescalada | Protótipo autoral BMD/Blender |
| Asas Celestiais | Leque de lâminas facetadas azul celeste com espinhos e pontas platina, canaletas de tempestade emissivas em zigzag, cobertura de bases, borda inferior em pontas e crista de estrela de 8 pontas com gema coração no osso raiz | Esqueleto Wing44 congelado (47 ossos, nomes/índices/hierarquia exatos, flap de 9 quadros com trava de raiz preservado); pele de influência única nos ossos 0, 1–5 e 24–28; flap provado em 3 poses nativas (erro < 1,1e-4, laço fechado); 16.004 triângulos | Protótipo autoral BMD/Blender |
| Pendant Olho de Zeus | Amuleto com gema-olho azul celeste (íris emissiva e pupila platina) em moldura estelar platina de 8 pontas, raios laterais em zigzag, argola e corrente fina | Contrato Necklace02 exato: 1 osso estático `Tube04`, 1 quadro sem trava, tudo no osso raiz; 1.862 triângulos; âncoras ChainTop/Frame/Eye registradas | Protótipo autoral BMD/Blender |
| Anel da Tempestade | Aro platina com dois filetes gravados e canaletas emissivas, coroa de garras, raio em degraus com cristal azul alongado e faísca emissiva no topo | Contrato Ring02 exato: 1 osso estático `Tube01`, 1 quadro sem trava; 1.890 triângulos; silhueta alta (38,6 de envergadura vertical); âncoras Band/Gem | Protótipo autoral BMD/Blender |
| Anel da Sabedoria | Aro platina irmão do anterior com coroa circular de 8 pontas estelares, pérolas azuis e gema redonda com núcleo emissivo | Contrato Ring02 exato: 1 osso estático `Tube01`, 1 quadro sem trava; 2.408 triângulos; silhueta larga (27,3 de envergadura horizontal, distinta da Tempestade); âncoras Band/Gem | Protótipo autoral BMD/Blender |

São **11 definições** (4 corpo + capa + 2 armas + asas + pendant + 2 anéis) e
**todas estão prototipadas** como BMD/Blender autorais nesta pasta — a lista de
definições está completa. “Rosto/Cabelo” não conta como item. Escudo não aparece
na prancha e não foi inventado. Efeitos da prancha (Aura Celestial, Lâmina de
Raios, Rastro Celestial, Buff Ativo) são **exemplos ilustrativos** — FX reais
são onda de integração com orçamento de partículas e opções gráficas, não
promessa desta lane.

## Armas: regras de empunhadura do MG/Duel Master

O Magic Gladiator/Duel Master **empunha espada e bastão simultaneamente**: o
inventário aceita arma em ambas as mãos (espada/ceifadora à direita, inclusive
staff à esquerda; regra de dano duplo 55%/55% auditada em
`ZzzInventory.cpp:6043`). Âncoras nativas: `Weapon[0].LinkBone = 33`
(`knife_gdf`, mão direita) e `Weapon[1].LinkBone = 42` (`hand_bofdgne01`,
mão esquerda) — ramo default de `CreateCharacterPointer`
(`ZzzCharacter.cpp:12105/12106`). Contrato adotado:

- **Arma única:** Espada de Zeus na mão direita (osso 33) OU Bastão Celestial
  na mão esquerda (osso 42), como qualquer arma nativa.
- **Dupla:** espada à direita + bastão à esquerda; os protótipos exportam a
  origem de pega própria (`Grip` em 0,0,0, `Core`, `Tip`) no eixo longitudinal
  −Y do espaço da mão MU, mesma conversão Z-up→mão usada pelo Poseidon; o
  encaixe final em animação in-game é aceite da onda de integração.

## Animação, efeitos e runtime

- Peças de corpo seguirão o esqueleto e ações do `player.bmd` (60 ossos,
  284 ações auditados); nada de animação local por peça. Provar parado,
  caminhada, corrida, ataque, skill, sentado/montado, morte antes de integrar.
- Capa: ferragem rígida + cloth são sistemas distintos; o tecido não vai no
  BMD (as pontas de raio do conceito são geometria rígida — grade de cloth
  termina sempre em barra reta). O estudo `cape-cloth-contract.md` documenta
  o contrato completo e a diferença de classe: o portão `bCloak` do cliente
  só cria grades para bases DARK_LORD/RAGEFIGHTER (e o MG base, num ramo
  genérico) — o Duel Master exige abrir o gate e um case próprio de cloth na
  integração; saia é exclusiva das skins DL/LE e o set não a usa.
- Brilho: perfil de runtime proposto no estudo
  `research/legendary-excellent-blue.md` — especular azul celeste só nos
  materiais azuis, tint neutro no branco platina, overlay Excellent clássico,
  `NextGradeObjectRender` +15 nativo, qualidade 0 desliga, distância > 1200
  cai para base, sem emissão branca uniforme.
- Nenhuma mudança de runtime, IDs, rede ou regras de servidor nesta pasta.

## Reprodução (lane atual)

```powershell
python art-source/zeus/audit_native_references.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/zeus/build_zeus.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/zeus/verify_zeus.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/zeus/build_zeus_armor.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/zeus/verify_zeus_armor.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/zeus/zeus_armor_equipped.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/zeus/build_zeus_wings.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/zeus/verify_zeus_wings.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/zeus/build_zeus_jewels.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/zeus/verify_zeus_jewels.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/zeus/build_zeus_cape.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/zeus/zeus_cape_equipped.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/zeus/verify_zeus_cape.py
python -m unittest discover -s art-source/zeus -p 'test_*.py' -v
```

`build_zeus.py -- --skip-render`, `build_zeus_armor.py -- --skip-render`,
`build_zeus_wings.py -- --skip-render`, `build_zeus_jewels.py --
--skip-render` e `build_zeus_cape.py -- --skip-render` regeneram sem
renderizar. Saídas:

- `prototype/zeus-weapons.blend`: coleções `Zeus Sword` e `Zeus Staff` (origem
  de pega compartilhada; habilitar as duas sobrepõe as alternativas).
- `prototype/zeus-armor.blend`: coleções `Zeus Armor`, `Zeus Pant`, `Zeus
  Glove` e `Zeus Boot` sobre os esqueletos Class304 congelados (51 ossos
  preservados, 1 pose; skin de influência única por vértice; ossos usados =
  distribuição nativa auditada: Armadura 2/3/10/17/18/25/26/27/34/35/36 —
  incluindo os tassets de quadril nativos —, Calça 0/2/3/4/10/11/17, Luva
  27/28/29/36/37/38, Bota 4/5/11/12).
- `prototype/models/`: BMDs de diagnóstico **não instaláveis**.
- `prototype/renders/`: 6 PNGs de estúdio por arma; frente/lado/costas por
  peça de corpo (12) + 2 provas equipadas no mannequin com as matrizes reais
  do `player.bmd` (`Zeus_ArmorSet_equipped_*`) — renders Blender, não imagens
  do jogo.
- `prototype/build-report.json` e `prototype/saved-roundtrip-report.json`:
  orçamentos, âncoras, hashes e prova de roundtrip (gate 1e-6) das armas.
- `prototype/armor-build-report.json`, `armor-saved-roundtrip-report.json` e
  `armor-equipped-review-report.json`: orçamentos das peças (bandas ±20% sobre
  as medidas Poseidon: armadura 6.280–9.400, calça 4.500–6.740, luva
  2.400–3.610, bota 3.350–5.030; justificativa registrada no relatório),
  âncoras de encaixe por osso, roundtrip < 1e-6 e gates de prova equipada
  (nearest < 20, farthest < 65, deslocamento entre poses ≥ 3).
- `prototype/zeus-wings.blend` (coleção `Zeus Wings` sobre o rig Wing44),
  `wings-build-report.json` e `wings-saved-roundtrip-report.json`: orçamento
  12.000–20.000 (medido: 16.004 triângulos em 9 malhas), esqueleto de 47 ossos
  vs. ZIP congelado, roundtrip < 1e-6, flap de 9 quadros provado
  (frames 0/4/8, erro ≤ 1,1e-4, 94% dos vértices em movimento, laço fechado) e
  prova equipada no mannequin via osso 47 do player + offset Z 15
  (precedente RenderLinkObject).
- `prototype/zeus-jewels.blend` (coleções `Zeus Pendant`, `Zeus Ring_Storm`,
  `Zeus Ring_Wisdom`), `jewels-build-report.json` e
  `jewels-saved-roundtrip-report.json`: orçamentos (pendant 1.700–3.200,
  anéis 1.600–2.800; medido 1.862/1.890/2.408), contratos Tube04/Tube01,
  roundtrip < 1e-6, âncoras Band/Gem/ChainTop/Frame/Eye provadas na superfície
  exportada e silhuetas Tempestade (alta) vs. Sabedoria (larga) distintas.
- `prototype/renders/`: asas frente/lado/costas + 2 poses de flap + vista
  equipada no mannequin; pendant e anéis frente/lado/detalhe; capa
  frente/lado/costas + 2 provas equipadas (`Zeus_CapeSet_equipped_*`, com o
  fantasma translúcido do cloth como no manto do Poseidon — nunca exportado).
- `prototype/zeus-cape.blend` (coleção `Zeus Cape` no espaço bind do player),
  `cape-build-report.json`, `cape-equipped-review-report.json` e
  `cape-saved-roundtrip-report.json`: orçamento 6.000–10.000 (medido: 7.986
  triângulos em 5 malhas, 1 osso), link do osso 19 reconstruído e provado por
  recomputação independente, roundtrip < 1e-6, gates equipados (root follow ≥
  3, drift de rigidez ≤ 0,75, alcance < 135) e portões de classe documentados.
- `cape-cloth-contract.md`: estudo do contrato de cloth para a integração
  (grades do runtime, link, `bCloak` por classe, portões de ID novo).
- `prototype/texture-dependencies.json`: os 3 atlases ausentes declarados.
- `native-reference-audit.json`: proveniência, rigs e contratos nativos
  (12 modelos, incluindo o precedente de capa `emperor_cape`).

A fonte regenerável são os `*.py`; editar só o `.blend` não altera o gerador.
Nenhuma instalação em `Data/`, nenhum deploy, nenhuma alteração nas pastas
`art-source/celestial` e `art-source/poseidon` nesta lane.

## Próximas ondas (não desta lane)

1. Capa na integração: abrir o portão `bCloak` para a família CLASS_DARK
   (Duel Master), case próprio de cloth para o ID Zeus (sem saia) e prova de
   colisão/vento in-game — mapa completo em `cape-cloth-contract.md`.
2. Ícones de inventário do pendant e dos anéis; as aparições no corpo dependem
   de suporte posterior do cliente.
3. Atlases `Zeus_*` com bordas/frente/costas coerentes e UV final; remover o
   bloqueio de protótipo só após auditoria das dependências.
4. Backend: IDs, requisitos, categoria, bônus/fases — **update 249 a reservar**
   na coordenação de ondas; nunca publicar conjunto incompleto como pronto.
5. Integração/FX: aura, ataque, movimento e buff com orçamento de partículas,
   validação visual in-game frente/costas/inventário/chão e comparação com o
   Legendary +15 Excellent na mesma câmera; para as asas, validar o flap
   (9 quadros), o link pelo osso 47 e interpenetrações com o torso em todas
   as ações do player.
