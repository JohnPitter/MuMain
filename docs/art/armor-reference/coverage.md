# Cobertura e referências pendentes

Nenhuma textura é buscada em outra instalação. Candidatos externos abaixo existem na mesma origem, mas só o carregador/renderer confirma qual diretório usar.

Casos comprovados: `hide_m.jpg` usa o prefixo `hid`, convertido em `BITMAP_HIDE` pelo carregador; o renderer não desenha essa malha. O arquivo físico não é exigido. O elmo Lucky70 usa `head helmet Luck 40.jpg`, encontrado em Lucky65; o cliente oferece reutilização por nome de textura já carregada (`LoadData.cpp:108`), e carrega Lucky65 antes de Lucky70. A referência continua marcada como externa ao diretório local, com os candidatos preservados abaixo.

## Materiais canônicos fora do diretório do modelo

| BMD | Material | Candidatos na mesma base |
|---|---|---|
| `data/item/darkhorsehorn.bmd` | `dkhead2.JPG` | data/skill/dkhead2.ozj |
| `data/item/darkhorsehorn.bmd` | `dkheadwing.JPG` | data/skill/dkheadwing.ozj |
| `data/item/lifestoneitem.bmd` | `Monster81.jpg` | data/monster/monster81.ozj |
| `data/item/lifestoneitem.bmd` | `Monster81_R2.jpg` | data/monster/monster81_r2.ozj |
| `data/item/partcharge2/expensiveitem04b.bmd` | `expensiveitem03bc.jpg` | data/item/partcharge1/expensiveitem03bc.ozj |
| `data/item/partcharge4/curemark.bmd` | `monmark02a.jpg` | data/item/monmark02a.ozj<br>data/item/partcharge1/monmark02a.ozj |
| `data/item/partcharge6/suhocham01.bmd` | `flareBlue.jpg` | data/effect/flareblue.ozj |
| `data/item/partcharge7/esdpotion.bmd` | `EPotion_R.jpg` | data/item/partcharge2/epotion_r.ozj |
| `data/item/s30_seed.bmd` | `chysukmoon.jpg` | data/effect/chysukmoon.ozj |
| `data/item/s30_sphere01.bmd` | `chysukmoon.jpg` | data/effect/chysukmoon.ozj |
| `data/item/s30_sphere02.bmd` | `chysukmoon.jpg` | data/effect/chysukmoon.ozj |
| `data/item/s30_sphere03.bmd` | `chysukmoon.jpg` | data/effect/chysukmoon.ozj |
| `data/item/s30_sphere04.bmd` | `chysukmoon.jpg` | data/effect/chysukmoon.ozj |
| `data/item/s30_sphere05.bmd` | `chysukmoon.jpg` | data/effect/chysukmoon.ozj |
| `data/item/shield_18.bmd` | `flareRed.jpg` | data/effect/flarered.ozj<br>data/item/xmas/flarered.ozj |
| `data/item/spiritbill.bmd` | `dkshelmet.jpg` | data/npc/dkshelmet.ozj<br>data/skill/dkshelmet.ozj |
| `data/item/spiritbill.bmd` | `dksbody.tga` | data/npc/dksbody.ozt<br>data/skill/dksbody.ozt |
| `data/item/sword34.bmd` | `Item762_Armor.jpg` | data/player/item762_armor.ozj |
| `data/item/sword34.bmd` | `Item762_Helm.jpg` | data/player/item762_helm.ozj |
| `data/item/sword36.bmd` | `gra.jpg` | data/effect/gra.ozj<br>data/item/gra.ozj<br>data/monster/gra.ozj<br>data/skill/gra.ozj |
| `data/item/swordl34.bmd` | `Item762_Armor.jpg` | data/player/item762_armor.ozj |
| `data/item/swordl34.bmd` | `Item762_Helm.jpg` | data/player/item762_helm.ozj |
| `data/item/swordr34.bmd` | `Item762_Armor.jpg` | data/player/item762_armor.ozj |
| `data/item/swordr34.bmd` | `Item762_Helm.jpg` | data/player/item762_helm.ozj |
| `data/item/waterring.bmd` | `waterring.JPG` | nenhum |
| `data/player/angel.bmd` | `ng01.jpg` | data/npc/ng01.ozj<br>data/object12/ng01.ozj<br>data/object25/ng01.ozj |
| `data/player/helper01.bmd` | `fairy.jpg` | data/item/fairy.ozj |
| `data/player/helper01.bmd` | `fairy2.jpg` | data/item/fairy2.ozj |
| `data/player/helper02.bmd` | `satan.jpg` | data/item/satan.ozj |
| `data/player/helper02.bmd` | `satan2.tga` | data/item/satan2.ozt |
| `data/player/helper03.bmd` | `unicon.jpg` | data/item/unicon.ozj |
| `data/player/helper03.bmd` | `unicon01.tga` | data/item/unicon01.ozt |
| `data/player/luckyitem/70/new_helm09.bmd` | `head helmet Luck 40.jpg` | data/player/luckyitem/65/head helmet luck 40.ozj |
| `data/player/wing02.bmd` | `angel_wing.tga` | data/item/angel_wing.ozt<br>data/item/ingameshop/angel_wing.ozj<br>data/item/ingameshop/angel_wing.ozt |

