# Manto do Poseidon — versão negra

## Estado e autorização

Início de conjunto autoral para **Lord Emperor**, baseado na imagem fornecida
`codex-clipboard-04794f6e-f53a-4555-a521-d0a673490353.png`.
Esta prancha é referência visual, não uma especificação aprovada de atributos.
Não foram adotados dano, defesa, nível, bônus, slots, raridade ou efeitos funcionais
escritos na imagem. Nada foi publicado, inserido em conta ou conectado ao launcher.

Entregue nesta etapa: projeto completo, auditoria das bases nativas, duas armas
autorais editáveis, BMD de diagnóstico, seis renders e verificações de integridade.
Armaduras, manto, acessórios, pets e efeitos abaixo são **projeto**, não malhas
concluídas. O aspecto final depende de escultura/texturas, ajuste de equipamento e
aprovação visual no cliente; não existe promessa de identidade pixel a pixel.

## Arquitetura de arte

Pipeline Python/Blender separado em formas, materiais, exportação, inspeção visual
e validação. O encoder BMD, leitor e primitivas geométricas gerais do Celestial são
importados em leitura, sem duplicação nem reaproveitamento dos seus desenhos.
As silhuetas Poseidon têm fonte própria em `poseidon_shapes.py`.
Nenhuma mudança de runtime, regras backend, IDs ou formato de rede nesta pasta.

O arquivo Blender é editável, mas as formas em Python são a fonte regenerável.
Editar manualmente apenas o `.blend` não altera o gerador; alterações aprovadas
devem voltar à fonte. Materiais de apresentação são procedimentos PBR 3D, sem
bitmap novo. Os BMD mencionam três futuros atlas, ausentes deliberadamente:
`Poseidon_Black.jpg`, `Poseidon_Gold.jpg`, `Poseidon_Blue.jpg`.
**Não copiar estes BMD para um cliente:** o renderer legado não interpreta PBR e
falharia ao resolver os atlas. `prototype/texture-dependencies.json` explicita isso.

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
| Elmo | Coroa fechada em tridente, lâmina central longa, cristal axial, sobrancelhas angulares, nuca articulada negra com borda ouro | Esqueleto Player/Lord Emperor; rosto sem atravessar viseira; inspeção 360° | Projeto |
| Armadura | Peitoral negro anatômico, losango azul no esterno, nervuras douradas em ondas; ombreiras assimétricas com fan de penas/placas e guarda curva; escápulas ornamentadas | Pesos rígidos MU nos ossos nativos; testes braços elevados, montaria e costas sem manto | Projeto |
| Calças | Lamelas estreitas, faixas de metal segmentadas sobre tecido escuro, quadril e traseira completos | Articulação de joelho/quadril; não soldar faldão às duas pernas | Projeto |
| Luvas | Dedos segmentados, garras discretas, guarda dorsal, filetes nas juntas; palma usável | Empunhadura de tridente/cetro e rédea; dedos não viram bloco | Projeto |
| Botas | Caneleira curva, tornozelo encaixado, calcanhar recortado e sabaton segmentado com solado | Flexão do pé, estribo e traseira; sem esfera no calcanhar ou cubo branco | Projeto |
| Manto | Tecido preto longo com abertura inferior, bordas douradas sinuosas e brasão de tridente; ponte de ombro e fechos metálicos | Cloth do Lord Emperor separado da ferragem BMD; colisão com torso/cavalo e vento | Projeto |
| Tridente | Três lâminas altas independentes, vazios entre pontas, cristal oceânico central, canaletas ouro, haste delgada e ponteira | Alternativa longa de arma; âncora de pega e FX próprias; classificação funcional pendente | Protótipo autoral BMD/Blender |
| Cetro oceânico | Coroa mais baixa/larga, garras em onda e núcleo azul; não é apenas o tridente reescalado | Alternativa ao tridente, não par obrigatório; compatibilidade de cetro DL a validar | Protótipo autoral BMD/Blender |
| Pendant | Marca de tridente dourada em pedestal negro, losango azul, corrente fina | Ícone e malha independentes; aparição no corpo depende de suporte posterior | Projeto |
| Anel da Maré | Aro negro com bordas ouro e cristal azul central, coroa de três garras | Modelo/ícone próprio; sem efeito de transformação implícito | Projeto |
| Anel do Abismo | Aro com ondas cruzadas e cristal menor; identidade de par sem duplicar integralmente o anterior | Segundo anel e ícone próprio; diferenças estéticas sem números aprovados | Projeto |
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
3. Construir peças restantes, manto com cloth e pets, sem trocar esqueletos por
   primitivas. Provar poses preservadas e meshes dentro dos limites do cliente.
4. Produzir atlas próprios e UV finais com bordas/frente/costas coerentes;
   refletância PBR das prévias não substitui textura MU. Remover o bloqueio de
   protótipo apenas após todas as dependências existirem e serem auditadas.
5. Somente com escopo aprovado: IDs, requisitos, categoria, bônus e fases no
   backend, apresentação no cliente/painel, pacote de testes isolado e avaliação
   pessoal do usuário. Não publicar um conjunto incompleto como pronto.

## Reprodução e arquivos

A partir da raiz deste worktree, com Blender 4.2:

```powershell
python art-source/poseidon/audit_native_references.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/poseidon/build_poseidon.py
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background --python-exit-code 1 --python art-source/poseidon/verify_poseidon.py
python -m unittest discover -s art-source/poseidon -p 'test_*.py' -v
```

`build_poseidon.py -- --skip-render` regenera malhas e Blender sem renderizar.
As duas coleções são `Poseidon Trident` e `Poseidon Scepter`; a primeira fica visível
por padrão e a outra pode ser habilitada para edição. Ambas compartilham a origem
de pega, portanto habilitá-las simultaneamente superpõe as alternativas.

- `prototype/poseidon-weapons.blend`: malhas 3D, materiais e estúdio.
- `prototype/models/`: BMD de diagnóstico **não instaláveis**.
- `prototype/renders/`: seis PNGs de estúdio (frontal, lateral oblíqua a 72° e
  detalhe), não imagens do jogo.
- `prototype/build-report.json`: contagens, hashes e prova durante geração.
- `prototype/saved-roundtrip-report.json`: prova reabrindo o Blender salvo.
- `prototype/texture-dependencies.json`: atlas pendentes explicitamente declarados.
- `native-reference-audit.json`: proveniência/rigs e contratos nativos verificados.

Nenhuma imagem de textura foi criada por Pillow nem houve alteração de arquivos
Celestial, modelos nativos, runtime ou instalação do jogador.
