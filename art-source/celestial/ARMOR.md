# Celestial: armadura própria em revisão

As cinco peças agora têm geometria própria. Nenhum triângulo de Phoenix Soul,
Venom Mist ou outro equipamento nativo é usado nos BMD deste lote. O esqueleto
e as poses do personagem são referências de encaixe, não aprovação visual.
**Ainda não está idêntico ao conceito, não foi implantado e não está validado ingame.**

## Fontes e reprodução

O projeto usa Blender 4.2. As formas estão em `celestial_cuirass.py` e
`celestial_limb_armor.py`; as superfícies, pesos e exportação são módulos
separados. Cada vértice tem exatamente uma influência, como exige o renderer
do MU; o exportador rejeita pesos que seriam silenciosamente perdidos.

Executar no Blender em background, com `--python-exit-code 1`:

1. `build_celestial_armor.py -- <diretório Data/Player do cliente>`.
2. `verify_armor_roundtrip.py`, somente depois de concluir a geração.
3. `render_armor_motion.py`, para a prévia curta de caminhada.
4. No Python, `python -m unittest discover -s art-source/celestial -p test_authored_armor.py -v`.

`--no-render` após o caminho dispensa as quatro imagens, não a geração ou a
medição das poses. Imagens antigas não comprovam uma geração nova sem render.

## Conteúdo verificado

- `authored-armor/celestial-authored-armor.blend`: malhas editáveis, texturas
  empacotadas, 51 ossos e cinco sequências nativas na linha do tempo.
- Marcadores: repouso com cajado, caminhada, corrida, magia e morte. Todos os
  vértices foram comparados com a transformação nativa em cada keyframe.
  Erro máximo medido: 0,000154 unidade; isto não mede interpenetrações.
- Cinco BMD em `authored-armor/Data/Player` e três OZJ. O relatório de ida e
  volta compara todos os cantos, ossos, normais e UVs. Erro < 0,000001;
  triângulos de área nula são rejeitados. Manifesto SHA256 cobre os oito arquivos.
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
aprovação visual, asas/halo, composição equipada com cajado/escudo e medição
de desempenho. O lote da armadura soma 31.050 triângulos; não há orçamento
de desempenho validado para cenas cheias de jogadores.

A prévia MP4 é animação real destas malhas no Blender, não captura do MU.
Antes de publicar, validar cliente +0/+15, inventário, chão, seleção, repouso,
caminhada, corrida, ataque/magia, morte, voo e efeitos ligados/desligados.
Somente distribuir executável e todos os assets juntos após essa revisão.

## Requisitos do conjunto completo

Continuam no escopo: cinco peças, cajado, escudo, asas, pendant e duas unidades
do anel (uma malha compartilhada). O código de inicialização consultado mantém
nível 400 e classes Soul Master/Grand Master. A defesa base das cinco peças é
324 contra 251 do Venom Mist; isto não certifica o balanceamento completo.
Os bônus exclusivos ilustrados no conceito, sockets, opções, progressão e
obtenção ainda precisam de auditoria. Nenhum estado remoto foi consultado ou
alterado nesta etapa artística.
