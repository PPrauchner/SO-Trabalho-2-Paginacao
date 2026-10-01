"""
Testes do gerador de gráficos (`plot.py`).

Responsabilidades:
- Verificar a leitura dos CSV de resultados e a seleção das curvas.
- Verificar que o comando gera os PDF esperados na pasta de saída.
"""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path

import plot

HEADER = "trace,policy,frames,history_bits,aging_interval,accesses,page_faults\n"


def write_csv(directory: Path, trace: str, lines: list[str]) -> None:
    """Grava um CSV de resultados com o cabeçalho do simulador.

    Args:
        directory: Pasta onde o CSV é gravado.
        trace: Nome do trace (vira `<trace>.csv`).
        lines: Linhas de dados, sem o cabeçalho.
    """
    (directory / f"{trace}.csv").write_text(HEADER + "".join(f"{line}\n" for line in lines))


class PolicyCurvesTest(unittest.TestCase):
    """Curvas falhas × frames por política (gráfico a)."""

    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def test_one_curve_per_policy_with_reference_lru_pair(self) -> None:
        write_csv(self.dir, "gcc", [
            "gcc,fifo,8,,,100,50",
            "gcc,fifo,4,,,100,70",
            "gcc,opt,4,,,100,40",
            "gcc,lru-approx,4,8,100,100,60",
            "gcc,lru-approx,4,8,1000,100,99",
            "gcc,lru-approx,4,4,100,100,98",
        ])
        rows = plot.load_results(self.dir)

        curves = plot.policy_curves(rows, "gcc")

        self.assertEqual(curves["FIFO"], ([4, 8], [70, 50]))
        self.assertEqual(curves["OPT"], ([4], [40]))
        self.assertEqual(curves["LRU aproximado (N=8, I=100)"], ([4], [60]))
        self.assertEqual(len(curves), 3)


class SensitivityCurvesTest(unittest.TestCase):
    """Curvas de sensibilidade do LRU aproximado a N e a I (gráficos b)."""

    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        write_csv(Path(self.tmp.name), "gcc", [
            "gcc,fifo,4,,,100,70",
            "gcc,lru-approx,4,8,100,100,60",
            "gcc,lru-approx,8,8,100,100,30",
            "gcc,lru-approx,4,8,1000,100,65",
            "gcc,lru-approx,4,4,100,100,62",
            "gcc,lru-approx,4,16,100,100,59",
            "gcc,lru-approx,4,4,1000,100,99",
            "sixpack,lru-approx,4,2,100,100,1",
        ])
        self.rows = plot.load_results(Path(self.tmp.name))

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def test_varying_n_keeps_reference_i_and_lists_every_n_from_the_csv(self) -> None:
        curves = plot.sensitivity_curves(self.rows, "gcc", vary="history_bits")

        self.assertEqual(list(curves), ["N=4", "N=8", "N=16"])
        self.assertEqual(curves["N=8"], ([4, 8], [60, 30]))
        self.assertEqual(curves["N=4"], ([4], [62]))

    def test_varying_i_keeps_reference_n(self) -> None:
        curves = plot.sensitivity_curves(self.rows, "gcc", vary="aging_interval")

        self.assertEqual(list(curves), ["I=100", "I=1000"])
        self.assertEqual(curves["I=1000"], ([4], [65]))


class GenerateAllTest(unittest.TestCase):
    """O comando grava os PDF do artigo."""

    def test_writes_three_pdfs_per_trace_into_the_output_dir(self) -> None:
        with tempfile.TemporaryDirectory() as results, tempfile.TemporaryDirectory() as out:
            for trace in ("gcc", "swim"):
                write_csv(Path(results), trace, [
                    f"{trace},fifo,4,,,100,70",
                    f"{trace},opt,4,,,100,40",
                    f"{trace},lru-approx,4,8,100,100,60",
                    f"{trace},lru-approx,4,4,100,100,62",
                    f"{trace},lru-approx,4,8,1000,100,65",
                ])
            output = Path(out) / "figuras"

            written = plot.generate_all(Path(results), output)

            expected = sorted(
                f"{prefix}_{trace}.pdf"
                for trace in ("gcc", "swim")
                for prefix in ("falhas", "sensibilidade_n", "sensibilidade_i")
            )
            self.assertEqual(sorted(p.name for p in written), expected)
            self.assertEqual(sorted(p.name for p in output.iterdir()), expected)
            for path in written:
                self.assertTrue(path.read_bytes().startswith(b"%PDF"))

    def test_warns_when_the_reference_pair_is_missing_but_still_writes_the_faults_pdf(
            self) -> None:
        with tempfile.TemporaryDirectory() as results, tempfile.TemporaryDirectory() as out:
            # Sem 8:100, mas com dados para os dois gráficos de sensibilidade.
            write_csv(Path(results), "gcc", [
                "gcc,fifo,4,,,100,70",
                "gcc,opt,4,,,100,40",
                "gcc,lru-approx,4,4,100,100,62",
                "gcc,lru-approx,4,8,1000,100,65",
            ])
            stderr = io.StringIO()

            with contextlib.redirect_stderr(stderr):
                written = plot.generate_all(Path(results), Path(out))

            self.assertIn("aviso: gcc", stderr.getvalue())
            self.assertIn("LRU aproximado", stderr.getvalue())
            self.assertIn("N=8, I=100", stderr.getvalue())
            self.assertIn(Path(out) / "falhas_gcc.pdf", written)
            self.assertTrue((Path(out) / "falhas_gcc.pdf").exists())

    def test_fails_clearly_when_there_are_no_results(self) -> None:
        with tempfile.TemporaryDirectory() as results, tempfile.TemporaryDirectory() as out:
            with self.assertRaises(SystemExit):
                plot.generate_all(Path(results), Path(out))


if __name__ == "__main__":
    unittest.main()
