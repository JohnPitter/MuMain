# Catálogo local da Loja de Itens

O cliente Luxview usa o catálogo que acompanha a distribuição e não tenta mais baixar arquivos do serviço legado da Webzen. Isso evita bloqueios e o erro `WAIT_TIMEOUT` ao abrir a Loja de Itens.

## Publicação

A distribuição do cliente deve conter os três arquivos da versão informada pelo servidor em:

```text
Data/InGameShopScript/000.2026.001/IBSCategory.txt
Data/InGameShopScript/000.2026.001/IBSPackage.txt
Data/InGameShopScript/000.2026.001/IBSProduct.txt
```

O CMake copia automaticamente todo o conteúdo de `src/bin` para o diretório do executável. Ao montar ZIPs ou patches manualmente, preserve o mesmo caminho relativo e a mesma capitalização.

Se algum arquivo estiver ausente, a loja falha imediatamente com uma mensagem que informa o diretório esperado, em vez de esperar uma conexão com a Webzen expirar.
