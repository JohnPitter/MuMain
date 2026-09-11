# Celestial: armadura própria em revisão

Estado mais recente: [revisão dourada de 11/09/2026](GOLDEN.md), com base ouro,
frisos claros, integridade das animações e identificação das prévias atuais.
As seções abaixo preservam a evolução histórica das malhas.

As cinco peças agora têm geometria própria. Nenhum triângulo de Phoenix Soul,
Venom Mist ou outro equipamento nativo é usado nos BMD deste lote. O esqueleto
e as poses do personagem são referências de encaixe, não aprovação visual.
O usuário confirmou a entrada no jogo e aprovou a aparência da versão publicada
em Testes em 11/09/2026. Esta revisão posterior melhora as botas; seus novos
detalhes ainda dependem da validação pessoal no cliente. Não é reprodução
pixel a pixel do conceito.

## Fontes e reprodução

O projeto usa Blender 4.2. As formas estão em `celestial_cuirass.py` e
`celestial_limb_armor.py`, botas em `celestial_boot_armor.py`, costas em
`celestial_rear_armor.py` e relevos
compartilhados em `celestial_gilding.py`;
as superfícies, pesos e exportação são módulos
separados. Cada vértice tem exatamente uma influência, como exige o renderer
do MU; o exportador rejeita pesos que seriam silenciosamente perdidos.

Executar no Blender em background, com `--python-exit-code 1`:

1. `build_celestial_armor.py -- <diretório Data/Player do cliente>`.
2. `verify_armor_roundtrip.py`, somente depois de concluir a geração.
3. `render_armor_motion.py`, para a prévia curta de caminhada.
4. No Python, `python -m unittest discover -s art-source/celestial -p test_authored_armor.py -v`.
5. `review_armor_details.py -- <diretório Data/Player>` para frente, costas,
   lateral, closes do elmo e das botas sem asas ou calça encobrindo os detalhes.
6. `review_equipped_set.py -- <diretório Data/Player>` para conferir a composição
   com cajado, escudo, asas e cabeça nas poses nativas.

`--no-render` após o caminho dispensa as quatro imagens, não a geração ou a
medição das poses. Imagens antigas não comprovam uma geração nova sem render.

## Conteúdo verificado

- `authored-armor/celestial-authored-armor.blend`: malhas editáveis, texturas
  empacotadas, 51 ossos e oito sequências nativas na linha do tempo.
- Marcadores: repouso, caminhada, corrida, magia, morte, repouso em voo,
  deslocamento em voo e ataque físico. Cajado de uma mão usa as poses
  `SWORD` (4/17/26), conforme seleção real do cliente, não as `WAND`.
  Todos os
  vértices foram comparados com a transformação nativa em cada keyframe.
  Erro máximo medido: 0,000160 unidade; isto não mede interpenetrações.
- Cinco BMD em `authored-armor/Data/Player` e três OZJ. O relatório de ida e
  volta compara todos os cantos, ossos, normais e UVs. Erro < 0,000001;
  triângulos de área nula são rejeitados. Manifesto SHA256 cobre os oito arquivos.
  O relatório registra também os hashes do BMD e do `.blend` efetivamente
  comparados; regenerar arte sem repetir a verificação invalida o pacote.
- O rosto original `HelmClass201.bmd` aparece **só como referência de encaixe**
  na coleção `REFERENCE ONLY`, nunca nos BMD exportados. Não é rosto novo.
- No cliente, a coroa habilita a cabeça nativa; o teste cobre todos os IDs
  para garantir que os outros elmos mantenham o comportamento anterior.
- Ouro, marfim e safira ficam separados: brilho de upgrade não cobre o
  conjunto inteiro. Os modelos nativos não são substituídos.

## O que falta

Proporções, placas e ornamentos ainda estão simplificados frente ao conceito.
É necessário revisar costas/laterais, encontro de ombreiras e braços, abertura
do rosto (também Grand Master), painéis da calça em movimento e botas. Falta
aprovação visual e medição de desempenho. Asas/halo e composição equipada
agora têm candidatos próprios documentados em `WINGS.md`, não aprovação.
O lote atual da armadura soma 39.486 triângulos; não há orçamento
de desempenho validado para cenas cheias de jogadores.

A prévia MP4 é animação real destas malhas no Blender, não captura do MU.
Antes de publicar, validar cliente +0/+15, inventário, chão, seleção, repouso,
caminhada, corrida, ataque/magia, morte, voo e efeitos ligados/desligados.
Somente distribuir executável e todos os assets juntos após essa revisão.

## Requisitos do conjunto completo

Continuam no escopo: cinco peças, cajado, escudo, asas, pendant e duas unidades
do anel (uma malha compartilhada). Os novos requisitos solicitados são nível 400
e Grand Master; a regra fica no servidor, não no gerador das malhas. A defesa
base das cinco peças é
324 contra 251 do Venom Mist; isto não certifica o balanceamento completo.
Os bônus completos foram implementados no servidor ca012d180 e a indicação
visual no cliente 18c1f0d6, ainda locais; ver `docs/gameplay/celestial-equipment-state.md`.
Sockets, opções, progressão e obtenção ainda precisam de auditoria. Nenhum
estado remoto foi consultado ou alterado nesta etapa artística.

