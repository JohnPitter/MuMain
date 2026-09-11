# Referência completa de equipamentos e armaduras MU

Levantamento de 11/09/2026 para orientar os próximos sets. Abrange as fontes
disponíveis neste workspace: arquivos, tabelas e código do cliente/servidor.
Não é uma afirmação de que todos os recursos encontrados estão disponíveis aos
jogadores ou foram testados dentro do jogo.

## Por onde começar

**Complementos posteriores ao snapshot:** [Golden e Dark Iron Knight](../../../art-source/celestial/research/dark-iron-knight.md),
[passes metálicos e efeitos Golden](../../../art-source/celestial/research/golden-runtime.md),
[Kundun e platinado](../../../art-source/celestial/research/kundun-platinum.md),
[acabamento atual do Celestial](../../../art-source/celestial/METAL.md) e
[início do Poseidon / Lord Emperor](../../../art-source/poseidon/design-spec.md).
O catálogo e o índice de efeitos abaixo preservam a revisão original da varredura;
hashes/linhas antigos do renderer Celestial não representam a alteração posterior.

| Para consultar | Documento / evidência |
|---|---|
| Cores, famílias e arquivos por origem | [Catálogo geral](catalog.md) |
| Cada peça de armadura, malhas, ossos, quadros e materiais | [Detalhes das armaduras](armor-details.md) |
| Cajados, espadas, escudos, asas, anéis, pendants e demais Data/Item | [Detalhes dos equipamentos](equipment-details.md) |
| Formas, estilos, frente e costas das 70 famílias | [Estudo visual](visual-styles.md) |
| Animações, encaixes, brilho, partículas, auras, sons e renderização | [Guia de execução e efeitos](runtime-guide.md) |
| Duplicatas, variantes, sentinelas e referências fora da pasta esperada | [Cobertura e exceções](coverage.md) |
| Consulta automatizada/reprodução | [Catálogo JSON](catalog.json), [famílias](canonical-families.json), [índice de efeitos](equipment-effects-index.json) |

## O que foi preservado no mapeamento

1. **Identidade:** origem, caminho exato, hash, grupo/número, nome nas tabelas,
   classe/requisito quando identificável e associação real no carregador. Arquivos
   Class/Head/Face, Monk e variações corporais não são confundidos com novos sets.
2. **Geometria:** meshes, vértices, normais, UVs, triângulos, limites espaciais,
   materiais e referências de textura. As fontes binárias continuam intactas;
   o relatório não substitui os BMD originais.
3. **Rig e movimento:** hierarquia/índices dos ossos, poses e quadros, sequências
   próprias versus ações compartilhadas de `Player.bmd`, movimento da raiz,
   trava de posição e ligação ao personagem. As 291 declarações `PLAYER_*`
   também ficam no índice, com aliases e linhas de origem.
4. **Superfícies:** atlas originais, dimensões, transparência, seis cores
   representativas por textura e miniaturas. A paleta mede pixels do atlas,
   não a proporção de cor visível após UVs e iluminação.
5. **Efeitos:** cores multiplicativas, passes aditivos, chrome/metal, brilho por
   upgrade, Excellent/Ancient, auras de conjunto, emissão por osso, partículas,
   sprites, joints, rastros, efeitos temporizados, recursos animados e sons.
   O código que cria e atualiza efeitos permanece localizável por arquivo/linha
   e referências `MODEL_*`, `BITMAP_*`, `SOUND_*`.
6. **Apresentação:** equipamento no corpo, inventário, chão e seleção; ocultação,
   transparência, escala, opções gráficas e diferenças entre os caminhos CPU/GPU.
   Um anel ou pendant ter ícone/modelo não prova que ele aparece no corpo.
7. **Regras e integração:** propriedade autoritativa no backend, aparência enviada
   por pacotes, opções e sockets separados dos efeitos visuais. No Celestial,
   Ultimate, Grand Master/400 e fases 30/60/100% continuam regras do servidor.

## Cobertura e confiabilidade

A varredura reúne **45 origens**, **20.555 ocorrências de BMD** e **1.000 hashes
distintos**: 998 arquivos de malha analisados sem falha e duas tabelas BMD
classificadas separadamente. Foram decodificadas **1.063 texturas distintas**.
As contagens detalhadas e exceções ficam no catálogo, que é a fonte dessa medição.

A base canônica contém **1.019 candidatos em Data/Player e Data/Item**. O mapa do
carregador identifica **70 famílias de armadura**; todas têm prévias de frente e
costas, totalizando **140 vistas**. As descrições de estilo foram feitas olhando
essas imagens. Geometria escondida por sentinela não deve ser usada como parte
visível do equipamento. Um fallback de textura compartilhada precisa ser
identificado explicitamente, nunca preenchido silenciosamente com outra textura.

