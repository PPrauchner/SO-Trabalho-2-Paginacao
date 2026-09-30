# 0001 — Parâmetros da grade de experimentos: N=8, I=100, frames 4..64

- **Status:** aceita (30/09/2026, issue #8)
- **Onde vale:** `scripts/grid.conf` (grade) e `scripts/plot.py` (par de referência)

## Contexto

O enunciado deixa a critério do aluno os **bits de histórico** (N) e o **intervalo de
envelhecimento** (I) do LRU aproximado, e os números de frames avaliados. A grade
inicial usava 8:1000 como provisório. A escolha definitiva saiu de uma rodada
exploratória sobre os quatro traces independentes (bzip, gcc, sixpack, swim).

### Varredura

N ∈ {1, 2, 4, 8, 16, 32} × I ∈ {10, 25, 50, 100, 200, 500, 1000, 10000, 100000},
frames de 4 a 64 (passo 4) e mais 80, 96, 128, 192 e 256.

Métrica: posição média do LRU aproximado entre o OPT (0) e o FIFO (1) nos frames de
4 a 64, tirando a média entre os quatro traces. Quanto menor, mais perto do ótimo.

| N \ I | 10    | 25    | 50    | 100       | 200   | 500   | 1000  | 10000 | 100000 |
|-------|-------|-------|-------|-----------|-------|-------|-------|-------|--------|
| 1     | 0,911 |       |       | 0,789     |       |       | 0,874 | 1,062 | 1,583  |
| 2     | 0,883 |       |       | 0,715     |       |       | 0,784 | 0,994 | 1,584  |
| 4     | 0,846 | 0,759 | 0,707 | 0,671     | 0,660 | 0,700 | 0,773 | 1,000 | 1,580  |
| 8     | 0,774 | 0,704 | 0,663 | **0,638** | 0,640 | 0,694 | 0,769 | 1,000 | 1,579  |
| 16    | 0,719 | 0,660 | 0,632 | 0,621     | 0,632 | 0,692 | 0,767 | 1,001 | 1,579  |
| 32    | 0,673 |       |       | 0,612     |       |       | 0,765 | 1,000 | 1,579  |

## Decisão

- **I = 100 acessos.** É o melhor intervalo em todos os traces (I=200 empata, com
  diferença de 0,002). Um intervalo curto demais (10) apaga a diferença entre páginas
  usadas há pouco e há muito tempo. Um intervalo longo demais (≥ 10000) deixa o bit de
  referência ligado em quase todas as páginas, e aí o LRU aproximado vira FIFO
  (posição 1,0) ou fica pior que ele (1,58 com I=100000).
- **N = 8 bits.** É onde o ganho de N satura. De 8 para 16 bits melhora só 0,017, e
  de 8 para 32 melhora 0,026, pagando dois a quatro bytes por página. Oito bits cabem
  num byte, o que é plausível num hardware ou SO real.
  Com I=1000 o N já satura em 4: a janela útil de memória é da ordem de N·I acessos.
- **O par provisório 8:1000 tinha um defeito:** no bzip com 12 frames ele fazia 8303
  falhas, contra 4581 do FIFO. Com 8:100 são 4023.
- **Frames: 4 a 64, passo 4 (16 pontos).** A faixa cobre o joelho do bzip (317
  páginas distintas, joelho entre 8 e 12 frames) e a região em que as políticas mais
  diferem. Os demais traces (2543 a 3890 páginas distintas) continuam caindo até 256
  frames, mas a razão LRU/FIFO se mantém estável depois de 64. Estender a faixa não
  muda a conclusão.
- **Sensibilidade:** a grade varia I ∈ {10, 100, 1000, 10000, 100000} com N=8 e
  N ∈ {1, 2, 4, 8, 16} com I=100. São cinco curvas por gráfico, uma por estilo de
  marcador/traço do `plot.py`.

## bigone

O `bigone` (4M acessos) é **exatamente** a concatenação `bzip + gcc + sixpack + swim`,
nessa ordem: cada bloco de 1M linhas tem o mesmo md5 do trace correspondente.
**Fica na grade**, como carga com trocas de fase (quatro programas em sequência). O
artigo deve declarar que ele é a concatenação e não tratá-lo como um quinto programa
independente.

## Consequências

- Os resultados e as figuras de `results/` e `article/figuras/` foram regerados com
  esta grade.
- Mudar o par de referência exige editar `grid.conf` **e** `plot.py`, porque os dois
  precisam concordar.
