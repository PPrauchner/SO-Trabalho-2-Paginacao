"""
Gera os gráficos do artigo a partir dos CSV de resultados.

Uso: `python scripts/plot.py [pasta_de_resultados] [pasta_de_saída]` — por padrão lê
`results/` e grava os PDF em `article/figuras/`.

Responsabilidades:
- Ler os CSV de `results/` gravados pelo simulador.
- Selecionar as curvas de cada gráfico (sem nenhuma lógica de simulação).
- Gravar, por trace, os gráficos de falhas × frames e de sensibilidade a N e a I, em PDF.
"""

from __future__ import annotations

import argparse
import csv
import sys
from dataclasses import dataclass
from pathlib import Path

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.ticker import FuncFormatter, MaxNLocator  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_RESULTS_DIR = REPO_ROOT / "results"
DEFAULT_OUTPUT_DIR = REPO_ROOT / "article" / "figuras"

# Par N/I de referência do LRU aproximado — único lugar a editar.
REFERENCE_HISTORY_BITS = 8
REFERENCE_AGING_INTERVAL = 1000

POLICY_LABELS = {"fifo": "FIFO", "opt": "OPT"}

Curve = tuple[list[int], list[int]]


@dataclass(frozen=True)
class Row:
    """Uma simulação: uma linha de um CSV de resultados.

    Attributes:
        trace: Nome do trace.
        policy: `fifo`, `opt` ou `lru-approx`.
        frames: Número de frames.
        history_bits: N do LRU aproximado (None nas demais políticas).
        aging_interval: I do LRU aproximado (None nas demais políticas).
        page_faults: Falhas de página.
    """

    trace: str
    policy: str
    frames: int
    history_bits: int | None
    aging_interval: int | None
    page_faults: int


def _optional_int(text: str) -> int | None:
    return int(text) if text else None


def load_results(results_dir: Path) -> list[Row]:
    """Lê todos os CSV de uma pasta de resultados.

    Args:
        results_dir: Pasta com os `<trace>.csv`.

    Returns:
        Todas as simulações, na ordem dos arquivos.
    """
    rows: list[Row] = []
    for path in sorted(results_dir.glob("*.csv")):
        with path.open(newline="") as f:
            for record in csv.DictReader(f):
                rows.append(Row(
                    trace=record["trace"],
                    policy=record["policy"],
                    frames=int(record["frames"]),
                    history_bits=_optional_int(record["history_bits"]),
                    aging_interval=_optional_int(record["aging_interval"]),
                    page_faults=int(record["page_faults"]),
                ))
    return rows


def _curve(rows: list[Row]) -> Curve:
    ordered = sorted(rows, key=lambda r: r.frames)
    return [r.frames for r in ordered], [r.page_faults for r in ordered]


def lru_label(history_bits: int, aging_interval: int) -> str:
    """Rótulo de legenda de um par N/I do LRU aproximado.

    Args:
        history_bits: N (bits de histórico).
        aging_interval: I (intervalo de envelhecimento, em acessos).

    Returns:
        O rótulo, por exemplo `LRU aproximado (N=8, I=1000)`.
    """
    return f"LRU aproximado (N={history_bits}, I={aging_interval})"


def policy_curves(rows: list[Row], trace: str) -> dict[str, Curve]:
    """Curvas falhas × frames de cada política num trace.

    O LRU aproximado entra só com o par N/I de referência.

    Args:
        rows: Simulações lidas dos CSV.
        trace: Trace a selecionar.

    Returns:
        Rótulo da política → (frames, falhas), em ordem crescente de frames.
    """
    of_trace = [r for r in rows if r.trace == trace]
    curves: dict[str, Curve] = {}
    for policy, label in POLICY_LABELS.items():
        selected = [r for r in of_trace if r.policy == policy]
        if selected:
            curves[label] = _curve(selected)
    reference = [
        r for r in of_trace
        if r.policy == "lru-approx"
        and r.history_bits == REFERENCE_HISTORY_BITS
        and r.aging_interval == REFERENCE_AGING_INTERVAL
    ]
    if reference:
        curves[lru_label(REFERENCE_HISTORY_BITS, REFERENCE_AGING_INTERVAL)] = _curve(reference)
    return curves


# Parâmetro variado → (parâmetro fixo, valor de referência do fixo, símbolo na legenda).
_SENSITIVITY = {
    "history_bits": ("aging_interval", REFERENCE_AGING_INTERVAL, "N"),
    "aging_interval": ("history_bits", REFERENCE_HISTORY_BITS, "I"),
}


