# Celestial — detalhes platinados com referência no Kundun Staff

Revisão de 11/09/2026, commit `535a9b4f`, sobre `f4c443b8`. Escopo: **somente o acabamento de `Celestial_Ivory.jpg`**; o nome continua por compatibilidade. Ouro, pedras, halo, texturas, malhas, UVs, rig, animações e opções de item não são alterados. Publicada somente em Testes, conforme registro ao final; nenhum jogo foi iniciado ou encerrado pelo agente.

## Referência real, não uma suposta cor “Excellent prata”

`MODEL_STAFF_OF_KUNDUN = MODEL_STAFF + 11` em [enum](C:/_wt-celestial-client/src/source/Core/Globals/_enum.h:1613); o loader abre explicitamente `Data/Item/Staff12.bmd` em [ZzzOpenData.cpp:761](C:/_wt-celestial-client/src/source/Engine/Object/ZzzOpenData.cpp:761). A inspeção dos atlas reais está em [kundun-staff/measurements.json](C:/_wt-celestial-client/art-source/celestial/research/kundun-staff/measurements.json). O atlas principal já contém cinza/prata e highlights pintados; o acabamento final depende também dos passes.

O tint específico de `PartObjectColor` para o Kundun é **Color 17, RGB `(.65, .45, .30)`**, um tom bronze, não platina: seleção em [ZzzObject.cpp:6721](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:6721) e tabela em [6908](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:6908). Copiar esse tint para o cinza Celestial o deixaria amarronzado, contrariando o pedido.

### Composição nativa do staff

O bloco específico em [ZzzObject.cpp:8106](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:8106) desenha:

1. Corpo com textura normal, respeitando o tipo de render recebido.
2. Malha 1 com `RENDER_CHROME | RENDER_BRIGHT`.
3. Malha 0 com `RENDER_TEXTURE | RENDER_BRIGHT` e pulsação `sin(WorldTime × .005)`.
4. Malha 0 com `RENDER_CHROME4 | RENDER_BRIGHT`.

O argumento `.2` da chamada Chrome da malha 1 é `blendMeshAlpha`, não o alpha do objeto; não deve ser lido como “20% de brilho” universal. O ramo Chrome em `RenderMesh` tem seu próprio tratamento de RGB/blend. Há ainda `BlendMeshLight = sin(WorldTime × .004) × .3 + .7` em [ZzzObject.cpp:10096](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:10096).

O +15 genérico, com qualidade suficiente, acrescenta **CHROME4 + METAL + CHROME**, com base escalada em `.9`: [pipeline +13/+15](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:10539). No shader combinado, `(base + chrome2) × luzDaBase + (metal + chrome1) × tintDoItem`: [BMDMeshShader.cpp:370](C:/_wt-celestial-client/src/source/Render/Shaders/BMDMeshShader.cpp:370). Isso não equivale a um único branco fixo.

Excellent também não significa prata: a camada adicional é `RENDER_TEXTURE | RENDER_BRIGHT` com cor `(L, .3L, 1−L)` e `L = sin(WorldTime × .002) × .5 + .5`: [ZzzObject.cpp:10647](C:/_wt-celestial-client/src/source/Engine/Object/ZzzObject.cpp:10647). Essa oscilação vermelho/azul não foi copiada para o Celestial e nenhuma flag Excellent foi modificada.

## Adaptação escolhida para o detalhe platinado

Usam-se **dois tipos de reflexo presentes no próprio Kundun** sobre a textura cinza já existente, não uma cópia integral de todas as suas camadas:

| Passe só do detalhe | Textura nativa explícita | Cor | Força +0 → +15 |
| --- | --- | --- | --- |
| `CHROME4 | BRIGHT` | `BITMAP_CHROME2` / `Effect/Chrome02.jpg` | Branco neutro `(1,1,1)` | `.65 → .90` |
| `CHROME | BRIGHT` | `BITMAP_CHROME` / `Effect/Chrome01.jpg` | Branco neutro `(1,1,1)` | `.25 → .35` |

