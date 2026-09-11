# Catálogo de referência dos equipamentos MU

Inventário diagnóstico dos arquivos disponíveis; não representa inspeção dentro do jogo nem garante que todos os recursos sejam equipáveis.

## Base e limites

- Base canônica: `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-ultimate-20260911\ultimate-base-downloaded.zip`.
- Alternativas permanecem separadas por origem e SHA-256; o mesmo nome pode possuir malhas ou texturas diferentes.
- Peças Class/Head/Face, esqueleto Player, ajudantes e consumíveis ficam identificados separadamente.
- Paletas medem pixels opacos dos atlas originais (amostra até 128×128, seis cores). Não medem área visível por UV, iluminação, brilho Excellent ou efeitos do jogo.
- As animações listadas são as que existem em cada BMD. Armaduras equipadas normalmente recebem os movimentos do Player.bmd; consulte o guia de execução.
- Nomes vêm das definições Season 6 e de quatro Item.bmd da base canônica. A tentativa CP932 é uma inferência de codificação, não confirmação semântica; os bytes originais e a confiabilidade ficam explícitos no JSON. Nomes não resolvidos ou potencialmente corrompidos exigem revisão.

## Cobertura

| Métrica | Total |
|---|---:|
| Bases de origem | 45 |
| Ocorrências de arquivos BMD | 20555 |
| Arquivos BMD únicos por SHA-256 | 1000 |
| Ocorrências duplicadas por conteúdo | 19555 |
| Arquivos BMD na base canônica | 1019 |
| Malhas únicas lidas com sucesso | 998 |
| Falhas de leitura de malha | 0 |
| Tabelas BMD sem malha 3D | 2 |
| Texturas únicas por SHA-256 | 1063 |
| Falhas de decodificação de textura | 0 |
| Referências de textura não resolvidas no diretório (todas as bases) | 787 |
| Referências de textura não resolvidas no diretório (base canônica) | 34 |
| Caminhos com malhas diferentes entre bases | 10 |
| Caminhos com variantes de arquivos de malha/material entre bases | 79 |

### Categorias canônicas

| Categoria | Modelos/arquivos |
|---|---:|
| Acessórios | 25 |
| Corpos/cabeças de classe | 90 |
| Consumíveis, missões ou outros | 98 |
| Peças e variantes de armadura | 380 |
| Ajudantes, mascotes ou outros | 75 |
| Referências de animação do personagem | 2 |
| Recursos auxiliares de personagem | 8 |
| Escudos | 23 |
| Recursos de item sem identidade confirmada | 154 |
| Armas | 138 |
| Asas ou capas | 26 |

### Origens

