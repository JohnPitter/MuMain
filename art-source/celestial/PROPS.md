# Celestial: acessórios próprios em revisão

Este lote reconstrói cajado, escudo, anel e pendant. Não contém a armadura final,
não constitui aprovação de fidelidade ao conceito e ainda não foi validado
no cliente do jogo. As duas unidades do anel usam a mesma malha.

## Editar e verificar

1. Executar `build_celestial_props.py` com Blender 4.2 em background. A saída
   fica em `authored-props/`; nada é copiado para instalações do jogo.
2. Abrir `authored-props/celestial-authored-props.blend`. Cada peça possui sua
   coleção. Habilitar a coleção desejada no Outliner. Cores, UVs, pedras e
   ornamentos são editáveis; os renders mostram geometria, não efeitos do MU.
3. Após mudanças nas fontes, repetir a geração e executar
   `verify_prop_roundtrip.py` também no Blender. Ele compara cada triângulo,
   normal e UV do `.blend` com a leitura independente do BMD exportado.
4. Executar `python test_authored_props.py`: verifica estrutura, índices,
   normais, triângulos, âncoras, encaixe da pedra do anel e todas as texturas.

`--no-render` após `--` dispensa apenas os renders. Prévias de uma execução
anterior não comprovam o conteúdo de uma execução nova sem render.

## Integração e aceitação

Distribuir os quatro `.bmd` e os quatro `.OZJ` em `Data/Item`, juntamente com
o executável que carrega os nomes `Celestial_*`. Não sobrescrever Staff12,
Shield13, Ring02 ou Necklace02: eles pertencem a equipamentos nativos.

O teste final deve observar inventário/baú, equipamento, chão, seleção e zoom,
com itens +0 e +15, luz diurna/noturna e qualidade de efeitos ligada/desligada.
Conferir escala na mão, orientação do escudo, brilho na gema/ponta do cajado,
e preservação de marfim/ouro/azul. Os testes de arquivo não substituem essa
verificação nem a comparação com a imagem do conceito.
