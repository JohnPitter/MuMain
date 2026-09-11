# Manto do Poseidon — versão negra

## Estado e autorização

Início de conjunto autoral para **Lord Emperor**, baseado na imagem fornecida
`codex-clipboard-04794f6e-f53a-4555-a521-d0a673490353.png`.
Esta prancha é referência visual, não uma especificação aprovada de atributos.
Não foram adotados dano, defesa, nível, bônus, slots, raridade ou efeitos funcionais
escritos na imagem. Nada foi publicado, inserido em conta ou conectado ao launcher.

Entregue nesta etapa: projeto completo, auditoria das bases nativas, duas armas
autorais editáveis, BMD de diagnóstico, seis renders e verificações de integridade.
Segundo lote entregue: elmo, peitoral e botas autorais sobre o esqueleto nativo
Class305 auditado, com exportação BMD diagnóstica, provas de roundtrip, prova
equipada no rig do player e onze renders de inspeção. Terceiro lote entregue:
calças e luvas autorais na mesma disciplina (ossos exatamente os das peças
nativas correspondentes, roundtrip abaixo de 1e-6, prova equipada e dezoito
renders de inspeção no conjunto). Quarto lote entregue: pendant Tridente Real e
os dois anéis distintos do painel ITENS ADICIONAIS (Anel das Marés à esquerda,
Anel do Imperador à direita - dois modelos autorais, nunca um anel equipado duas
vezes) como itens rígidos no contrato nativo de item auditado, BMDs diagnósticos,
roundtrip abaixo de 1e-6 e nove renders de inspeção. Manto, acessórios restantes,
pets e efeitos abaixo são **projeto**, não malhas concluídas. O aspecto final depende de
escultura/texturas, ajuste de equipamento e aprovação visual no cliente; não existe
promessa de identidade pixel a pixel.

## Arquitetura de arte

Pipeline Python/Blender separado em formas, materiais, exportação, inspeção visual
e validação. O encoder BMD, leitor e primitivas geométricas gerais do Celestial são
importados em leitura, sem duplicação nem reaproveitamento dos seus desenhos.
As silhuetas Poseidon têm fonte própria em `poseidon_shapes.py`.
Nenhuma mudança de runtime, regras backend, IDs ou formato de rede nesta pasta.

O arquivo Blender é editável, mas as formas em Python são a fonte regenerável.
Editar manualmente o `.blend` não altera o gerador; alterações aprovadas
devem voltar à fonte. Os três atlas declarados (`Poseidon_Black.jpg`,
`Poseidon_Gold.jpg`, `Poseidon_Blue.jpg`) agora existem como arte autoral
regenerável: masters 1024 com acabamento gravado em
`textures/masters/`, exportação do jogo 512 JPEG RGB q95 em `textures/` e
empacotamento OZJ (cabeçalho de 24 bytes zero + JPEG, o mesmo contrato
Celestial) em `textures/ozj/`, tudo produzido por `generate_poseidon_atlases.py`
com `--check` de determinismo. Os materiais Blender amostram os atlas definitivos
(`poseidon_materials.py`); os nomes de material/textura nos BMDs não mudaram, e
os BMDs reconstruídos continuaram **byte a byte idênticos**. `Branco Perolado`
existe na paleta do conceito como acessório/brilho; as cinco peças modeladas têm
só os três papéis declarados, e o perolado entra como âncora de brilho dos
masters, aguardando peças de acessório com UV própria. **Continua proibido
copiar estes BMDs para um cliente:** são protótipos diagnósticos; a instalação
dos atlas e dos modelos é decisão explícita de integração.
`prototype/texture-dependencies.json` registra os hashes e o status
`AUTHORED_ATLAS_REGENERABLE`.

## Linguagem visual

Massa principal de metal negro polido, reflexos cinza-frios localizados, contornos
em ouro antigo e cristais azul-oceano. Azul restrito a joias e energia; a armadura
não deve virar uma superfície azul. Curvas de ondas, pontas triplas e losangos
unificam a família. Ouro é estrutura e filete, não cobertura uniforme que apague o
preto. Não usar auréola solar ou asas angelicais do Celestial.