| Origem | Canônica | BMD candidatos |
|---|---|---:|
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-ultimate-20260911\ultimate-base-downloaded.zip!` | sim | 1019 |
| `C:\Users\joaop\Desenvolvimento\openmu\BloodMuV4.zip!BloodMu v4` | não | 1009 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\art-source\celestial\authored-armor!` | não | 5 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\art-source\celestial\authored-props!` | não | 4 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\art-source\celestial\authored-wings!` | não | 1 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\art-source\celestial\packaged-review\celestial-candidate-0c080d887494a28c.zip!` | não | 10 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\art-source\celestial\packaged-review\celestial-candidate-53c95bb32d1018cb.zip!` | não | 10 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\art-source\celestial\packaged-review\celestial-candidate-63aeef897617c761.zip!` | não | 10 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\art-source\celestial\packaged-review\celestial-candidate-67da30272109806a.zip!` | não | 10 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\art-source\celestial\packaged-review\celestial-candidate-e068d66146f6b4cf.zip!` | não | 10 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release!` | não | 1009 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release!` | não | 1009 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release!` | não | 1009 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86-asan\src\Release!` | não | 176 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug!` | não | 1010 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release!` | não | 1010 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin!` | não | 1010 |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release!` | não | 1009 |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4!` | não | 1009 |
| `C:\Users\joaop\Desenvolvimento\openmu\backups\Data-before-pvp-99d.zip!` | não | 1009 |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-golden-20260911\verified\celestial-candidate-0e9aef42faaa3f48.zip!` | não | 10 |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-rear-mirror-before-20260911T0031.zip!art-source/celestial/authored-armor` | não | 2 |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-release-20260911\celestial-candidate-9b302fce3c55fa63.zip!` | não | 10 |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-release-20260911\test-client-downloaded.zip!` | não | 1019 |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-ultimate-20260911\client-changes.zip!` | não | 1 |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-ultimate-20260911\verified\client-changes.zip!` | não | 1 |
| `C:\Users\joaop\Desenvolvimento\openmu\site\patch\Data.zip!` | não | 1009 |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game!` | não | 1010 |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2!` | não | 1010 |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data.zip!` | não | 1009 |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data.zip!` | não | 1009 |
| `C:\_wt-celestial-client\art-source\celestial\authored-armor!` | não | 5 |
| `C:\_wt-celestial-client\art-source\celestial\authored-props!` | não | 4 |
| `C:\_wt-celestial-client\art-source\celestial\authored-wings!` | não | 1 |
| `C:\_wt-celestial-client\art-source\celestial\packaged-review\celestial-candidate-0c080d887494a28c.zip!` | não | 10 |
| `C:\_wt-celestial-client\art-source\celestial\packaged-review\celestial-candidate-53c95bb32d1018cb.zip!` | não | 10 |
| `C:\_wt-celestial-client\art-source\celestial\packaged-review\celestial-candidate-63aeef897617c761.zip!` | não | 10 |
| `C:\_wt-celestial-client\art-source\celestial\packaged-review\celestial-candidate-67da30272109806a.zip!` | não | 10 |
| `C:\_wt-celestial-client\art-source\celestial\packaged-review\celestial-candidate-9b302fce3c55fa63.zip!` | não | 10 |
| `C:\_wt-celestial-client\art-source\celestial\packaged-review\celestial-candidate-dab88290884ff1b8.zip!` | não | 10 |
| `C:\_wt-celestial-client\art-source\celestial\packaged-review\celestial-candidate-e068d66146f6b4cf.zip!` | não | 10 |
| `C:\_wt-celestial-client\art-source\celestial\packaged-review\celestial-candidate-e74c184a7779991a.zip!` | não | 10 |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release!` | não | 1009 |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release!` | não | 1009 |
| `C:\_wt-celestial-client\src\bin!` | não | 1009 |

## Famílias de armadura canônicas

A ausência de elmo ou luvas pode ser intencional para a classe. Nº53 é carregado pelo cliente, mas não possui definição/nome confirmado; não é rotulado como equipamento utilizável.

