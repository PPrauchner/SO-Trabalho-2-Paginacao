#!/usr/bin/env bash
#
# Roda a grade de experimentos e grava um CSV por trace em results/.
#
# Responsabilidades:
# - Ler os valores da grade (traces, frames, pares N:I) de scripts/grid.conf.
# - Para cada trace presente, simular FIFO, OPT e o LRU aproximado com cada par N:I
#   em todos os números de frames, num CSV com um único cabeçalho.
# - Avisar sobre trace ausente e seguir com os demais.
#
# Uso: make grid   (ou: bash scripts/run_grid.sh, com build/sim já compilado)
# Variáveis de ambiente (para os testes): GRID_CONF, TRACES_DIR, RESULTS_DIR, SIM.

set -euo pipefail

GRID_CONF="${GRID_CONF:-scripts/grid.conf}"
TRACES_DIR="${TRACES_DIR:-traces}"
RESULTS_DIR="${RESULTS_DIR:-results}"
SIM="${SIM:-./build/sim}"

# shellcheck source=grid.conf
source "$GRID_CONF"

mkdir -p "$RESULTS_DIR"
missing=0

# Uma falha no meio de um trace aborta o script (set -e) antes do mv: o trap
# apaga o temporário em vez de deixá-lo órfão em results/.
tmp=""
trap 'rm -f "$tmp"' EXIT

for name in $TRACES; do
    trace="$TRACES_DIR/$name.trace"
    if [[ ! -f "$trace" ]]; then
        echo "aviso: trace ausente, pulando: $trace" >&2
        missing=$((missing + 1))
        continue
    fi

    csv="$RESULTS_DIR/$name.csv"
    echo "simulando $name..." >&2
    # Grava num temporário: uma falha no meio não deixa CSV pela metade.
    tmp="$csv.tmp"
    # shellcheck disable=SC2086  # FRAMES é lista separada por espaço
    {
        "$SIM" "$trace" fifo $FRAMES
        "$SIM" "$trace" opt $FRAMES | tail -n +2
        for pair in $LRU_PAIRS; do
            "$SIM" "$trace" lru-approx "${pair%%:*}" "${pair##*:}" $FRAMES | tail -n +2
        done
    } > "$tmp"
    mv "$tmp" "$csv"
done

if (( missing > 0 )); then
    echo "aviso: $missing trace(s) ausente(s) em $TRACES_DIR — veja o README para baixar" >&2
fi