Preservar a estética clássica de MU: silhueta forte na câmera isométrica, massa
legível em tamanho de inventário e pequenos efeitos que não escondam mãos/pés.
Frente, costas e lados recebem detalhe real; nenhuma superfície posterior termina
como bloco liso. Não reutilizar Kundun Staff, Venom Mist ou pets nativos como se
fossem a aparência nova. Os modelos nativos documentam rig e comportamento apenas.

## Inventário completo do projeto

| Peça | Silhueta e detalhes a construir | Vinculação / aceitação | Estado |
| --- | --- | --- | --- |
| Elmo | Coroa fechada em tridente, lâmina central longa, cristal axial, sobrancelhas angulares, nuca articulada negra com borda ouro | Esqueleto Player/Lord Emperor; rosto sem atravessar viseira; inspeção 360° | Protótipo autoral BMD/Blender |
| Armadura | Peitoral negro anatômico, losango azul no esterno, nervuras douradas em ondas; ombreiras assimétricas com fan de penas/placas e guarda curva; escápulas ornamentadas | Pesos rígidos MU nos ossos nativos; testes braços elevados, montaria e costas sem manto | Protótipo autoral BMD/Blender |
| Calças | Lamelas estreitas, faixas de metal segmentadas sobre tecido escuro, quadril e traseira completos | Articulação de joelho/quadril; não soldar faldão às duas pernas | Protótipo autoral BMD/Blender |
| Luvas | Dedos segmentados, garras discretas, guarda dorsal, filetes nas juntas; palma usável | Empunhadura de tridente/cetro e rédea; dedos não viram bloco | Protótipo autoral BMD/Blender |
| Botas | Caneleira curva, tornozelo encaixado, calcanhar recortado e sabaton segmentado com solado | Flexão do pé, estribo e traseira; sem esfera no calcanhar ou cubo branco | Protótipo autoral BMD/Blender |
| Manto | Tecido preto longo com abertura inferior, bordas douradas sinuosas e brasão de tridente; ponte de ombro e fechos metálicos | Cloth do Lord Emperor separado da ferragem BMD; colisão com torso/cavalo e vento | Protótipo autoral BMD/Blender (ferragem; tecido é cloth do runtime — `cape-cloth-contract.md`) |
| Tridente | Três lâminas altas independentes, vazios entre pontas, cristal oceânico central, canaletas ouro, haste delgada e ponteira | Alternativa longa de arma; âncora de pega e FX próprias; classificação funcional pendente | Protótipo autoral BMD/Blender |
| Cetro oceânico | Coroa mais baixa/larga, garras em onda e núcleo azul; não é apenas o tridente reescalado | Alternativa ao tridente, não par obrigatório; compatibilidade de cetro DL a validar | Protótipo autoral BMD/Blender |
| Pendant | Marca de tridente dourada em pedestal negro, losango azul, corrente fina | Ícone e malha independentes; aparição no corpo depende de suporte posterior | Protótipo autoral BMD/Blender |
| Anel das Marés (esquerdo) | Aro negro com bordas ouro, assento arredondado, ondas em volta da gota azul e bolhas de espuma | Modelo/ícone próprio; sem efeito de transformação implícito | Protótipo autoral BMD/Blender |
| Anel do Imperador (direito) | Aro facetado com coroa de cinco pontas e losango azul angular; par distinto do anterior, sem repetir malha | Segundo anel e ícone próprios; diferenças estéticas sem números aprovados | Protótipo autoral BMD/Blender |
| Cavalo negro metálico | Barda articulada negra/ouro, crista na testa/pescoço, joias azuis, cascos metálicos, cauda em faixas | Rig DarkHorse; preservar locomoção/ataque e sela/posição do cavaleiro | Projeto |
| Águia Imperial | Penas negras em camadas com borda ouro, cristal peitoral, olhar azul e garras reais | Rig DarkSpirit separado do personagem; voo, retorno e ataque em prova futura | Projeto |