| ID | Família (nome preservado) | Peças primárias | Polígonos BMD | Texturas |
|---:|---|---|---:|---:|
| 0 | Bronze | Helm, Armor, Pants, Gloves, Boots | 706 | 9 |
| 1 | Dragon | Helm, Armor, Pants, Gloves, Boots | 874 | 9 |
| 2 | Pad | Helm, Armor, Pants, Gloves, Boots | 654 | 11 |
| 3 | Legendary | Helm, Armor, Pants, Gloves, Boots | 682 | 8 |
| 4 | Bone | Helm, Armor, Pants, Gloves, Boots | 684 | 9 |
| 5 | Leather | Helm, Armor, Pants, Gloves, Boots | 681 | 9 |
| 6 | Scale | Helm, Armor, Pants, Gloves, Boots | 745 | 8 |
| 7 | Sphinx | Helm, Armor, Pants, Gloves, Boots | 797 | 11 |
| 8 | Brass | Helm, Armor, Pants, Gloves, Boots | 660 | 10 |
| 9 | Plate | Helm, Armor, Pants, Gloves, Boots | 694 | 7 |
| 10 | Vine | Helm, Armor, Pants, Gloves, Boots | 616 | 9 |
| 11 | Silk | Helm, Armor, Pants, Gloves, Boots | 590 | 8 |
| 12 | Wind | Helm, Armor, Pants, Gloves, Boots | 680 | 8 |
| 13 | Spirit | Helm, Armor, Pants, Gloves, Boots | 642 | 9 |
| 14 | Guardian | Helm, Armor, Pants, Gloves, Boots | 840 | 8 |
| 15 | Storm Crow | Armor, Pants, Gloves, Boots | 648 | 5 |
| 16 | Black Dragon | Helm, Armor, Pants, Gloves, Boots | 880 | 9 |
| 17 | Dark Phoenix | Helm, Armor, Pants, Gloves, Boots | 1324 | 8 |
| 18 | Grand Soul | Helm, Armor, Pants, Gloves, Boots | 800 | 8 |
| 19 | Divine | Helm, Armor, Pants, Gloves, Boots | 1588 | 9 |
| 20 | Thunder Hawk | Armor, Pants, Gloves, Boots | 949 | 5 |
| 21 | Great Dragon | Helm, Armor, Pants, Gloves, Boots | 1211 | 8 |
| 22 | Dark Soul | Helm, Armor, Pants, Gloves, Boots | 998 | 9 |
| 23 | Hurricane | Armor, Pants, Gloves, Boots | 1050 | 5 |
| 24 | Red Sprit | Helm, Armor, Pants, Gloves, Boots | 1552 | 10 |
| 25 | Light Plate | Helm, Armor, Pants, Gloves, Boots | 923 | 7 |
| 26 | Adamantine | Helm, Armor, Pants, Gloves, Boots | 1351 | 7 |
| 27 | Dark Steel | Helm, Armor, Pants, Gloves, Boots | 1053 | 8 |
| 28 | Dark Master | Helm, Armor, Pants, Gloves, Boots | 1275 | 8 |
| 29 | Dragon Knight | Helm, Armor, Pants, Gloves, Boots | 1224 | 5 |
| 30 | Venom Mist | Helm, Armor, Pants, Gloves, Boots | 1188 | 6 |
| 31 | Sylphid Ray | Helm, Armor, Pants, Gloves, Boots | 1450 | 9 |
| 32 | Volcano | Armor, Pants, Gloves, Boots | 1078 | 4 |
| 33 | Sunlight | Helm, Armor, Pants, Gloves, Boots | 1473 | 7 |
| 34 | Ashcrow | Helm, Armor, Pants, Gloves, Boots | 1203 | 6 |
| 35 | Eclipse | Helm, Armor, Pants, Gloves, Boots | 1428 | 8 |
| 36 | Iris | Helm, Armor, Pants, Gloves, Boots | 1436 | 10 |
| 37 | Valiant | Armor, Pants, Gloves, Boots | 1201 | 5 |
| 38 | Glorious | Helm, Armor, Pants, Gloves, Boots | 1403 | 7 |
| 39 | Mistery | Helm, Armor, Pants, Gloves, Boots | 1838 | 7 |
| 40 | Red Wing | Helm, Armor, Pants, Gloves, Boots | 2042 | 7 |
| 41 | Ancient | Helm, Armor, Pants, Gloves, Boots | 2064 | 9 |
| 42 | Black Rose | Helm, Armor, Pants, Gloves, Boots | 2272 | 7 |
| 43 | Aura | Helm, Armor, Pants, Gloves, Boots | 2548 | 8 |
| 44 | Lilium | Helm, Armor, Pants, Gloves, Boots | 2277 | 9 |
| 45 | Titan | Helm, Armor, Pants, Gloves, Boots | 2032 | 6 |
| 46 | Brave | Helm, Armor, Pants, Gloves, Boots | 2533 | 8 |
| 47 | Destroy | Armor, Pants, Gloves, Boots | 2050 | 6 |
| 48 | Phantom | Armor, Pants, Gloves, Boots | 1854 | 5 |
| 49 | Seraphim | Helm, Armor, Pants, Gloves, Boots | 2163 | 7 |
| 50 | Faith | Helm, Armor, Pants, Gloves, Boots | 2142 | 7 |
| 51 | Paewang | Helm, Armor, Pants, Gloves, Boots | 2000 | 7 |
| 52 | Hades | Helm, Armor, Pants, Gloves, Boots | 2019 | 6 |
| 53 | Name unresolved (nome requer revisão) | Helm, Armor, Pants, Gloves, Boots | 2128 | 8 |
| 59 | Sacred | Helm, Armor, Pants, Boots | 1958 | 2 |
| 60 | Storm Hard | Helm, Armor, Pants, Boots | 2279 | 2 |
| 61 | Piercing | Helm, Armor, Pants, Boots | 2031 | 4 |
| 62 | スケイルアーマー (nome requer revisão) | Helm, Armor, Pants, Gloves, Boots | 745 | 8 |
| 63 | シルクの鎧 (nome requer revisão) | Helm, Armor, Pants, Gloves, Boots | 590 | 8 |
| 64 | スピンクスアーマー (nome requer revisão) | Helm, Armor, Pants, Gloves, Boots | 797 | 11 |
| 65 | サイクロンアーマー (nome requer revisão) | Helm, Armor, Pants, Gloves, Boots | 1838 | 7 |
| 66 | ダークソウルアーマー (nome requer revisão) | Helm, Armor, Pants, Gloves, Boots | 1351 | 7 |
| 67 | ドラゴンアーマー (nome requer revisão) | Helm, Armor, Pants, Gloves, Boots | 874 | 9 |
| 68 | パルテナの鎧 (nome requer revisão) | Helm, Armor, Pants, Gloves, Boots | 840 | 8 |
| 69 | オーディーンの鎧 (nome requer revisão) | Helm, Armor, Pants, Gloves, Boots | 682 | 7 |
| 70 | レッドウィングアーマー (nome requer revisão) | Helm, Armor, Pants, Gloves, Boots | 2042 | 6 |
| 71 | ケルベロスアーマー (nome requer revisão) | Armor, Pants, Gloves, Boots | 648 | 5 |
| 72 | ストームハザードアーマー (nome requer revisão) | Helm, Armor, Pants, Boots | 2279 | 2 |
| 73 | Phoenix Soul | Helm, Armor, Pants, Boots | 3148 | 7 |
| 74 | Celestial | Helm, Armor, Pants, Gloves, Boots | 39486 | 3 |