Os renders finais omitem 115 malhas de sentinela em 33 famílias, conforme a regra
do cliente. O elmo Lucky70 reutiliza a textura carregada em Lucky65, com origem e
hash registrados. As 140 vistas não têm referências de textura pendentes. Já o
catálogo maior mantém 34 referências canônicas fora da pasta esperada, separadas
em [coverage.md](coverage.md); algumas têm candidatos compartilhados, outras ainda
precisam de confirmação. Não são automaticamente 34 erros do jogo.

As 13 cópias RAR encontradas também foram conferidas por hash e listagem:
um conteúdo distinto, com três tabelas TXT da loja e nenhuma entrada visual
identificada. A [auditoria suplementar](archive-audit.json) conserva a evidência;
o scanner padrão continua sem suporte genérico a RAR/7z.

O índice de efeitos varreu 949 fontes de C++, com 386 arquivos indexados e 39.973
registros. É um **índice lexical abrangente**, incluindo NPCs e monstros, não um
grafo causal completo. Uma chamada perto do nome de um item não comprova que se
aplica a ele; leia as condições e handlers indicados pelo guia antes de reutilizar.
Efeitos encontrados no código não equivalem a execução/visibilidade confirmada.

### Base congelada e revisão do Celestial

Referência canônica: `ultimate-base-downloaded.zip`, baixada da API de Testes,
SHA-256 `baedcdbe3b66dc2763a9a6538f8d0fd592bc7cccd76844952ec174ca323d3c80`.
É a base **Ultimate anterior à revisão dourada**, mantida imutável para comparação.
Origens alternativas são separadas por caminho/hash; não se mistura uma instalação
antiga com a publicada para concluir que um recurso foi perdido.

O Celestial novo está documentado em [GOLDEN.md](../../../art-source/celestial/GOLDEN.md):
ouro nas superfícies principais, frisos claros e safiras preservadas; correção da
dupla sobreposição Excellent. As prévias atuais do conjunto estão em
[equipped-review](../../../art-source/celestial/equipped-review/). A família 74
das pranchas nativas mostra a base congelada anterior, não essa nova paleta.

## Como usar para criar o próximo equipamento

- Escolher uma família pela silhueta e examinar também as costas; comparar o custo
  de geometria e o rig, não só a beleza do ícone. Não copiar malha de outro item
  quando a proposta exige um modelo próprio.
- Definir cada componente: cinco peças ou exceções da classe, mãos/cajado/escudo,
  asas, pendant e dois anéis. Anotar slots, escala, materiais e pontos de ligação.
- Separar cor base, detalhe, gema e emissão. Definir orçamento de brilho por passe
  e testar +0/+15 com e sem Excellent; branco saturado esconde detalhes mesmo
  quando a textura é dourada. Não inventar normal maps/PBR onde o cliente não os usa.
- Reutilizar o esqueleto e ações compatíveis; conferir encaixe em repouso, caminhada,
  corrida, ataque, magia, morte e voo. Asas precisam de movimento real, não apenas
  partículas; armadura recebe animação do personagem.
- Para cada efeito selecionado, registrar modelo/textura/som, osso ou posição,
  condição de ativação, frequência/tempo de vida, intensidade/alpha, atualização,
  descarte e opção gráfica que o desativa. Conferir recurso carregado no caminho real.
- Implementar requisitos/bônus exclusivamente no backend; cliente apresenta o
  contrato. Nome Ultimate/Excellent não deve criar regras locais divergentes.
- Validar ida e volta BMD, referências de textura, orientação/normais, root motion,
  limites do loader, comportamento de ocultação e custo com vários personagens.
- Distribuir Main e assets juntos em base isolada, com hashes, preservação dos
  demais arquivos e caminho de retorno; verificar o download real do launcher.
- Registrar capturas ingame frente/costas, inventário, chão e seleção; comparar
  animação, saturação e desempenho. Render Blender é diagnóstico, não aceite final.

## Manutenção e reprodução

Ferramentas em [tools/armor_catalog](../../../tools/armor_catalog/): descoberta e
leitura de bases, decodificação de tabelas/texturas, classificação do loader,
relatórios, renderização nativa e montagem das pranchas. Consulte `--help` de
`build_catalog.py` e `render_native_families.py` antes de repetir. O scanner usa
Python com Pillow; os renders usam Blender 4.2. Nenhum deles inicia o jogo.

`build_visual_styles.py` compõe o Markdown a partir das notas inspecionadas, sem
inventar nomes. `build-runtime-index.mjs` recompõe o índice de fonte com Node.
Após mudar código ou base, atualizar hashes, executar os testes do scanner,
regerar somente os diagnósticos afetados e reinspecionar as imagens. Conservar
datas/limites da observação; não promover uma inferência a fato validado ingame.