## Mesmo caminho, arquivos de componentes diferentes entre origens

Agrupamento pelo SHA bruto da malha e lista ordenada de arquivos de textura; diferenças de wrapper/compactação podem representar os mesmos pixels. Isso não prova aparência visual diferente. Uma textura não resolvida é preservada como tal, sem supor equivalência.

| Caminho | Variantes | SHA das malhas |
|---|---:|---|
| `data/item/agnecklace.bmd` | 2 | c41c7a2d6ad79848 |
| `data/item/ancientstatue.bmd` | 2 | c5e3e27bfce82e05 |
| `data/item/antidote01.bmd` | 2 | a45db1fcb476651f |
| `data/item/bead.bmd` | 2 | be3fb57e45fb79b7 |
| `data/item/beer02.bmd` | 2 | a11fd6cde60f4bca |
| `data/item/blue01.bmd` | 2 | 88a546c4dd9e0100 |
| `data/item/bow20.bmd` | 2 | 69ee5fecb5a9720d |
| `data/item/bow_24.bmd` | 2 | 29fbcb7e48cd11a8 |
| `data/item/celestial_pendant.bmd` | 2 | 543966a6807d5009, 7d983077eeaceb13 |
| `data/item/celestial_ring.bmd` | 2 | 27f799ad23178a4f, bc49208efa3b68b6 |
| `data/item/celestial_shield.bmd` | 2 | 74933f78247bfbc3, ddfbacdf436e9469 |
| `data/item/celestial_staff.bmd` | 2 | 35b30916ab1a7c03, 5ac97450a5ae8f3c |
| `data/item/celestial_wings.bmd` | 4 | a26654803c1f7ed9, b1eddd650dd7b445, ec9d30497f835b8d, eceb007501a07385 |
| `data/item/crossbow06.bmd` | 2 | bb4ecb436a8c9790 |
| `data/item/crossbow07.bmd` | 2 | 3ba0110930e2421d |
| `data/item/cw_sword.bmd` | 2 | 2f1f676bb0f50c30 |
| `data/item/cw_sword2.bmd` | 2 | 8d19ad6fa72a221b |
| `data/item/devil02.bmd` | 2 | 1bf4fab5671f7f94 |
| `data/item/dn3_darksw.bmd` | 2 | 2f1f676bb0f50c30 |
| `data/item/drink00.bmd` | 2 | 4aad6074c546fd73 |
| `data/item/event02.bmd` | 2 | 86f0554927ab1551 |
| `data/item/event03.bmd` | 2 | c3aef21c3c05f299 |
| `data/item/eventbloodcastle02.bmd` | 2 | 3de1f0063208d239 |
| `data/item/eventbloodcastle03.bmd` | 2 | 290013d085bcc593 |
| `data/item/eventbloodcastle04.bmd` | 2 | d22d529bf19788c0 |
| `data/item/eventbloodcastle05.bmd` | 2 | d22d529bf19788c0 |
| `data/item/eventbloodcastle06.bmd` | 2 | d22d529bf19788c0 |
| `data/item/eventchaoscastle.bmd` | 2 | 534740af2ac70150 |
| `data/item/gamble_stick.bmd` | 2 | 3b9fa9056844478f |
| `data/item/gamble_stickx01.bmd` | 2 | cef8b3d215cab0ac |
| `data/item/gem01.bmd` | 2 | be3fb57e45fb79b7 |
| `data/item/gem02.bmd` | 2 | be3fb57e45fb79b7 |
| `data/item/gem03.bmd` | 2 | be3fb57e45fb79b7 |
| `data/item/gem04.bmd` | 2 | be3fb57e45fb79b7 |
| `data/item/gem05.bmd` | 2 | be3fb57e45fb79b7 |
| `data/item/gem06.bmd` | 2 | a272db2c20f3efe6 |
| `data/item/gem07.bmd` | 2 | 9780f0afbb63f16b |
| `data/item/gem08.bmd` | 2 | 9780f0afbb63f16b |
| `data/item/gem10.bmd` | 2 | 9780f0afbb63f16b |
| `data/item/gem11.bmd` | 2 | 9780f0afbb63f16b |
| `data/item/gem12.bmd` | 2 | 9780f0afbb63f16b |
| `data/item/gem13.bmd` | 2 | 9780f0afbb63f16b |
| `data/item/gem14.bmd` | 2 | 64be02cebe994369 |
| `data/item/giftbox_b.bmd` | 2 | f8591f582474b99c |
| `data/item/giftbox_bb.bmd` | 2 | ceaf87d624491f42 |
| `data/item/giftbox_bp.bmd` | 2 | 8725de5ff34345cd |
| `data/item/giftbox_br.bmd` | 2 | 7b5a3bf87c23cf1d |
| `data/item/giftbox_g.bmd` | 2 | 8232d854fd3dfc77 |
| `data/item/giftbox_r.bmd` | 2 | e3af5af149702085 |
| `data/item/gm01.bmd` | 2 | 837724474687b81d |
| `data/item/gm02.bmd` | 2 | c8192fb1c765d7a9 |
| `data/item/hellasitem00.bmd` | 2 | 4be53b18b72bb841 |
| `data/item/hellasitem01.bmd` | 2 | 34b697e9c468e9dd |
| `data/item/hellowinscroll.bmd` | 2 | fde5fd282d1f76fa |
| `data/item/hobakhead.bmd` | 2 | 0f2ea3b0042b4020 |
| `data/item/horseshoe.bmd` | 2 | 93afbfb6eb98e6ef |
| `data/item/icenecklace.bmd` | 2 | 41d17388a7c38da8 |
| `data/player/armorclass03.bmd` | 2 | daa11bc586320ed1 |
| `data/player/armorelf01.bmd` | 2 | 4ec7058878f5e8a1 |
| `data/player/armorelf02.bmd` | 2 | fb59496f1e19d3d3 |
| `data/player/armorelf03.bmd` | 2 | 8c2747ce08ef6280 |
| `data/player/armorelf04.bmd` | 2 | 12f713fa7b56300d |
| `data/player/armorelf05.bmd` | 2 | db81571d430ee4aa |
| `data/player/bootclass03.bmd` | 2 | dd7e037db38d1cc6 |
| `data/player/celestial_armor.bmd` | 6 | 812e73d01f18990d, 9806cfc0de83caa6, a4a8dc17f9db6450, ae2af9809dd1e954, d72e091f1c06e539 |
| `data/player/celestial_boots.bmd` | 6 | 2b0fa6e2bf66dd99, 3de97201eead854e, 9d8019f8aae2bf04, ce797b8db53393b2, fe3444e620831180 |
| `data/player/celestial_gloves.bmd` | 3 | 351bfe2d3914824f, 77efd4a100fcc4c4, 8b1bf2c0300383a0 |
| `data/player/celestial_helm.bmd` | 5 | 3e96319b37bc1e11, 4847619d47b889bf, 6440a384f32506dc, f298fdc1aa3533ef |
| `data/player/celestial_pants.bmd` | 3 | 16e3779737335fde, c9dcd8394a862106, d2e480f1e198fed2 |
| `data/player/gloveclass03.bmd` | 2 | dd5eeb3a3c8a8ba8 |
| `data/player/helmclass03.bmd` | 2 | df5e615415d3d0d6 |
| `data/player/helmelf05.bmd` | 2 | f6b63677d7e59307 |
| `data/player/helmmale04.bmd` | 2 | a8eb9171b3e5cfe2 |
| `data/player/helmmale05.bmd` | 2 | 1d14dee629610aab |
| `data/player/helper04.bmd` | 2 | 53a97468001f6beb |
| `data/player/pantclass03.bmd` | 2 | eb6e977521c87ba3 |
| `data/player/pantelf01.bmd` | 2 | 6059609506789bcb |
| `data/player/pantelf02.bmd` | 2 | e0dd32cc975031d4 |
| `data/player/pantelf03.bmd` | 2 | 5a2b6300acc378a9 |