## Falhas e recursos não resolvidos

Nenhuma falha de leitura BMD no conjunto inventariado.

Referências de textura ausentes são registradas por ocorrência em `catalog.json`; podem ser recursos externos do renderer, placeholders ou arquivos realmente faltantes. Não são substituídas por texturas de outra origem.

## Arquivos do catálogo

- `catalog.json`: todas as ocorrências, modelos únicos, ossos, ações, materiais, texturas e paletas.
- `canonical-families.json`: caminhos originais no ZIP agrupados por família/peça para inspeção 3D.
- `item-names.json`: nomes e requisitos declarados nas fontes; não substitui configuração persistida do servidor.
- [Detalhes das armaduras](armor-details.md) e [demais equipamentos](equipment-details.md): métricas e materiais peça a peça.
- [Cobertura e pendências](coverage.md): referências externas e diferenças entre origens.
- `texture-sheets/`: pranchas diagnósticas dos atlas reais, sem recoloração.
- `texture-thumbnails/`: amostras diagnósticas deduplicadas por hash.
- `equipment-effects-index.json` e `runtime-guide.md`: índice de efeitos e funcionamento do renderer (levantamento complementar).

### Pranchas de texturas

- [textures-01.jpg](texture-sheets/textures-01.jpg)
- [textures-02.jpg](texture-sheets/textures-02.jpg)
- [textures-03.jpg](texture-sheets/textures-03.jpg)
- [textures-04.jpg](texture-sheets/textures-04.jpg)
- [textures-05.jpg](texture-sheets/textures-05.jpg)
- [textures-06.jpg](texture-sheets/textures-06.jpg)
- [textures-07.jpg](texture-sheets/textures-07.jpg)
- [textures-08.jpg](texture-sheets/textures-08.jpg)
- [textures-09.jpg](texture-sheets/textures-09.jpg)
- [textures-10.jpg](texture-sheets/textures-10.jpg)
- [textures-11.jpg](texture-sheets/textures-11.jpg)
- [textures-12.jpg](texture-sheets/textures-12.jpg)
- [textures-13.jpg](texture-sheets/textures-13.jpg)
- [textures-14.jpg](texture-sheets/textures-14.jpg)
- [textures-15.jpg](texture-sheets/textures-15.jpg)
- [textures-16.jpg](texture-sheets/textures-16.jpg)
- [textures-17.jpg](texture-sheets/textures-17.jpg)
- [textures-18.jpg](texture-sheets/textures-18.jpg)
- [textures-19.jpg](texture-sheets/textures-19.jpg)
- [textures-20.jpg](texture-sheets/textures-20.jpg)
- [textures-21.jpg](texture-sheets/textures-21.jpg)
- [textures-22.jpg](texture-sheets/textures-22.jpg)
- [textures-23.jpg](texture-sheets/textures-23.jpg)
- [textures-24.jpg](texture-sheets/textures-24.jpg)
- [textures-25.jpg](texture-sheets/textures-25.jpg)
- [textures-26.jpg](texture-sheets/textures-26.jpg)
- [textures-27.jpg](texture-sheets/textures-27.jpg)
- [textures-28.jpg](texture-sheets/textures-28.jpg)
- [textures-29.jpg](texture-sheets/textures-29.jpg)
- [textures-30.jpg](texture-sheets/textures-30.jpg)
- [textures-31.jpg](texture-sheets/textures-31.jpg)
- [textures-32.jpg](texture-sheets/textures-32.jpg)
- [textures-33.jpg](texture-sheets/textures-33.jpg)
- [textures-34.jpg](texture-sheets/textures-34.jpg)
- [textures-35.jpg](texture-sheets/textures-35.jpg)
- [textures-36.jpg](texture-sheets/textures-36.jpg)
