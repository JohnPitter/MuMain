# Loading inicial animado

A tela de carregamento inicial mantém os quatro painéis `LSBg01.JPG`–`LSBg04.JPG` e desenha uma barra com brilho pulsante. O progresso usa o contador existente `LoadingWorld`; enquanto o contador está no estado sentinela, o brilho percorre a barra inteira para indicar uma espera indeterminada.

A animação é **multi-frame**: a primeira chamada de `LoadingScene` cria a cena, renderiza e encaminha o fluxo para `MAIN_SCENE`, mas não libera a cena nem as texturas. Enquanto o carregamento ainda está ativo (`LoadingWorld > 30` ou `EnableMainRender == false`), o caminho de renderização de `MAIN_SCENE` reapresenta o overlay a cada frame. Quando o carregamento termina, o overlay é liberado uma única vez e a renderização normal assume, sem reentrância.

Arquivos alterados:

- `src/source/Scenes/LoadingScene.h`: expõe o ciclo de vida do overlay e mantém o relógio da animação.
- `src/source/Scenes/LoadingScene.cpp`: desenha trilho, preenchimento e brilho pulsante e mantém a cena viva até a transição final.
- `src/source/Scenes/SceneManager.cpp`: reapresenta o overlay durante o carregamento e libera-o somente quando a renderização principal pode assumir.

Não são necessárias screenshots. A animação usa apenas o estado de carregamento já mantido pelo client e não bloqueia o fluxo de startup.
