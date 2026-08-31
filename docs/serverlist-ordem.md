# Ordem dos canais na seleção de servidores

A lista de grupos é exibida pela ordem crescente da coluna `sequence` do
`ServerList.bmd`, não pela ordem física dos registros nem pelo `ServerID`.

O cliente carrega cada grupo em `CServerListManager::InsertServerGroup`, usando
`m_bySequence` como chave de `m_mapServerGroup`. `SetFirst`/`GetNext` percorre
essa `std::map`, e `CServerSelWin::UpdateDisplay` cria os botões nessa ordem.
O índice do grupo (`ServerID / MAX_SERVER_PER_GROUP`) apenas associa os canais
ao grupo correto.

## Configuração atual

| Canal | índice BMD | ServerID | `sequence` |
| --- | ---: | ---: | ---: |
| MuRaiz 99d Hard | 1 | 20 | 1 |
| MuRaiz S2 Medium | 2 | 40 | 2 |
| MuRaiz S6 Easy | 0 | 0 | 3 |

Os nomes e os `ServerID`s do banco não foram alterados. Portanto, não é
necessário reiniciar o servidor; basta reabrir o cliente para recarregar o BMD.

## Validação

No checkout do projeto, valide os registros e nomes com:

```powershell
.\MuMain\tools\edit-serverlist.ps1 `
  -Path .\MuMain\src\bin\Data\Local\ServerList.bmd -Verify
```

A saída deve listar os índices `0`, `1`, `2` com os nomes esperados. Para
confirmar a ordem, decodifique a coluna `sequence` no BuxConvert: os valores
devem ser `index 1 = 1`, `index 2 = 2` e `index 0 = 3`. Na tela de seleção, a
ordem esperada é **99d Hard**, **S2 Medium**, **S6 Easy**.

O arquivo anterior foi preservado em
`backups/ServerList.bmd.pre-order-20250214`.