## Revisão de ornamentos e silhueta — b99ad7e8

As referências originais do usuário estão preservadas em `references/`, sem
alterações. A revisão adiciona lâminas sobrepostas à coroa, relevos ao peitoral,
ombreiras e antebraços, e placas às caneleiras/pés. Os bordados dos painéis
foram reamostrados com menos segmentos: 11.906 triângulos na calça, em vez de
18.242. Total da armadura: 29.274 em vez de 31.050, mesmo com os novos relevos
e o fechamento das pontas/calcanhares. A vista traseira revelou as aberturas
dos pés: foram fechadas e o verificador exige dois sabatons sem borda aberta.

Os oito movimentos e o retorno BMD/Blender foram medidos novamente. Isto não
aprova a fidelidade: as formas ainda são geométricas/regulares demais perante
o conceito, e faltam textura/gravura fina, revisão das costas e detalhes do
encaixe. A cabeça é a nativa do Soul Master, não uma recriação do rosto da arte.

## Revisão traseira — 2026-09-11

Substituído o volume dourado liso do elmo por uma base marfim mais contida,
com lâminas sobrepostas, crista central, ornamentos na nuca e safira traseira.
Costas da armadura recebem placas escapulares, lâminas dorsais articuladas,
relevos e guarnições posteriores dos ombros/braços. O ornamento lombar foi
aproximado da superfície após inspeção da vista traseira. As novas superfícies
usam os mesmos materiais e ossos; não há dependência de equipamento nativo.

Elmo: 3.880 triângulos (antes 2.506); armadura: 9.024 (antes 7.098).
Calça, luvas, botas, cajado, escudo, asas e joias não mudaram nesta revisão.
Total das cinco peças: 32.574, antes 29.274. Aumento de geometria não implica
desempenho aprovado. Os 21 relevos traseiros do elmo e 48 da armadura são
verificados quanto a fechamento, orientação e posição posterior, além da
conferência existente de todos os cantos/normais/UVs exportados.

As oito sequências foram medidas novamente, quatro renders da fonte, cinco
vistas do BMD sem asas, cinco vistas equipadas e o vídeo de caminhada atualizados.
Tudo é prévia de geometria real no Blender, não captura do cliente. Continuam
diferenças de proporção, riqueza de ornamentos, gravuras e iluminação em
relação ao conceito. Não está idêntico, não está validado ingame e não foi
publicado. Conferir também Grand Master e cenas com vários jogadores.

## Placas curvas e candidato de Testes — 2026-09-11

Peitorais e ombreiras agora têm quatro superfícies frontais curvas, com
contornos arredondados e profundidade progressiva, em vez das faces piramidais.
O verificador exige fechamento, volume positivo, normais suaves e níveis de
profundidade; mantém a comparação de todos os cantos exportados. Armadura:
11.632 triângulos, total das cinco peças: 35.182. Demais BMD permanecem iguais.

As prévias foram atualizadas, incluindo caminhada e duas vistas adicionais
com a cabeça nativa Grand Master (`HelmClass301.bmd`). A referência Soul Master
continua `HelmClass201.bmd`; nenhuma cabeça é exportada como equipamento.
Os 29 testes Python passaram. Os renders não comprovam encaixe ou desempenho
no cliente. A coroa ainda cobre a região dos olhos, e as proporções e o nível
de detalhe continuam diferentes do conceito.

O usuário autorizou publicar este candidato somente em Testes e prefere fazer
pessoalmente a validação ingame. Essa autorização não é aprovação artística.
O pacote deve preservar a DLL de conexão e todos os arquivos não relacionados,
usar uma base exclusiva de Testes e manter produção inalterada. O registro de
implantação e seus hashes ficam no CHANGELOG operacional do workspace.

## Botas posteriores e formato do pé — 2026-09-11

As caneleiras recebem lâminas douradas com esmalte marfim na panturrilha,
uma nervura sobre o tendão, safiras posteriores, penas laterais e contorno
articulado do tornozelo. O pé arredondado foi substituído por um sabaton com
calcanhar angular, sola definida e quatro placas sobrepostas no peito do pé.
Ouro, marfim e safira continuam separados, sem alterar as texturas compartilhadas.

Somente `Celestial_Boots.bmd` muda nos oito arquivos distribuídos da armadura.
São 9.168 triângulos nas botas, antes 4.864; as cinco peças somam 39.486.
Os ossos de panturrilha/pé e as oito sequências nativas permanecem iguais.
O verificador exige dois pés e duas solas fechados, oito placas de peito do pé,
oito relevos laterais e 44 ornamentos posteriores, todos com volume positivo.
As 29 verificações Python e a comparação de cada canto/normal/UV exportado
passaram. Os renders equipados agora usam a cabeça nativa Grand Master.

As prévias de frente, costas, lateral e caminhada são geometria real no Blender,
não captura do jogo. A aprovação da versão anterior não substitui a revisão
destes novos detalhes e do desempenho em cenas com vários personagens.