São 13 definições visuais planejadas, incluindo duas alternativas de arma e os
dois pets. Isso não define regras de equipar simultaneamente, drop ou comércio.
Escudo não aparece como peça independente na referência recebida e não foi
inventado nesta etapa. Qualquer expansão deve ser confirmada antes de balancear.

## Animação e efeitos: contrato planejado, não implementação

- Armaduras acompanham o esqueleto e ações do personagem; não inventar animação
  local incompatível por peça. Provar poses parada, caminhada, ataque, skill,
  sentado/montado e morte antes de integrar.
- Manto: ferragens rígidas e tecido com física são sistemas distintos. O BMD
  nativo de um osso não contém sozinho a animação de tecido. As bordas douradas
  precisam acompanhar a mesma malha de cloth, sem flutuar como armação rígida.
- Armas: protótipos têm uma pose rígida e âncoras `Grip`, `Core`, `Tip`.
  O movimento futuro vem da mão do Lord Emperor; não são armas animadas em jogo
  nesta entrega. Transformação proposta autor Z-up → eixo longitudinal -Y MU.
- Cavalo: manter todas as sete ações do rig nativo; conferir cascos, sela,
  colisão com o manto, giro, ataque e contatos com o chão.
- Águia: manter as quatro ações e a hierarquia nativa; recolhimento/abertura das
  penas devem usar pesos únicos por vértice, sem soldar as asas.
- FX propostos: pulso azul no núcleo; ondas discretas junto aos pés; traço de
  água azul com aresta ouro na arma; brasão de tridente em buff; pequenas fagulhas
  frias no cavalo/águia. Somente desenho visual nesta etapa, não buffs funcionais.
- O brilho deve respeitar invisibilidade, seleção/inventário, distância e
  orçamento de partículas. Evitar acumular passes aditivos Excellent/+15 que
  tornem preto e ouro brancos. Ícones não precisam de efeitos em tempo real.

## Bases nativas verificadas

Fonte congelada: `ultimate-base-downloaded.zip`, SHA-256
`baedcdbe3b66dc2763a9a6538f8d0fd592bc7cccd76844952ec174ca323d3c80`.
`native-reference-audit.json` guarda hash integral de cada arquivo, hierarquia de
ossos, ações, texturas e evidências de código com linhas/hashes. Nenhuma malha
nativa foi reexportada como Poseidon.

| Função | Arquivo real no ZIP | Evidência medida |
| --- | --- | --- |
| Rig do personagem | `Data/Player/player.bmd` | 60 ossos, 284 ações, sem meshes |
| Base visual Lord Emperor | `Data/Player/{Helm,Armor,Pant,Glove,Boot}Class305.bmd` | 51 ossos / 1 pose em cada peça |
| Manto do imperador | `Data/Item/DarkLordRobe02.bmd` | 1 osso, 1 pose, 3 meshes; cloth no runtime |
| Cavalo animado | `Data/Skill/DarkHorse.bmd` | 60 ossos / 7 ações; quadros 7,7,6,10,41,51,31 |
| Águia animada | `Data/Skill/darkspirit.bmd` | 77 ossos / 4 ações; quadros 6,4,4,4 |
| Cetro nativo para orientação | `Data/Item/saint.bmd` | 1 osso, 1 pose, 1 mesh; desenho não reutilizado |

`DarkHorseHorn.bmd` e `DarkHorseSoul.bmd` são representações de item, não o rig
animado da montaria/águia. Esse é um ponto importante para não exportar pets imóveis.

O cliente usa `CLASS_DARK_LORD` como classe-base e `CLASS_LORDEMPEROR` como evolução;
o contrato de servidor tem `LordEmperor = 17`. São enums distintos, não intercambiáveis.
`ZzzOpenData.cpp` carrega `Class3` com índice-base +1: Dark Lord é índice 4, logo
sufixo `305`. A mão padrão usa ossos 33/42 (`ZzzCharacter.cpp`, ramo default), e a
ferragem do manto liga ao osso 19. O cloth usa o osso 19 com colisores no torso,
ossos 17/2; a saia Lord Emperor usa osso 18. A implementação atual condiciona esses
trechos a modelos específicos, portanto um futuro ID novo exigirá integração
explícita: renomear BMD não ativa o cloth automaticamente.