## Arquivos/arquivos compactados fora do levantamento 3D

| Caminho | Motivo |
|---|---|
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\InGameShopScript\512.2011.006.rar` | unsupported_archive_format |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\Logo\Face01.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\Logo\Face02.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\Logo\Face03.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\Logo\Face04.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\Logo\face05.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\Monster\ex01shadow_master_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\Monster\ex01shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\Monster\shadow_knight_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\Monster\shadow_pawn_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\Monster\shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\NPC\FemaleBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\NPC\FemaleBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\NPC\ManBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\NPC\ManBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\NPC\ManGloves01.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64\src\Release\Data\NPC\ManGloves02.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\InGameShopScript\512.2011.006.rar` | unsupported_archive_format |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\Logo\Face01.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\Logo\Face02.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\Logo\Face03.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\Logo\Face04.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\Logo\face05.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\Monster\ex01shadow_master_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\Monster\ex01shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\Monster\shadow_knight_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\Monster\shadow_pawn_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\Monster\shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\NPC\FemaleBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\NPC\FemaleBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\NPC\ManBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\NPC\ManBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\NPC\ManGloves01.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\out\build\windows-x64-startup-fix\src\Release\Data\NPC\ManGloves02.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\InGameShopScript\512.2011.006.rar` | unsupported_archive_format |
| `C:\_wt-celestial-client\src\bin\Data\Logo\Face01.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\Logo\Face02.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\Logo\Face03.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\Logo\Face04.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\Logo\face05.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\Monster\ex01shadow_master_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\Monster\ex01shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\Monster\shadow_knight_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\Monster\shadow_pawn_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\Monster\shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\NPC\FemaleBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\NPC\FemaleBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\NPC\ManBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\NPC\ManBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\NPC\ManGloves01.bmd` | loose_model_outside_Data_Player |
| `C:\_wt-celestial-client\src\bin\Data\NPC\ManGloves02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\Logo\Face01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\Logo\Face02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\Logo\Face03.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\Logo\Face04.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\Logo\face05.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\Monster\ex01shadow_master_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\Monster\ex01shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\Monster\shadow_knight_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\Monster\shadow_pawn_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\Monster\shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\NPC\FemaleBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\NPC\FemaleBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\NPC\ManBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\NPC\ManBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\NPC\ManGloves01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\NPC\ManGloves02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\_arena-ref\bloodmuv4\BloodMu v4\Data\World3\World3.zip` | no_player_equipment |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\InGameShopScript\512.2011.006.rar` | unsupported_archive_format |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\Logo\Face01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\Logo\Face02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\Logo\Face03.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\Logo\Face04.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\Logo\face05.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\Monster\ex01shadow_master_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\Monster\ex01shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\Monster\shadow_knight_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\Monster\shadow_pawn_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\Monster\shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\NPC\FemaleBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\NPC\FemaleBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\NPC\ManBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\NPC\ManBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\NPC\ManGloves01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\tests\src\Release\Data\NPC\ManGloves02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\InGameShopScript\512.2011.006.rar` | unsupported_archive_format |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\Logo\Face01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\Logo\Face02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\Logo\Face03.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\Logo\Face04.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\Logo\face05.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\Monster\ex01shadow_master_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\Monster\ex01shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\Monster\shadow_knight_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\Monster\shadow_pawn_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\Monster\shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\NPC\FemaleBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\NPC\FemaleBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\NPC\ManBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\NPC\ManBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\NPC\ManGloves01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64\src\Release\Data\NPC\ManGloves02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\InGameShopScript\512.2011.006.rar` | unsupported_archive_format |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\Logo\Face01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\Logo\Face02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\Logo\Face03.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\Logo\Face04.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\Logo\face05.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\Monster\ex01shadow_master_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\Monster\ex01shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\Monster\shadow_knight_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\Monster\shadow_pawn_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\Monster\shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\NPC\FemaleBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\NPC\FemaleBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\NPC\ManBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\NPC\ManBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\NPC\ManGloves01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x64-asan\src\Release\Data\NPC\ManGloves02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\InGameShopScript\512.2011.006.rar` | unsupported_archive_format |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\Logo\Face01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\Logo\Face02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\Logo\Face03.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\Logo\Face04.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\Logo\face05.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\Monster\ex01shadow_master_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\Monster\ex01shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\Monster\shadow_knight_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\Monster\shadow_pawn_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\Monster\shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\NPC\FemaleBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\NPC\FemaleBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\NPC\ManBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\NPC\ManBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\NPC\ManGloves01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Debug\Data\NPC\ManGloves02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\InGameShopScript\512.2011.006.rar` | unsupported_archive_format |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\Logo\Face01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\Logo\Face02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\Logo\Face03.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\Logo\Face04.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\Logo\face05.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\Monster\ex01shadow_master_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\Monster\ex01shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\Monster\shadow_knight_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\Monster\shadow_pawn_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\Monster\shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\NPC\FemaleBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\NPC\FemaleBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\NPC\ManBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\NPC\ManBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\NPC\ManGloves01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86\src\Release\Data\NPC\ManGloves02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\out\build\windows-x86-asan\src\Release\Data\InGameShopScript\512.2011.006.rar` | unsupported_archive_format |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\InGameShopScript\512.2011.006.rar` | unsupported_archive_format |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\Logo\Face01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\Logo\Face02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\Logo\Face03.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\Logo\Face04.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\Logo\face05.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\Monster\ex01shadow_master_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\Monster\ex01shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\Monster\shadow_knight_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\Monster\shadow_pawn_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\Monster\shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\NPC\FemaleBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\NPC\FemaleBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\NPC\ManBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\NPC\ManBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\NPC\ManGloves01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\bin\Data\NPC\ManGloves02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\InGameShopScript\512.2011.006.rar` | unsupported_archive_format |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\Logo\Face01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\Logo\Face02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\Logo\Face03.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\Logo\Face04.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\Logo\face05.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\Monster\ex01shadow_master_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\Monster\ex01shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\Monster\shadow_knight_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\Monster\shadow_pawn_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\Monster\shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\NPC\FemaleBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\NPC\FemaleBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\NPC\ManBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\NPC\ManBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\NPC\ManGloves01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\MuMain\src\build\Release\Data\NPC\ManGloves02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-catalog-generated-recovery.zip` | no_player_equipment |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-golden-20260911\staged\golden-celestial-armor.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-golden-20260911\staged\golden-celestial-boots.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-golden-20260911\staged\golden-celestial-gloves.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-golden-20260911\staged\golden-celestial-helm.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-golden-20260911\staged\golden-celestial-pants.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-release-20260911\startup-diagnostics-patch.zip` | no_player_equipment |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-release-20260911\startup-fix-patch.zip` | no_player_equipment |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-release-20260911\test-patch-downloaded.zip` | no_player_equipment |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-state-mirror-before-20260911T0013.zip` | no_player_equipment |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-tooltip-mirror-before-20260911T0110.zip` | no_player_equipment |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-ultimate-20260911\server-changes.zip` | no_player_equipment |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-ultimate-20260911\ultimate-patch-downloaded.zip` | no_player_equipment |
| `C:\Users\joaop\Desenvolvimento\openmu\scratchpad\celestial-ultimate-20260911\verified\server-changes.zip` | no_player_equipment |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\InGameShopScript\512.2011.006.rar` | unsupported_archive_format |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\Logo\Face01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\Logo\Face02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\Logo\Face03.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\Logo\Face04.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\Logo\face05.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\Monster\ex01shadow_master_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\Monster\ex01shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\Monster\shadow_knight_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\Monster\shadow_pawn_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\Monster\shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\NPC\FemaleBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\NPC\FemaleBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\NPC\ManBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\NPC\ManBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\NPC\ManGloves01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game\Data\NPC\ManGloves02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\InGameShopScript\512.2011.006.rar` | unsupported_archive_format |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\Logo\Face01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\Logo\Face02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\Logo\Face03.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\Logo\Face04.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\Logo\face05.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\Monster\ex01shadow_master_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\Monster\ex01shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\Monster\shadow_knight_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\Monster\shadow_pawn_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\Monster\shadow_rock_7_helmet.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\NPC\FemaleBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\NPC\FemaleBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\NPC\ManBoots01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\NPC\ManBoots02.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\NPC\ManGloves01.bmd` | loose_model_outside_Data_Player |
| `C:\Users\joaop\Desenvolvimento\openmu\teste-launcher\Game2\Data\NPC\ManGloves02.bmd` | loose_model_outside_Data_Player |
