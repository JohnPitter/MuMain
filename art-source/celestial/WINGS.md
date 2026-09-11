# Celestial: asas próprias, animação e montagem em revisão

Estado mais recente: [revisão dourada de 11/09/2026](GOLDEN.md). Penas principais
douradas, detalhes claros, movimento preservado. As seções abaixo são históricas;
vídeos/imagens isolados antigos não representam a paleta nova.

**Candidato local, não publicado e ainda não idêntico ao conceito.** As imagens
e vídeos são renderizações das malhas reais no Blender, não capturas do MU.

## Conteúdo e origem

- Geometria própria: 114 penas em três camadas por lado, hastes e cristas
  douradas, gemas, halo duplo e raios. Nenhuma superfície de Wing44 é copiada.
- Wing44 fornece somente esqueleto e movimento: 47 ossos nativos, uma
  sequência de nove quadros. O osso 47 adicional é a âncora do halo.
- Cada vértice tem uma influência inteira. O BMD preserva `LockPositions`
  e os nove vetores de posição da ação; a prévia reproduz a trava XY da raiz.
- Exportador compartilhado `export_skinned_bmd.py`: BMD v10, 28.870
  triângulos em 15 meshes e três texturas próprias. Reexportar as cinco
  armaduras por ele preservou os respectivos BMD byte a byte.
- Brilho do cliente é dourado, localizado no halo, respeitando distância
  e efeitos desligados. Não há mais cobertura azul sobre todas as penas
  nem fallback silencioso para asas nativas neste cliente de desenvolvimento.

## Reprodução no Blender 4.2

Usar `-b --python-exit-code 1 --python <script>`:

1. `build_authored_wings.py -- <Data/Item do cliente>`.
2. `verify_wings_roundtrip.py` depois da geração. Compara todos os cantos,
   normais, UVs, ossos e os nove keyframes; também verifica regressão da armadura.
3. `render_wing_motion.py`: 32 quadros a 24 fps, 540x450, H.264.
4. `review_equipped_set.py -- <Data/Player do cliente>`: repouso, magia, voo,
   costas e lateral,
   a partir de todos os BMD decodificados e das ligações nativas do jogador.

As três saídas ficam em `authored-wings/` e `equipped-review/`; nenhum script
instala arquivos em Game/Game2. `--no-render` na geração dispensa só as imagens,
que não podem ser usadas como prova de uma geração posterior.

O cajado na mão acompanha diretamente o osso 33 e o escudo o 42. Asas usam
o 47 com deslocamento local Z=15, conforme `RenderLinkObject`. A rotação extra
da arma guardada nas costas não se aplica à arma segurada. Joias continuam
sendo modelos do inventário; o cliente não as desenha vestidas no personagem.

## Verificação e pacote

Executar no diretório desta documentação:

```text
python -m unittest test_authored_props test_authored_armor test_authored_wings test_candidate_package -v
python package_candidate.py <caminho do Main.exe Release x64>
```

São 29 testes. Relatórios de geometria registram os hashes exatos do projeto
editável e BMD comparados. O empacotador rejeita provas antigas, modelos ou
texturas ausentes, divergências de manifesto e texturas compartilhadas
conflitantes. Produz ZIP determinístico com dez BMD, sete OZJ e Main.exe;
duas unidades do anel compartilham um BMD. Não contém servidor, Item.bmd,
DLL de conexão ou cliente base; não é instalação autossuficiente nem deploy.

Erro máximo medido nas poses das asas: 0,000112 unidade; ida e volta da
geometria: inferior a 0,000001. Os 138.141 vértices comparados são amostras
acumuladas dos nove quadros, não o tamanho de uma única malha.

## Diferenças visuais e critérios ainda pendentes

As penas foram curvadas, suavizadas e escalonadas, com silhueta mais vertical.
A barra contínua que dobrava abruptamente foi substituída por 48 folhas
sobrepostas, cada uma rigidamente ligada a um osso. A prévia da meia-batida
não tem a quebra angular da barra anterior. Não é prova de ausência de
interpenetrações em todas as poses.

Ainda faltam a variedade e o acabamento dos ornamentos dourados do conceito.
As penas continuam geometricamente regulares e a cor precisa ser conferida
no renderer real, não pelo reflexo PBR do Blender. Coroa, peitoral e botas
também receberam detalhes, sem aprovação de fidelidade. Esses pontos não
estão aprovados por passarem nos testes estruturais.

A avaliação final precisa acontecer no cliente: inventário, baú, chão,
seleção, +0/+15, caminhada/corrida, magia/ataque, morte e voo; Soul Master
e Grand Master. Medir interpenetrações, escala e desempenho com vários
jogadores. Nenhum cliente foi iniciado nesta etapa. Não publicar este pacote
como visual final nem sobrescrever outras melhorias/DLL já distribuídas.
