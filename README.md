# Simulador de Substituição de Páginas

Trabalho 2 de Sistemas Operacionais (UNIPAMPA). Mede o número de falhas de página
das políticas **FIFO**, **OPT** e **LRU aproximado** (bit de referência + N bits de
histórico, envelhecidos a cada I acessos) sobre traces reais de acesso à memória,
variando o número de frames.

## Pré-requisitos

- **WSL** com Ubuntu (ou qualquer Linux).
- **g++** com suporte a C++17 e **make**: `sudo apt install g++ make`.

Nenhuma outra dependência: o simulador usa só a biblioteca padrão.

> No Windows, se a distribuição padrão do WSL não for o Ubuntu (por exemplo, com o
> Docker Desktop instalado), entre nela explicitamente: `wsl -d Ubuntu`.

## Traces

Os traces não estão no repositório (são grandes). Baixe-os para `traces/`:

```bash
mkdir -p traces
for t in bzip gcc sixpack swim bigone; do
    wget -P traces "https://www.inf.unioeste.br/~marcio/SO/trace/$t.trace"
done
```

Use **HTTPS** — o endereço por HTTP é bloqueado por firewall. Cada linha do trace é
`<endereço hex de 32 bits> <R|W>`; a página é o endereço sem os 12 bits de
deslocamento (páginas de 4096 bytes).

## Compilar e testar

```bash
make        # compila → build/sim
make test   # compila e roda os testes
```

## Reproduzir os resultados

```bash
make grid
```

Compila se necessário e roda a grade completa: todos os traces × FIFO, OPT e LRU
aproximado × todos os números de frames × todos os pares N/I. Grava um CSV por
trace em `results/<trace>.csv`. Um trace ausente em `traces/` gera um aviso e é
pulado, sem interromper os demais. A simulação é determinística: rodar a grade de
novo produz CSV idênticos aos versionados.

Os valores da grade (traces, números de frames e pares N:I) ficam **só** em
[`scripts/grid.conf`](scripts/grid.conf).

### Formato do CSV

```
trace,policy,frames,history_bits,aging_interval,accesses,page_faults
gcc,fifo,4,,,1000000,302860
gcc,lru-approx,4,8,100,1000000,409047
```

- `policy`: `fifo`, `opt` ou `lru-approx`.
- `history_bits` (N) e `aging_interval` (I): só preenchidos no LRU aproximado.
- `page_faults`: falhas de página, incluindo as compulsórias.

## Simulação avulsa

```bash
./build/sim <trace> fifo|opt <frames>...
./build/sim <trace> lru-approx <N> <I> <frames>...
```

Exemplo: `./build/sim traces/gcc.trace lru-approx 8 1000 4 8 16`.
