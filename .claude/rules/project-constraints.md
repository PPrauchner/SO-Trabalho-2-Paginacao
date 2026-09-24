# Restrições deste projeto

> **Semente.** O ARK entrega este arquivo uma vez, vazio, para o projeto preencher —
> e nunca mais o toca. Quem preenche é a skill `adopt-repo`, na sessão de *grill with
> docs* (log em `docs/grills_logs/`).
>
> O genérico do ARK (idioma, clean code) não mora aqui: vem do clone, importado pela
> Instalação global. Este arquivo é só o que vale **neste** repositório.
>
> Modelo de domínio em [`CONTEXT.md`](../../CONTEXT.md); decisões e porquês em
> [`docs/adr/`](../../docs/adr/).

---

O que entra aqui é o que **não pode mudar** e não se descobre lendo o código:
linguagem e versão, dependências centrais vs. opcionais, tipo de interface
(CLI/web/API), formato de persistência, exigência de reprodutibilidade, limites de
ambiente, estrutura de pastas — o que for específico e não-óbvio deste projeto.

Dependência que é decisão, e não acaso, entra. Acaso, não.

## Stack

- **Simulador em C++17 puro, sem dependências externas** (só a STL). Compila com
  `g++ -std=c++17 -O2` via Makefile no **WSL Ubuntu** — o professor precisa conseguir
  compilar sem instalar nada além do g++.
- **Python só para gráficos** (matplotlib), lendo os CSV. Nenhuma lógica de simulação
  em Python.

## Interface e saída

- **CLI**: trace, política, número de frames, N e I entram por argumento.
- **Saída em CSV** em `results/`, uma linha por simulação.

## Semântica da simulação (não negociável)

- **Determinismo:** mesma entrada → mesmo número de falhas. Sem aleatoriedade;
  desempates conforme o `CONTEXT.md` (bit de referência desligado, depois FIFO).
- **Fidelidade ao enunciado:** página de 4096 bytes e endereços de 32 bits, fixos.
  Leitura e escrita contam igual. Falhas compulsórias entram na contagem.
- **Só o OPT enxerga o futuro** do trace. FIFO e LRU aproximado não podem usar
  informação futura.

## Repositório

- Traces (`traces/`) e slides (`Slides/`) ficam **fora do git**.

## Prazo

- Entrega e apresentação em **30/09/2026**. Na dúvida entre elegância e entregar,
  entregar.