## Entrega atual e próximos critérios

1. **Agora:** silhuetas próprias das duas armas, peças fechadas, juntas estruturais,
   materiais procedurais, inspeção frontal/lateral/detalhe, exportação BMD v10 e
   comparação de triângulos, normais e UV com erro máximo permitido de `2e-5`.
2. **Próximo lote artístico:** elmo/peitoral/bota, apresentados frente/costas/lado
   junto ao rig de Lord Emperor; aprovar família visual antes de multiplicar peças.
   Entregues como protótipos autorais skinados no rig Class305 (ossos 18/20 no
   elmo, 17/18/25/26/27/34/35/36 no peitoral, 4/5/11/12 nas botas); aprovação
   visual e atlases próprios continuam pendentes. Terceiro lote no mesmo molde:
   calças nos ossos nativos 2/3/4/10/11/17/44 (cintura, quadril, coxas, joelhos e
   o osso Bone02 do faldão frontal livre, nunca soldado às pernas) e luvas nos
   ossos 27/28/29/36/37/38 (antebraço, mão e Finger0 de cada braço; os dedos
   extras do esqueleto continuam sem uso, como no nativo).
3. Construir peças restantes, manto com cloth e pets, sem trocar esqueletos por
   primitivas. Provar poses preservadas e meshes dentro dos limites do cliente.
4. Atlas definitivos autorados para os três papéis declarados, com acabamento
   gravado, paleta amostrada do conceito e provas de determinismo, contraste e
   roundtrip. Pendente na mesma linha: UV final por região (chart particionado
   por peça, com bordas/frente/costas dedicadas) quando a geometria evoluir —
   os masters atuais já cobrem qualquer re-UV como acabamento de campo cheio.
   Instalação no cliente continua bloqueada até a decisão de integração.
5. Somente com escopo aprovado: IDs, requisitos, categoria, bônus e fases no
   backend, apresentação no cliente/painel, pacote de testes isolado e avaliação
   pessoal do usuário. Não publicar um conjunto incompleto como pronto.

## Reprodução e arquivos

A partir da raiz deste worktree, com Blender 4.2:

```powershell
python art-source/poseidon/audit_native_references.py
python art-source/poseidon/generate_poseidon_atlases.py  # --check prova determinismo
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/poseidon/build_poseidon.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/poseidon/verify_poseidon.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/poseidon/build_poseidon_armor.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/poseidon/verify_poseidon_armor.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/poseidon/poseidon_armor_equipped.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/poseidon/build_poseidon_legs_hands.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/poseidon/verify_poseidon_legs_hands.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/poseidon/poseidon_legs_hands_equipped.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/poseidon/build_poseidon_jewels.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/poseidon/verify_poseidon_jewels.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/poseidon/inspect_poseidon_uvs.py
python -m unittest discover -s art-source/poseidon -p 'test_*.py' -v
```

`build_poseidon.py -- --skip-render` regenera malhas e Blender sem renderizar.
As duas coleções são `Poseidon Trident` e `Poseidon Scepter`; a primeira fica visível
por padrão e a outra pode ser habilitada para edição. Ambas compartilham a origem
de pega, portanto habilitá-las simultaneamente superpõe as alternativas.

As peças de armadura (`build_poseidon_armor.py`) geram as coleções `Poseidon Helm`,
`Poseidon Armor` e `Poseidon Boot` em `prototype/poseidon-armor.blend`, os BMDs
diagnósticos `Poseidon_{Helm,Armor,Boot}.bmd` em `prototype/models/`, as vistas
frente/lado/costas em `prototype/renders/` e a prova equipada
(`poseidon_armor_equipped.py`) posa os BMDs exportados com as matrizes do próprio
`player.bmd` junto a um mannequin neutro de prévia — reconstrução de matemática de
encaixe, não captura in-game.

