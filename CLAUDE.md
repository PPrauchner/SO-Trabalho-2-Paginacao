# CLAUDE.md — SO Trabalho 2: Substituição de Páginas

Trabalho 2 de Sistemas Operacionais (UNIPAMPA, 4º semestre). Simulador que mede
o número de falhas de página de três políticas de substituição — **FIFO**, **OPT**
e **LRU aproximado** (bit de referência + N bits de histórico/envelhecimento) — sobre
traces de acesso à memória, variando o número de frames (4, 8, 12, 16, 20, …).

Entrega: implementação + artigo no formato SBC, **30/09/2026** (apresentação no
mesmo dia, artigo impresso). Individual.

Enunciado: `docs/enunciado.md` e `docs/Trabalho2Bim.pdf` — ler antes de implementar.

## Stack

- **Simulador:** C++17, g++ + Makefile, compilado e executado no **WSL Ubuntu**.
  Emite resultados em CSV.
- **Gráficos:** Python 3 + matplotlib (`scripts/`), lê os CSV.
- **Artigo:** LaTeX, template SBC (`article/`).

## Trace

Uma linha por acesso: `<endereço hex 32 bits> <R|W>` (ex.: `31348900 W`).
Página de 4096 bytes → número da página = `endereço >> 12`.

## Estrutura

```
src/        simulador C++ (leitura de trace, políticas, main)
traces/     traces de entrada — NÃO versionados (baixar de
            https://www.inf.unioeste.br/~marcio/SO/trace/ — só HTTPS;
            HTTP cai no firewall): bzip, gcc, sixpack, swim (1M acessos
            cada) e bigone (4M)
results/    CSV gerados pelo simulador
scripts/    geração de gráficos (Python)
article/    artigo SBC (LaTeX)
docs/       enunciado, adr/, agents/
Slides/     aulas SO08–SO13 (referência teórica, NÃO versionadas)
```

## Comandos (no WSL)

```bash
make                     # compila → build/sim
make test                # testes (a definir)
./build/sim <args>       # CLI a definir
python3 scripts/plot.py  # gráficos a partir de results/
```

Artigo (Windows, MiKTeX — não há LaTeX no WSL):

```bash
cd article && latexmk -pdf sbc-template.tex
```

## Documentação

- `CONTEXT.md` — glossário de domínio (pt-BR ↔ identificadores em inglês).
- `docs/adr/` — decisões e porquês.
- `.claude/rules/` — restrições e calibragem deste projeto.

## Agent skills

### Issue tracker

Issues no GitHub (`PPrauchner/SO-Trabalho-2-Paginacao`, via `gh`). See `docs/agents/issue-tracker.md`.

### Triage labels

Vocabulário padrão (needs-triage, needs-info, ready-for-agent, ready-for-human, wontfix). See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: `CONTEXT.md` + `docs/adr/` na raiz. See `docs/agents/domain.md`.