def sensitivity_curves(rows: list[Row], trace: str, vary: str) -> dict[str, Curve]:
    """Curvas do LRU aproximado variando N ou I, com o outro no valor de referência.

    Os valores variados vêm do próprio CSV, não de uma lista fixa.

    Args:
        rows: Simulações lidas dos CSV.
        trace: Trace a selecionar.
        vary: `history_bits` (varia N) ou `aging_interval` (varia I).

    Returns:
        Rótulo (`N=4`, `I=100`, …) → (frames, falhas), em ordem crescente do valor variado.
    """
    fixed, fixed_value, symbol = _SENSITIVITY[vary]
    selected = [
        r for r in rows
        if r.trace == trace and r.policy == "lru-approx" and getattr(r, fixed) == fixed_value
    ]
    values = sorted({getattr(r, vary) for r in selected})
    return {
        f"{symbol}={value}": _curve([r for r in selected if getattr(r, vary) == value])
        for value in values
    }


# Marcador e traço por curva: o artigo é impresso, e a cor sozinha some em tons de cinza.
_STYLES = [("o", "-"), ("s", "--"), ("^", "-."), ("D", ":"), ("v", "-")]


def _thousands(value: float, _position: int) -> str:
    """Formata um tick do eixo com separador de milhar em pt-BR (1.234.567)."""
    return f"{value:,.0f}".replace(",", ".")


def _save_figure(curves: dict[str, Curve], title: str, path: Path) -> None:
    """Desenha um gráfico falhas × frames com uma curva por rótulo e grava em PDF.

    Args:
        curves: Rótulo da legenda → (frames, falhas).
        title: Título do gráfico.
        path: Arquivo PDF de saída.
    """
    fig, ax = plt.subplots(figsize=(6.0, 3.6))
    for index, (label, (frames, faults)) in enumerate(curves.items()):
        marker, linestyle = _STYLES[index % len(_STYLES)]
        ax.plot(frames, faults, marker=marker, linestyle=linestyle,
                markersize=4, linewidth=1.2, label=label)
    ax.set_title(title)
    ax.set_xlabel("Número de frames")
    ax.set_ylabel("Falhas de página")
    ax.xaxis.set_major_locator(MaxNLocator(integer=True))
    ax.yaxis.set_major_formatter(FuncFormatter(_thousands))
    ax.grid(True, linewidth=0.4, alpha=0.5)
    ax.legend()
    fig.tight_layout()
    # Sem data de criação: rodar de novo sobre os mesmos CSV gera o mesmo PDF.
    fig.savefig(path, format="pdf", metadata={"CreationDate": None})
    plt.close(fig)


def generate_all(results_dir: Path, output_dir: Path) -> list[Path]:
    """Gera todos os gráficos do artigo a partir dos CSV de resultados.

    Por trace: `falhas_<trace>.pdf` (FIFO, OPT e LRU aproximado de referência),
    `sensibilidade_n_<trace>.pdf` (varia N, I de referência) e
    `sensibilidade_i_<trace>.pdf` (varia I, N de referência).

    Args:
        results_dir: Pasta com os `<trace>.csv`.
        output_dir: Pasta dos PDF; criada se não existir.

    Returns:
        Os arquivos gravados.

    Raises:
        SystemExit: Se não houver nenhum resultado em `results_dir`.
    """
    rows = load_results(results_dir)
    if not rows:
        raise SystemExit(f"nenhum resultado em {results_dir} — rode `make grid` antes")
    output_dir.mkdir(parents=True, exist_ok=True)
    reference = f"N={REFERENCE_HISTORY_BITS}, I={REFERENCE_AGING_INTERVAL}"
    written: list[Path] = []
    for trace in sorted({r.trace for r in rows}):
        figures = [
            ("falhas", policy_curves(rows, trace), f"{trace}: falhas de página por política"),
            ("sensibilidade_n", sensitivity_curves(rows, trace, vary="history_bits"),
             f"{trace}: LRU aproximado variando N (I={REFERENCE_AGING_INTERVAL})"),
            ("sensibilidade_i", sensitivity_curves(rows, trace, vary="aging_interval"),
             f"{trace}: LRU aproximado variando I (N={REFERENCE_HISTORY_BITS})"),
        ]
        for prefix, curves, title in figures:
            if not curves:
                print(f"aviso: {trace} sem dados para {prefix} ({reference})", file=sys.stderr)
                continue
            path = output_dir / f"{prefix}_{trace}.pdf"
            _save_figure(curves, title, path)
            written.append(path)
    return written


def main() -> None:
    """Ponto de entrada da linha de comando."""
    parser = argparse.ArgumentParser(description="Gera os gráficos do artigo a partir dos CSV.")
    parser.add_argument("results_dir", nargs="?", type=Path, default=DEFAULT_RESULTS_DIR)
    parser.add_argument("output_dir", nargs="?", type=Path, default=DEFAULT_OUTPUT_DIR)
    args = parser.parse_args()
    for path in generate_all(args.results_dir, args.output_dir):
        print(path)


if __name__ == "__main__":
    main()