As calças e luvas (`build_poseidon_legs_hands.py`) seguem o mesmo desenho: coleções
`Poseidon Pant` e `Poseidon Glove` em `prototype/poseidon-legs-hands.blend`, BMDs
diagnósticos `Poseidon_{Pant,Glove}.bmd`, seis vistas de inspeção, prova equipada
(`poseidon_legs_hands_equipped.py`) e relatórios próprios — sem tocar em
`texture-dependencies.json`, que continua descrevendo apenas os três atlas pendentes.

- `prototype/poseidon-weapons.blend`: malhas 3D, materiais e estúdio.
- `prototype/models/`: BMD de diagnóstico **não instaláveis**.
- `prototype/renders/`: vistas de inspeção frente/lado/costas das duas armas,
  do elmo, do peitoral e das botas, prova equipada e painel de paleta
  (`Poseidon_atlas_palette.png`); nenhuma é imagem do jogo.
- `textures/masters/`: masters definitivos 1024 dos três atlas (gravura
  procedural determinística; `generate_poseidon_atlases.py --check`).
- `textures/Poseidon_{Black,Gold,Blue}.jpg`: exportação do jogo 512 JPEG RGB q95.
- `textures/ozj/Poseidon_{Black,Gold,Blue}.OZJ`: wrapper do cliente
  (24 bytes zero + JPEG), mesmo contrato dos atlas Celestial.
- `textures/atlas-report.json`: paleta amostrada, âncoras, sementes, hashes e
  contraste P95−P05 por atlas.
- `prototype/uv-region-report.json`: prova do layout UV — projeção planar por
  componente cobrindo o quadrado 0-1 inteiro, com o mapa componente→atlas dos
  cinco modelos; por isso os masters são acabamentos gravados de campo cheio,
  não charts particionados.
- `prototype/build-report.json`: contagens, hashes e prova durante geração.
- `prototype/saved-roundtrip-report.json`: prova reabrindo o Blender salvo.
- `prototype/texture-dependencies.json`: atlas pendentes explicitamente declarados.
- `prototype/poseidon-armor.blend`: elmo, peitoral e botas sobre o rig auditado.
- `prototype/armor-build-report.json`: orçamentos, ossos usados, âncoras e prova.
- `prototype/armor-saved-roundtrip-report.json`: roundtrip das peças reabrindo o Blender.
- `prototype/equipped-review-report.json`: prova de encaixe no rig do player.
- `prototype/poseidon-legs-hands.blend`: calças e luvas sobre o rig auditado.
- `prototype/legs-hands-build-report.json`: orçamentos justificados, ossos usados
  (contrato medido nos nativos), âncoras e prova.
- `prototype/legs-hands-saved-roundtrip-report.json`: roundtrip das duas peças
  reabrindo o Blender salvo.
- `prototype/legs-hands-equipped-review-report.json`: prova de encaixe das calças e
  luvas no rig do player, com deslocamento mínimo entre as poses parada/caminhada.
- `prototype/poseidon-jewels.blend`: pendant e dois anéis como itens rígidos
  (contrato do item nativo auditado: malhas rígidas, pose única, geometria no
  osso 0; âncoras de montagem nomeiam bail/gema/banda).
- `prototype/jewels-build-report.json`: orçamentos de joia justificados, âncoras
  em espaço item, contrato do item nativo e prova de geração.
- `prototype/jewels-saved-roundtrip-report.json`: roundtrip dos três BMDs
  reabrindo o Blender salvo.
- `native-reference-audit.json`: proveniência/rigs e contratos nativos verificados.

As imagens de textura são geradas por Pillow em script determinístico
(`generate_poseidon_atlases.py`), versionadas junto com os BMDs; nenhuma
imagem nativa do cliente foi alterada, nenhum atlas foi instalado em `Data/`
e não houve alteração de arquivos Celestial, modelos nativos, runtime ou
instalação do jogador.
