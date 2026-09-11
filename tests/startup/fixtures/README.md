# Fixture do teste de limpeza de personagens

`Mix.bmd` é uma cópia do arquivo nativo Season 6 usado no cliente local de
Testes. SHA256: `fa3f7a4bd3b35846812492864c0d36ce4753cc9583d25c1a09d6dcd363629894`.

O gerenciador legado de receitas abre esse arquivo em um construtor global,
antes de `main`. Sem ele, a descoberta de testes abre um diálogo e pode sair
com código zero sem executar nenhuma verificação. O CMake prepara a fixture
somente no diretório de trabalho do teste; nenhuma instalação do jogo muda.

O executável usa `MuClient` sem `WHOLE_ARCHIVE` e as funções reais de criação
e limpeza de personagens. Não chama `WinMain`, não abre o jogo e não se conecta
ao servidor. A checagem pós-build exige que as asserções realmente executem,
rejeitando uma saída zero vazia causada por inicializadores legados.
