# Celestial — ouro Golden e cinza metálico

> Revisão posterior: os detalhes cinza ganharam [reflexo platinado inspirado no
> Kundun](research/kundun-platinum.md), sem trocar atlas ou ouro. Ver o changelog
> operacional para a publicação mais recente; o registro Golden Metal abaixo é histórico.

Revisão de 11/09/2026 após o retorno do usuário de que a primeira paleta dourada
ainda estava opaca. Esta revisão substitui o acabamento descrito em [GOLDEN.md](GOLDEN.md).
A classe Grand Master, nível 400, Ultimate, opções individuais e fases do conjunto
continuam autoritativos no servidor e não foram alterados.

## Acabamento implementado

- Ouro com dois reflexos do cliente: `RENDER_METAL` / Shiny01 e
  `RENDER_CHROME` / Chrome01. A cor âmbar `(1, .5, 0)` e a intensidade no +15
  acompanham os passes nativos Golden examinados; o chrome se move com o tempo.
- Índices de textura explícitos somente no Celestial, evitando que o caminho
  GPU amostre o atlas difuso no lugar do mapa de reflexo. Outros itens e monstros
  não foram alterados. O efeito não depende das opções Excellent do item.
- As superfícies antes brancas agora têm base cinza metálica, com reflexo menor
  que o ouro. O nome `Celestial_Ivory.jpg` permanece por compatibilidade com os
  BMDs; **não descreve mais a cor**. `Gold`/`Ivory` continuam identificadores
  físicos únicos, e `Base`/`Trim` são os papéis de construção.
- Novos atlas com gravuras, bordas claras e sulcos escuros. O estudo do Dark Iron
  Knight mostrou que boa parte do detalhe vem da pintura, não de polígonos extras.
  Não são cópias do atlas do monstro nem prova de detalhe idêntico ingame.
- Halo, emissão das pedras, encaixes, animações e limite de distância mantidos.
  Qualidade gráfica 0 desliga os reflexos; 1 usa apenas o primeiro passe dourado;
  2 ou maior permite a composição completa. Cloaking e fade são respeitados.

Fontes: [runtime Golden](research/golden-runtime.md) e
[estudo Dark Iron Knight](research/dark-iron-knight.md), com atlas, UVs, rig,
animações, máscaras luminosas, fórmulas e caminhos do código para reutilização.

## Fontes de arte e reprodução

A skill ImageGen foi usada para criar dois masters de textura; não alterou as
malhas nem as regras. [Prompts completos](textures/masters/golden-metal/prompts.json),
[master ouro](textures/masters/golden-metal/Gold.png) e
[master aço](textures/masters/golden-metal/Ivory.png) ficam preservados.
`prepare_metal_textures.ps1` faz somente a exportação mecânica para JPEG RGB
512 × 512, qualidade 95. Os geradores existentes empacotam os JPEGs em OZJ.
Metallic/roughness em `celestial_palette.py` orientam o Blender; o MU usa os
atlas e passes próprios, não esses parâmetros PBR diretamente.

Os três geradores foram executados novamente e suas fontes `.blend`, manifestos
e provas independentes de exportação atualizados. [Integridade da revisão](research/metal-revision-integrity.json):
**dez BMDs byte a byte idênticos à base Golden anterior**, incluindo geometria,
normais, UVs, ossos e todas as ações. Só quatro texturas únicas distribuídas mudam:
Gold e Ivory em `Data/Item` e `Data/Player`. Sapphire e Emissive ficam intactos.

O contraste medido dos atlas inteiros (P95 − P05) aumentou de 32 para 126 no ouro
e de 9 para 90 no antigo Ivory. A mediana deste último caiu de 229 para 106.
Isso mede pixels, não iluminação ou legibilidade final em jogo.
[Comparação reproduzível](research/dark-iron-knight/finish-texture-comparison.json).

## Verificação e limites

Build Release x64 aprovado na árvore `windows-x64-startup-fix`, com o guard de
dependências ativo e sem warnings de compilador nesta compilação incremental.
Isso não certifica o legado inteiro como livre de warnings.
Os 39 casos C++ de equipamento, brilho e inicialização passaram, incluindo as
duas fixtures binárias reais do emissor .NET. Também passaram 62 testes Python
de arte/integridade e 24 do catálogo/pesquisa. O verificador decodifica o JPEG
inteiro, não só o cabeçalho; um teste negativo rejeita arquivo truncado.
As três famílias de modelos passaram a comparação independente BMD/Blender.

Uma primeira seleção CTest incluiu por engano alvos não compilados de outros
módulos; não foi uma falha do cliente. A rodada correta `21,59`, conferida na lista
de testes desta build, passou os 39 casos. O recibo válido é
`scratchpad/celestial-golden-metal-20260911/client-tests-selected.xml` no workspace operacional.

As cinco [prévias equipadas](equipped-review/) e as [vistas de armadura](armor-details/)
foram regeneradas. São renders Blender e não reproduzem exatamente os reflexos
do MU. Imagens isoladas em `authored-*` e vídeos anteriores continuam históricos.
**Aparência, desempenho e animação no cliente real aguardam o teste pessoal do
usuário; nenhum jogo foi iniciado ou encerrado para esta revisão.**

O novo [Manto do Poseidon](../poseidon/design-spec.md) é um trabalho separado para
Lord Emperor. Seus protótipos não fazem parte do pacote Celestial.

## Publicação somente em Testes

Concluída em **11/09/2026 15:26:44 UTC / 12:26 Brasília**. Base imutável
`openmu-assets/celestial-test-golden-metal-20260911.zip`, 761.715.845 bytes,
SHA256 `3600ab2fd60160fe9a835d0b1abc26f0620c4fa10699b8be22432f001719047f`.
Main `ed591f15a47260cf67fcb2880bab374a5d046e8d4edacba3bc1df249820534a8`.
Somente Main e quatro OZJ mudaram; 13.832 outras entradas foram preservadas.

Às15:28:13 UTC, a API autenticada entregou a base completa e o patch; hashes e
união efetiva dos 19 arquivos protegidos conferem. O patch não contém Celestial
assets; as novas texturas vêm da base. A DLL de conexão permaneceu intacta.
Produção foi comparada antes/depois sem mudança. Não houve reinício do servidor,
alteração de banco/personagens nem instalação na cópia local do jogador.

Recibos no workspace operacional `scratchpad/celestial-golden-metal-20260911`:
`golden-metal-deployed.json`, `golden-metal-api.json`, `golden-metal-before.json`.
A base Golden anterior `10141413…e426ed` permanece intacta para retorno.
Código runtime `d49aed78`, acabamento/estudo `3a27630c`; commits locais sem push.
**Entrega do launcher verificada; aceitação visual ingame continua pendente.**
