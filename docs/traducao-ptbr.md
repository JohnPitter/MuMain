# Tradução PT-BR

O cliente usa PT-BR (`pt`) como idioma padrão. A fonte das mensagens fica em
`src/Localization/Game.pt.resx`; os arquivos C++ em `out/.../Generated/I18N`
são gerados e não devem ser editados manualmente.

## Diretrizes

- Use linguagem natural e curta para menus e botões: `Grupo` (Party), `Missão`
  (Quest) e `Sair` (Exit).
- Preserve nomes próprios e nomes de mapas do MU: Lorencia, Devias, Noria,
  Atlans, Icarus, Devil Square e Chaos Castle.
- Preserve termos reconhecidos pelos jogadores: Guilda, Nível Master, Reset,
  Zen, HP, MP, AG, SD e comandos como `/guild`, `/party` e `/warp`.
- Joias seguem a nomenclatura do MU BR: Bless, Soul, Chaos, Creation e Life.
- Nunca altere placeholders (`%d`, `%s`, `%lu`, etc.) nem tags de comando.

## Regenerar e compilar

A geração ocorre automaticamente pelo CMake a partir de todos os `.resx`, por
meio do `tools/ResxGen`, antes da compilação do alvo `Main`. No checkout do
Windows, configure/compile com o preset x86 Release:

```powershell
VsDevCmd -arch=x86
cmake --build --preset windows-x86-release
```

Após editar uma tradução, confira o arquivo gerado em
`out/build/windows-x86/Generated/I18N/Game.cpp` e valide o cliente Release.
O locale também pode ser alterado no `config.ini`, em `[UI] Locale=pt`.
