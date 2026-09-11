# Celestial — revisão dourada de 11/09/2026

## Direção visual

Atende ao retorno do usuário após o teste da revisão Ultimate: grandes superfícies
antes marfim passam a ouro; frisos antes dourados passam a marfim claro. Safiras
azuis e elementos emissivos mantêm sua identidade. Corrente/bail do pendant e
estrutura externa dos anéis continuam dourados; relevos ficam claros.

`celestial_palette.py` é a fonte única dos papéis `Base`, `Trim`, `Sapphire` e
`Emissive`. Os geradores de armadura, acessórios e asas usam esses papéis, não
trocas globais do significado físico de Gold/Ivory. As quatro texturas originais
continuam com os mesmos bytes e nomes. Alguns nomes históricos de objetos ainda
contêm gold/ivory; o material atribuído é a referência de cor.

## Brilho no cliente

O passe genérico Excellent/Ancient era aplicado depois do brilho próprio do
Celestial, somando luz novamente sobre o modelo inteiro. Somente os dez modelos
Celestial deixam de receber esse passe genérico; suas opções de item continuam
intactas. Outros equipamentos conservam a condição anterior.

O brilho dedicado agora limita ouro, marfim, safira e emissão separadamente.
Intensidade e alpha são pré-multiplicados em RGB para o blending aditivo ONE/ONE,
inclusive no caminho CPU. A emissão usa o caminho unlit existente. Halo e marfim
recebem intensidades menores para preservar contornos sem transformar tudo em
uma massa branca. A intensidade final em áreas sobrepostas exige teste ingame.

## Evidências e integridade

- Dez BMD regenerados; 101.252 triângulos no conjunto de modelos, sem aumento.
- Posições, normais, bones, orientação dos triângulos, bind pose, ações e movimento
  da raiz preservados contra a base Ultimate anterior. Não houve nova animação.
- UVs exatos em oito modelos. Somente UVs metálicos de Ring/Pendant admitem
  até 0,000001 por deduplicação do exportador; maior diferença medida no pendant
  foi 0,000000924. Safira/emissão permanecem exatas. Testes negativos cobrem
  alterações indevidas na geometria, animação, materiais e UVs.
- Prova: [palette-revision-integrity.json](authored-props/palette-revision-integrity.json).
- Verificadores independentes BMD/Blender aprovados para armaduras, asas e acessórios.
- 47 testes Python e 34 casos C++ aprovados, com fixtures binárias reais do backend.
- Build Main aprovado; warning legado C4312 em ZzzObject continua fora desta correção.

## Prévias válidas desta paleta

As cinco imagens e cenas `.blend` de [equipped-review](equipped-review/) foram
regeneradas e mostram a paleta atual: [repouso](equipped-review/Celestial_Equipped_idle.png),
[costas](equipped-review/Celestial_Equipped_rear.png),
[lateral](equipped-review/Celestial_Equipped_side.png),
[magia](equipped-review/Celestial_Equipped_cast.png) e
[voo](equipped-review/Celestial_Equipped_flight.png).
São renders Blender das malhas, não screenshots do jogo nem simulação exata de
seus passes de brilho. As cenas-fonte `authored-*/celestial-authored-*.blend` estão atuais.

**Prévia arquivada:** imagens isoladas dentro de `authored-*`, `armor-details` e
vídeos de movimento não foram regenerados nesta revisão. Servem apenas para a
geometria/movimento anterior, não para avaliar a cor atual.

Reprodução: executar os três builders com Blender 4.2, os três verificadores
roundtrip e `review_equipped_set.py`; em seguida `verify_palette_revision.py`
contra a base Ultimate anterior. Empacotar somente após obter os novos manifests
e relatórios com hashes correspondentes, usando `package_candidate.py`.

Publicação restrita ao cliente de Testes. Sem mudança de regras, migração de
banco, personagens, baú, DLL de conexão ou reinício do servidor. O resultado de
distribuição fica no CHANGELOG operacional do workspace; fidelidade, saturação
com outros efeitos e desempenho aguardam validação pessoal do usuário.
