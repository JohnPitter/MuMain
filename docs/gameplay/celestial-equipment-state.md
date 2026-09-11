# Celestial: estado completo e movimento (candidato local)

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

## Verificações reproduzíveis

Compilar Main e os alvos `test_equipment_state` e
`test_celestial_model_contract` com o ambiente Windows x64. Para conferir o
emissor real contra o decoder, executar primeiro `EquipmentStatePacketTests`
no OpenMU (`--no-restore -p:ci=true`) e definir `CELESTIAL_PACKET_FIXTURE` para
o arquivo `celestial-equipment-state-v1.bin` produzido no diretório dos testes.
Sem a variável, o teste informa que essa conferência não foi executada.

Testes locais não comprovam renderização nem comunicação numa sessão de jogo.
Antes de publicar, conferir Soul Master e Grand Master, +0/+15, onze peças,
remoção de cada peça e quebra/reparo. Repetir com outro jogador observando,
entrada/saída de escopo, relog, transformação, freeze/slow, montaria e safezone.
Comparar a velocidade e o estado com o servidor; verificar asas, cajado, escudo,
armadura, rosto e ícones junto às referências originais.

O candidato contém Main e os 17 assets Celestial. Não contém o catálogo Item.bmd,
base completa nem DLL de conexão. Preservar a DLL publicada e as melhorias não
relacionadas ao montar a distribuição final. Nenhuma publicação faz parte
desta validação local. A fidelidade ao conceito e o desempenho seguem pendentes.