O branco neutro e as forças acima são **uma decisão artística para a leitura platinada**, não números alegadamente extraídos do material completo do Kundun. O objetivo é reflexo branco/prateado móvel sobre áreas ainda cinza entre highlights. O total máximo teórico é `1.25` por canal antes da amostragem das texturas; pontos podem saturar como highlights metálicos, mas isso não é uma emissão branca uniforme. A auréola e a emissão das pedras não aumentaram.

`CHROME4` calcula a projeção a partir da normal e do tempo; utiliza o mapa `Chrome02`, não `Chrome04`: [ZzzBMD.cpp:1489](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:1489), [fórmula UV](C:/_wt-celestial-client/src/source/Render/Models/ZzzBMD.cpp:1648), [loader](C:/_wt-celestial-client/src/source/Engine/Object/ZzzOpenData.cpp:5550). O binding explícito preserva a correção de sampler da revisão Golden. Não foi adicionado shader ou alterada infraestrutura global.

O número de passes continua em dois para o detalhe. A troca de METAL para CHROME4 muda o caminho: o CHROME4 isolado usa cálculo CPU de normais/UV e VBO dinâmico existente, enquanto o Chrome simples pode usar skinning GPU. Portanto igual quantidade de draw calls **não prova igual custo de CPU**. Qualidade abaixo de 2 e distância/cloaking/fade continuam obedecendo às mesmas restrições Celestial.

## Preservação e validação

O único novo case em cada mapeamento do adaptador é `Chrome4 → RENDER_CHROME4 / BITMAP_CHROME2`. Os valores e a ordem dos enums anteriores permanecem intactos. A função Golden continua com RGB `(1,.5,0)`, METAL `.90+.10×upgrade` e CHROME `.70+.30×upgrade`, sem nenhuma mudança.

Os testes em [test_celestial_shimmer.cpp](C:/_wt-celestial-client/tests/celestial/test_celestial_shimmer.cpp) verificam as famílias CHROME4/CHROME, branco neutro, força/fade limitado e ausência de emissão sólida. O novo teste de regressão compara **exatamente** o perfil dourado para todos os níveis 0–15 e qualidades 1–4. Os testes já existentes continuam cobrindo ouro no +0/+15, qualidade desligada, gems, halo e entradas não finitas.

Build Release incremental aprovado sem warnings de compilador, 41 casos CTest aprovados com fixtures reais .NET. Ouro exatamente preservado em16níveis×4qualidades; candidato auditado com os17assets byte-idênticos à base Golden Metal. Próxima validação: o usuário comparar o detalhe com um Kundun Staff Excellent +15 dentro do mesmo mapa/câmera. Os dois passes escolhidos não reproduzem todas as máscaras, UVs, geometria, camada bronze e oscilação Excellent do staff original.

## Publicação e passagem de contexto

Somente em Testes, 11/09/2026 16:20:59 UTC: base `openmu-assets/celestial-test-platinum-20260911.zip`, SHA256 `6d769784a07cf4f7d7a3cb76353bd15f1c80282c475ba2fe2ea4a4e8080f6c42`. Só Main mudou;13.836outras entradas preservadas. Main SHA256 `3d59dfa6d4bfc259c4cd985c582c3414b90a3b92904a0ef7277b8a3af96eda02`.

API autenticada conferida às16:22:40UTC: base inteira efetivamente baixada e19arquivos protegidos corretos após o overlay. Produção, DB, baú, servidor e instalação local não alterados; base anteriorGoldenMetal3600ab2f…719047f conservada para retorno. Recibos em `C:/Users/joaop/Desenvolvimento/openmu/scratchpad/celestial-platinum-20260911`.

Resumo completo para continuidade: [CONTINUIDADE_CELESTIAL_POSEIDON.md](C:/Users/joaop/Desenvolvimento/openmu/CONTINUIDADE_CELESTIAL_POSEIDON.md). O Poseidon segue como protótipo separado. **Entrega do launcher validada; fidelidade e desempenho ingame ainda pendentes.**
