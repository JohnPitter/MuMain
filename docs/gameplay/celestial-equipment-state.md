# Celestial Ultimate: fases, requisitos e consulta

## Revisão Ultimate

Celestial é uma categoria Ultimate exclusiva, exibida com título dourado. Os
itens existentes conservam Excellent, Luck, Harmony, adicionais e sockets.
Somente Grand Master, nível400, pode usá-los. Os requisitos recebidos do servidor
substituem os incrementos legados de nível Excellent e +0–+15 apenas nestas peças.

Os bônus do conjunto são cumulativos, mas não se somam duas vezes:

| Equipamento utilizável | Parcela dos bônus completos |
|---|---:|
| Elmo, armadura, calça, luvas e botas | 30% |
| As cinco peças + cajado e escudo | 60% |
| Os sete itens + asas, pendant e dois anéis | 100% |

Por exemplo, dano+15% do conjunto completo passa a+4,5% na primeira fase e+9%
na segunda. Atributos individuais e opções de cada item são independentes.
Retirar/quebrar uma peça reduz o conjunto à maior fase ainda válida. Asas e
joias sozinhas não pulam as fases anteriores. O servidor continua autoritativo.

O Shift mostra o percentual confirmado, as três fases e todos os valores como
**ativo / total**. O cliente aceita E7/E8v1 anteriores e a versãov2 com metadados
Ultimate, requisitos, fases e percentual ativo. Sem estado válido, não presume
bônus pela aparência nem pelos itens do baú. O catálogo é limpo ao trocar de sessão.

O brilho próprio agora existe no+0, sem exigir flags Excellent: reflexo dourado
mais forte, marfim moderado e emissão controlada nas gemas/filamentos. Efeitos
respeitam distância, nível gráfico, transparência e Cloaking; não alteram combate.
As botas receberam panturrilha, tornozelo, calcanhar e sola detalhados, preservando
o esqueleto e as sequências de animação nativas. Prévia de Blender não comprova
o brilho do renderizador MU; a revisão requer inspeção pessoal ingame.

Para os testes cruzados v2, usar os arquivos reais.NET
`celestial-equipment-state-v2.bin` e `celestial-equipment-catalog-v2.bin` nas
variáveis `CELESTIAL_PACKET_FIXTURE`/`CELESTIAL_CATALOG_FIXTURE`. Testar também
`test_celestial_shimmer`. O teste de limpeza de personagens não chama WinMain.

## Comportamento da versão anterior (v1)

As onze peças usam elegibilidade calculada pelo servidor. A aura não depende
de upgrade +10 nem apenas das cinco peças de armadura. Uma peça ausente ou
quebrada desativa o bônus; clientes anteriores sem este contrato não recebem
a nova indicação. O efeito respeita a configuração de efeitos visuais e cloaking.

O servidor estendido envia `C1 F3 E8`, versão 1, depois de criar o personagem e
ao alterar seu estado. O contrato completo está no OpenMU, documento
`docs/celestial-full-set-bonuses.md`. O fator de movimento recebido já contém
equipamento e lentidão: não aplicar freeze/slow novamente. Velocidades de
ataque/magia recebidas também atualizam personagens observados.

Sem uma mensagem válida, o movimento continua legado e a aura completa fica
desativada. Remoção/reutilização do personagem limpa o estado recebido.
Os brilhos próprios de asas, cajado e demais peças são independentes do bônus.

## Consultar o conjunto completo

Passe o mouse sobre qualquer peça Celestial no inventário, equipamento, loja
ou baú. O tooltip normal mantém os atributos e opções do item; quando há espaço,
acrescenta o estado do **seu conjunto equipado** e a dica de consulta.
Segure **Shift** para ver todos os bônus do conjunto. Solte para voltar ao item.
Tooltips de itens vinculados ao chat mantêm sua apresentação nativa.

O nome e as unidades são traduzidos em português/inglês; os valores, a quantidade
de peças e o upgrade exigido são recebidos do servidor. Chance crítica/ignorar
defesa usam pontos percentuais, enquanto dano/vida/velocidade multiplicativos
usam porcentagem. Recuperação não significa cura extra de poções ou heals.
O servidor ainda aplica os limites de atributos, independentemente do tooltip.

O catálogo recebido não ativa o bônus. Ativo/inativo/aguardando refletem apenas
o estado confirmado do próprio jogador, inclusive ao consultar uma peça que
está no baú. Uma peça quebrada ou ausente impede o conjunto completo. Dois
anéis são duas instâncias exigidas, não dois tipos diferentes de anel.

Em canais sem catálogo válido, os itens mantêm o tooltip normal. Troca de
personagem limpa o catálogo anterior. Depois de uma alteração administrativa
de configuração, reconectar para renovar a descrição. A lista detalhada é
separada para não ocupar as cinquenta linhas já usadas pelas opções nativas.

## Verificações reproduzíveis

Compilar Main e os alvos `test_equipment_state`, `test_equipment_catalog` e
`test_celestial_model_contract` com o ambiente Windows x64. Para conferir o
emissor real contra o decoder, executar primeiro `EquipmentStatePacketTests`
no OpenMU (`--no-restore -p:ci=true`) e definir `CELESTIAL_PACKET_FIXTURE` para
o arquivo `celestial-equipment-state-v1.bin` produzido no diretório dos testes.
Sem a variável, o teste informa que essa conferência não foi executada.

Executar também `EquipmentBonusCatalogTests` no OpenMU e definir
`CELESTIAL_CATALOG_FIXTURE` para `celestial-equipment-catalog-v1.bin`.
Os nove testes novos verificam limites de mensagens/linhas, canais vazios,
troca de catálogo, valores/unidades e o resultado do emissor real. A seleção
atual totaliza 20 testes C++: 7 estado/movimento, 9 catálogo/tooltip e 4 modelos.

Testes locais não comprovam renderização nem comunicação numa sessão de jogo.
Antes de publicar, conferir Soul Master e Grand Master, +0/+15, onze peças,
remoção de cada peça e quebra/reparo. Repetir com outro jogador observando,
entrada/saída de escopo, relog, transformação, freeze/slow, montaria e safezone.
Comparar a velocidade e o estado com o servidor; verificar asas, cajado, escudo,
armadura, rosto e ícones junto às referências originais.
Conferir Shift em PT/EN com +0/+15, Excellent/socket, mochila, baú, loja e
bordas da tela. Comparar o texto antes/depois de retirar/quebrar/reparar peças,
reconectar e alternar de canal. Esses cenários de interface ainda exigem jogo real.

O candidato contém Main e os 17 assets Celestial. Não contém o catálogo Item.bmd,
base completa nem DLL de conexão. Preservar a DLL publicada e as melhorias não
relacionadas ao montar a distribuição final. Nenhuma publicação faz parte
desta validação local. A fidelidade ao conceito e o desempenho seguem pendentes.

## Diagnóstico de fechamento antes da seleção

Se o cliente de testes fechar logo após o carregamento, preserve `MuError.log`
da pasta desse cliente. As linhas `[Startup]` registram a última etapa da entrada;
no Windows/MSVC uma falha nativa durante essa transição registra também módulo,
código e deslocamento para análise com os símbolos do mesmo executável.
O diagnóstico não grava senhas, pacotes, dumps de memória nem altera os itens.
Não impede o fechamento nem representa correção da causa. A confirmação de
inicialização e aparência continua dependendo do teste pessoal no jogo.
