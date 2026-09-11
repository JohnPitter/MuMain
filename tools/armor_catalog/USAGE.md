# Catálogo diagnóstico dos equipamentos

O levantamento não altera modelos, texturas, ZIPs, cliente ou servidor. Ele lê as bases locais e grava somente os relatórios e miniaturas no diretório de saída. Requer Python 3.12+, Pillow e `rg` disponíveis; reutiliza `art-source/celestial/inspect_bmd_rig.py` sem modificá-lo.

## Atualizar o catálogo

```powershell
python tools/armor_catalog/build_catalog.py `
  --canonical 'C:/Users/joaop/Desenvolvimento/openmu/scratchpad/celestial-ultimate-20260911/ultimate-base-downloaded.zip' `
  --client 'C:/_wt-celestial-client' `
  --server 'C:/_wt-celestial-server' `
  --roots 'C:/Users/joaop/Desenvolvimento/openmu' 'C:/_wt-celestial-client' `
  --output 'C:/_wt-celestial-client/docs/art/armor-reference'
```

Para exportar apenas as famílias e nomes antes da leitura completa, execute `catalog_names.py` com os mesmos argumentos, exceto `--roots`.

## Validar

```powershell
python -m unittest discover -s tools/armor_catalog -p test_catalog.py -v
python tools/armor_catalog/verify_catalog.py --output docs/art/armor-reference
```

`catalog-validation.json` registra conferência das referências internas, miniaturas, hash do ZIP canônico e preservação do parser compartilhado. Os testes de mapeamento usam as definições em `C:/_wt-celestial-server`.

## Consultar

- `catalog.md`: cobertura geral, famílias e origens.
- `armor-details.md`: cada peça, malha, ossos, quadros de animação e atlas.
- `equipment-details.md`: armas, escudos, asas, acessórios e recursos auxiliares.
- `coverage.md`: diferenças entre bases, texturas externas e limites da varredura.
- `catalog.json`: ocorrências separadas por origem; `mapping_ref` aponta para `mappings`, `sha256` para `models`; texturas são deduplicadas separadamente.
- `item-names.json`: nomes declarados no servidor e nas quatro localizações do cliente, com bytes de origem preservados.

As pastas `node_modules`, `.git`, `ThirdParty` e `.nuget` ficam fora da descoberta. Arquivos RAR/7z são listados como não inspecionados; ZIPs sem equipamentos são registrados. Modelos de NPC/cenário fora de `Data/Player` e `Data/Item` são listados à parte quando o nome parece uma peça de armadura. Na árvore `client-assets`, só foi encontrado `Data/Local/ServerList.bmd`, que não é equipamento.

O mapeamento de arquivos acompanha os carregadores do cliente desta revisão; a atualização de regras do carregador exige revisar os testes correspondentes. Nomes do ZIP canônico não são usados para renomear malhas antigas: cada variante mantém nome embutido, caminho de origem e hash próprios. A interpretação de estilo visual exige as pranchas 3D e não deve ser deduzida apenas pelo nome do item.

Os hashes e campos medidos são reproduzíveis para as mesmas entradas. Os relatórios não validam login, disponibilidade para jogadores, animação dentro do cliente ou configuração persistida da VPS.
