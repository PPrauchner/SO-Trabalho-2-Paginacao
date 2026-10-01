/**
 * Testes do simulador: CLI de ponta a ponta e interface comum das políticas.
 *
 * Responsabilidades:
 * - Executar o binário build/sim sobre traces escritos à mão e conferir o CSV.
 * - Chamar as políticas em processo, sem arquivo envolvido.
 * - Executar a grade de experimentos (scripts/run_grid.sh) com uma configuração
 *   própria e conferir os CSV gerados.
 *
 * Qualquer assert que falhar aborta o processo com código ≠ 0, o que faz o
 * `make test` falhar.
 */

#undef NDEBUG
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "fifo.hpp"
#include "lru_approx.hpp"
#include "opt.hpp"

namespace {

const std::string kSimBinary = "./build/sim";
const std::string kScratchDir = "build/tests";

/// Escreve um trace no disco, um acesso por linha.
/// @param name  Nome do arquivo dentro do diretório de rascunho dos testes.
/// @param lines Linhas do trace (`<endereço hex> <R|W>`).
/// @return Caminho do arquivo escrito.
std::string write_trace(const std::string& name, const std::vector<std::string>& lines) {
    const std::string path = kScratchDir + "/" + name;
    std::ofstream out(path);
    for (const std::string& line : lines) {
        out << line << '\n';
    }
    return path;
}

/// Converte números de página em linhas de trace, alternando leitura e escrita.
std::vector<std::string> trace_lines_for_pages(const std::vector<uint32_t>& pages) {
    std::vector<std::string> lines;
    bool write = false;
    for (uint32_t page : pages) {
        std::ostringstream line;
        line << std::hex << (page << 12) << (write ? " W" : " R");
        lines.push_back(line.str());
        write = !write;
    }
    return lines;
}

struct RunResult {
    int exit_code;
    std::vector<std::string> stdout_lines;
    std::string stderr_text;
};

/// Executa o simulador com os argumentos dados e captura a saída padrão e a de erro.
RunResult run_sim(const std::string& args) {
    const std::string out_path = kScratchDir + "/sim_stdout.txt";
    const std::string err_path = kScratchDir + "/sim_stderr.txt";
    const std::string command =
        kSimBinary + " " + args + " > " + out_path + " 2> " + err_path;
    const int status = std::system(command.c_str());

    RunResult result{status, {}, {}};
    std::ifstream out(out_path);
    std::string line;
    while (std::getline(out, line)) {
        result.stdout_lines.push_back(line);
    }
    std::ifstream err(err_path);
    std::ostringstream err_text;
    err_text << err.rdbuf();
    result.stderr_text = err_text.str();
    return result;
}

/// Confere que a execução falhou por erro de entrada: código ≠ 0, nada em stdout
/// e uma mensagem em stderr contendo o trecho esperado.
/// @param args             Argumentos passados ao simulador.
/// @param expected_message Trecho que a mensagem de erro deve conter.
void expect_input_error(const std::string& args, const std::string& expected_message) {
    const RunResult result = run_sim(args);
    if (result.exit_code == 0 || !result.stdout_lines.empty() ||
        result.stderr_text.find(expected_message) == std::string::npos) {
        std::cerr << "args: " << args << "\nexit: " << result.exit_code
                  << "\nstdout lines: " << result.stdout_lines.size()
                  << "\nstderr: " << result.stderr_text << '\n';
    }
    assert(result.exit_code != 0);
    assert(result.stdout_lines.empty());
    assert(result.stderr_text.find(expected_message) != std::string::npos);
}

const std::vector<uint32_t> kSilberschatzPages = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3,
                                                  0, 3, 2, 1, 2, 0, 1, 7, 0, 1};
const std::string kCsvHeader =
    "trace,policy,frames,history_bits,aging_interval,accesses,page_faults";

void test_cli_fifo_silberschatz_three_frames() {
    const std::string trace =
        write_trace("silberschatz.trace", trace_lines_for_pages(kSilberschatzPages));

    const RunResult result = run_sim(trace + " fifo 3");

    assert(result.exit_code == 0);
    assert(result.stdout_lines.size() == 2);
    assert(result.stdout_lines[0] == kCsvHeader);
    assert(result.stdout_lines[1] == "silberschatz,fifo,3,,,20,15");
}

void test_cli_opt_silberschatz_three_frames() {
    const std::string trace =
        write_trace("silberschatz.trace", trace_lines_for_pages(kSilberschatzPages));

    const RunResult result = run_sim(trace + " opt 3");

    assert(result.exit_code == 0);
    assert(result.stdout_lines.size() == 2);
    assert(result.stdout_lines[0] == kCsvHeader);
    assert(result.stdout_lines[1] == "silberschatz,opt,3,,,20,9");
}

void test_cli_emits_one_line_per_frame_count() {
    const std::string trace =
        write_trace("silberschatz.trace", trace_lines_for_pages(kSilberschatzPages));

    const RunResult result = run_sim(trace + " fifo 1 3 4 7");

    assert(result.exit_code == 0);
    assert(result.stdout_lines.size() == 5);
    assert(result.stdout_lines[0] == kCsvHeader);
    assert(result.stdout_lines[1] == "silberschatz,fifo,1,,,20,20");
    assert(result.stdout_lines[2] == "silberschatz,fifo,3,,,20,15");
    assert(result.stdout_lines[3] == "silberschatz,fifo,4,,,20,10");
    assert(result.stdout_lines[4] == "silberschatz,fifo,7,,,20,6");
}

void test_cli_addresses_in_same_page_are_same_page() {
    // Três endereços da página 0x31348 (lido e escrito), depois a página 0x31349.
    const std::string trace = write_trace(
        "same_page.trace", {"31348000 R", "31348fff W", "31348900 R", "31349000 W"});

    const RunResult result = run_sim(trace + " fifo 1");

    assert(result.exit_code == 0);
    assert(result.stdout_lines.size() == 2);
    assert(result.stdout_lines[1] == "same_page,fifo,1,,,4,2");
}

void test_cli_rejects_access_type_other_than_read_or_write() {
    const std::string trace = write_trace("bad_type.trace", {"1000 R", "2000 X"});

    expect_input_error(trace + " fifo 1", "bad_type.trace:2:");
}

void test_cli_rejects_non_hex_address() {
    const std::string trace =
        write_trace("bad_address.trace", {"1000 R", "2000 W", "12zz W"});

    expect_input_error(trace + " fifo 1", "bad_address.trace:3:");
}

void test_cli_rejects_address_wider_than_32_bits() {
    const std::string trace = write_trace("wide_address.trace", {"123456789 R"});

    expect_input_error(trace + " fifo 1", "wide_address.trace:1:");
}

void test_cli_rejects_line_with_missing_fields() {
    const std::string trace = write_trace("missing_type.trace", {"1000 R", "2000"});
    expect_input_error(trace + " fifo 1", "missing_type.trace:2:");

    const std::string blank = write_trace("blank_line.trace", {"1000 R", "", "2000 W"});
    expect_input_error(blank + " fifo 1", "blank_line.trace:2:");
}

void test_cli_rejects_line_with_extra_fields() {
    const std::string trace = write_trace("extra_field.trace", {"1000 R 7"});

    expect_input_error(trace + " fifo 1", "extra_field.trace:1:");
}

void test_cli_rejects_missing_trace() {
    expect_input_error(kScratchDir + "/does_not_exist.trace fifo 1",
                       "cannot open trace");
}

void test_cli_rejects_unreadable_trace() {
    // Um diretório abre como arquivo, mas não pode ser lido.
    expect_input_error(kScratchDir + " fifo 1", "cannot read trace");
}

void test_cli_rejects_unknown_policy() {
    const std::string trace =
        write_trace("silberschatz.trace", trace_lines_for_pages(kSilberschatzPages));

    expect_input_error(trace + " lifo 3", "usage:");
}

void test_cli_rejects_non_positive_frame_count() {
    const std::string trace =
        write_trace("silberschatz.trace", trace_lines_for_pages(kSilberschatzPages));

    expect_input_error(trace + " fifo 0", "usage:");
    expect_input_error(trace + " fifo -3", "usage:");
    expect_input_error(trace + " fifo 3 abc", "usage:");
    expect_input_error(trace + " fifo 3x", "usage:");
    // O stoull pularia o espaço e converteria o -1 em ULLONG_MAX.
    expect_input_error(trace + " fifo ' -1'", "usage:");
    expect_input_error(trace + " fifo +5", "usage:");
}

void test_cli_rejects_missing_arguments() {
    const std::string trace =
        write_trace("silberschatz.trace", trace_lines_for_pages(kSilberschatzPages));

    expect_input_error(trace + " fifo", "usage:");
    expect_input_error(trace, "usage:");
    expect_input_error("", "usage:");
}

void test_policy_enough_frames_faults_equal_distinct_pages() {
    // Páginas distintas: {7, 0, 1, 2, 3, 4} → 6.
    const std::size_t distinct_pages = 6;
    for (std::size_t frame_count = distinct_pages; frame_count <= distinct_pages + 4;
         ++frame_count) {
        assert(simulate_fifo(kSilberschatzPages, frame_count) == distinct_pages);
        assert(simulate_opt(kSilberschatzPages, frame_count) == distinct_pages);
        assert(simulate_lru_approx(kSilberschatzPages, frame_count, 8, 4) ==
               distinct_pages);
    }
}

/// Sequência pseudoaleatória determinística de páginas (gerador congruencial linear).
/// @param length     Número de acessos.
/// @param page_range Páginas geradas ficam em [0, page_range).
/// @param seed       Semente do gerador.
std::vector<uint32_t> pseudo_random_pages(std::size_t length, uint32_t page_range,
                                          uint32_t seed) {
    std::vector<uint32_t> pages;
    uint32_t state = seed;
    for (std::size_t i = 0; i < length; ++i) {
        state = state * 1664525u + 1013904223u;
        pages.push_back((state >> 16) % page_range);
    }
    return pages;
}

void test_opt_evicts_page_never_used_again() {
    // Com 2 frames, o acesso à página 3 exige uma vítima: a 1 nunca mais é usada e
    // a 2 é. Escolher a 1 dá 3 falhas; escolher a 2 (o que o FIFO faz) daria 4.
    const std::vector<uint32_t> pages = {2, 1, 3, 2, 3};

    assert(simulate_opt(pages, 2) == 3);
    assert(simulate_fifo(pages, 2) == 4);
}

void test_opt_never_worse_than_fifo() {
    const std::vector<std::vector<uint32_t>> traces = {
        kSilberschatzPages,
        {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5},  // sequência da anomalia de Belady
        pseudo_random_pages(2000, 10, 1),
        pseudo_random_pages(2000, 40, 7),
    };
    for (const std::vector<uint32_t>& pages : traces) {
        for (std::size_t frame_count = 1; frame_count <= 12; ++frame_count) {
            assert(simulate_opt(pages, frame_count) <= simulate_fifo(pages, frame_count));
        }
    }
}

void test_opt_is_deterministic() {
    const std::vector<uint32_t> pages = pseudo_random_pages(5000, 50, 3);
    for (std::size_t frame_count : {4, 8, 16}) {
        const uint64_t first = simulate_opt(pages, frame_count);
        for (int run = 0; run < 3; ++run) {
            assert(simulate_opt(pages, frame_count) == first);
        }
    }
}

void test_lru_approx_spares_recently_used_page_on_history_tie() {
    // 2 frames, N = 2, I = 2. O envelhecimento no acesso 2 deixa as páginas 1 e 2
    // com o mesmo histórico e bit de referência desligado; o acesso 3 religa o bit
    // da página 1. No acesso 4 a vítima é a 2 (bit desligado), não a 1 — que o
    // FIFO expulsaria por ser a carregada há mais tempo. O acesso 5 acerta a 1.
    const std::vector<uint32_t> pages = {1, 2, 1, 3, 1};

    assert(simulate_lru_approx(pages, 2, 2, 2) == 3);
    assert(simulate_fifo(pages, 2) == 4);
}

void test_lru_approx_full_tie_evicts_oldest_loaded_page() {
    // 2 frames, sem envelhecimento antes da falha (I = 100): no acesso 3 as páginas
    // 2 e 1 empatam em histórico e bit de referência. A vítima é a 2, carregada há
    // mais tempo (não a de menor número); o acesso 4 acerta a 1.
    const std::vector<uint32_t> pages = {2, 1, 3, 1};

    assert(simulate_lru_approx(pages, 2, 8, 100) == 3);
}

void test_lru_approx_spares_page_loaded_since_last_aging() {
    // 2 frames, N = 8, I = 2. O envelhecimento no acesso 2 dá às páginas 1 e 2 o
    // mesmo histórico e desliga os bits; o acesso 3 expulsa a 1 (a mais antiga) e
    // carrega a 3 com histórico zerado e bit ligado. No acesso 4 a vítima é a 2 (bit
    // desligado), não a 3 — que foi usada neste intervalo, embora tenha histórico
    // menor. O acesso 5 acerta a 3.
    const std::vector<uint32_t> pages = {1, 2, 3, 4, 3};

    assert(simulate_lru_approx(pages, 2, 8, 2) == 4);
}

void test_lru_approx_ages_exactly_on_multiple_of_interval() {
    // 2 frames, N = 2, sequência 1 2 1 3 2.
    // I = 2: o envelhecimento no acesso 2 desliga os bits; o acesso 3 religa o da
    // página 1, então o acesso 4 expulsa a 2 e o acesso 5 falha → 4 falhas.
    // I = 3: o envelhecimento só ocorre no acesso 3 e desliga os dois bits; o acesso
    // 4 cai no empate total e expulsa a 1 (a mais antiga); o acesso 5 acerta → 3.
    const std::vector<uint32_t> pages = {1, 2, 1, 3, 2};

    assert(simulate_lru_approx(pages, 2, 2, 2) == 4);
    assert(simulate_lru_approx(pages, 2, 2, 3) == 3);
}

void test_lru_approx_small_history_saturates_with_interval_one() {
    // 3 frames, I = 1 (envelhece a cada acesso), sequência 1 2 1 3 3 3 4 2.
    // N = 8 ainda lembra que a 2 foi usada antes da 1: o acesso 4 (página 4) expulsa
    // a 2 e o último acesso falha → 5 falhas.
    // N = 2 só guarda os dois últimos acessos, ambos à página 3: as páginas 1 e 2
    // empatam com histórico zerado, a 1 (mais antiga) sai e o último acesso acerta → 4.
    const std::vector<uint32_t> pages = {1, 2, 1, 3, 3, 3, 4, 2};

    assert(simulate_lru_approx(pages, 3, 8, 1) == 5);
    assert(simulate_lru_approx(pages, 3, 2, 1) == 4);
}

void test_opt_never_worse_than_lru_approx() {
    const std::vector<std::vector<uint32_t>> traces = {
        kSilberschatzPages,
        {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5},
        pseudo_random_pages(2000, 10, 1),
        pseudo_random_pages(2000, 40, 7),
    };
    const std::vector<std::pair<uint32_t, uint64_t>> history_and_interval = {
        {1, 1}, {2, 3}, {8, 1}, {8, 16}, {32, 5}};
    for (const std::vector<uint32_t>& pages : traces) {
        for (std::size_t frame_count = 1; frame_count <= 12; ++frame_count) {
            for (const auto& [history_bits, aging_interval] : history_and_interval) {
                assert(simulate_opt(pages, frame_count) <=
                       simulate_lru_approx(pages, frame_count, history_bits,
                                           aging_interval));
            }
        }
    }
}

void test_lru_approx_is_deterministic() {
    const std::vector<uint32_t> pages = pseudo_random_pages(5000, 50, 3);
    for (std::size_t frame_count : {4, 8, 16}) {
        const uint64_t first = simulate_lru_approx(pages, frame_count, 8, 4);
        for (int run = 0; run < 3; ++run) {
            assert(simulate_lru_approx(pages, frame_count, 8, 4) == first);
        }
    }
}

void test_cli_lru_approx_fills_history_bits_and_aging_interval() {
    // Mesma sequência do teste de fronteira do envelhecimento: I = 2 → 4 falhas.
    const std::string trace =
        write_trace("aging.trace", trace_lines_for_pages({1, 2, 1, 3, 2}));

    const RunResult result = run_sim(trace + " lru-approx 2 2 2 5");

    assert(result.exit_code == 0);
    assert(result.stdout_lines.size() == 3);
    assert(result.stdout_lines[0] == kCsvHeader);
    assert(result.stdout_lines[1] == "aging,lru-approx,2,2,2,5,4");
    assert(result.stdout_lines[2] == "aging,lru-approx,5,2,2,5,3");
}

void test_cli_rejects_invalid_lru_approx_parameters() {
    const std::string trace =
        write_trace("silberschatz.trace", trace_lines_for_pages(kSilberschatzPages));

    expect_input_error(trace + " lru-approx 0 4 3", "usage:");
    expect_input_error(trace + " lru-approx 33 4 3", "usage:");
    expect_input_error(trace + " lru-approx -1 4 3", "usage:");
    expect_input_error(trace + " lru-approx 8 0 3", "usage:");
    expect_input_error(trace + " lru-approx 8 -2 3", "usage:");
    expect_input_error(trace + " lru-approx 8x 4 3", "usage:");
    expect_input_error(trace + " lru-approx 8 4", "usage:");
    expect_input_error(trace + " lru-approx 8", "usage:");
    expect_input_error(trace + " lru-approx", "usage:");
}

void test_cli_accepts_history_bits_bounds() {
    const std::string trace =
        write_trace("silberschatz.trace", trace_lines_for_pages(kSilberschatzPages));

    for (const std::string history_bits : {"1", "32"}) {
        const RunResult result = run_sim(trace + " lru-approx " + history_bits + " 1 3");
        assert(result.exit_code == 0);
        assert(result.stdout_lines.size() == 2);
    }
}

/// Trace sintético grande, gerado por um LCG de semente fixa: endereços espalhados
/// por algumas centenas de páginas, com deslocamento dentro da página e leituras e
/// escritas misturadas.
/// @param access_count Número de acessos do trace.
/// @param page_span    Número de páginas distintas possíveis.
/// @return Linhas do trace (`<endereço hex> <R|W>`).
std::vector<std::string> synthetic_trace_lines(uint64_t access_count, uint32_t page_span) {
    uint32_t state = 12345;
    const auto next = [&state]() {
        state = state * 1664525u + 1013904223u;  // Numerical Recipes
        return state >> 8;  // descarta os bits baixos, de período curto
    };
    std::vector<std::string> lines;
    lines.reserve(access_count);
    for (uint64_t i = 0; i < access_count; ++i) {
        const uint32_t page = next() % page_span;
        const uint32_t offset = next() % 4096;
        const bool write = next() % 2 == 1;
        std::ostringstream line;
        line << std::hex << ((page << 12) | offset) << (write ? " W" : " R");
        lines.push_back(line.str());
    }
    return lines;
}

void test_cli_is_deterministic_on_large_trace() {
    const std::string trace =
        write_trace("synthetic.trace", synthetic_trace_lines(50000, 300));

    for (const std::string policy_args : {"fifo", "opt", "lru-approx 8 100"}) {
        const std::string args = trace + " " + policy_args + " 4 8 16 64 256";
        const RunResult first = run_sim(args);
        const RunResult second = run_sim(args);
        assert(first.exit_code == 0 && second.exit_code == 0);
        assert(first.stdout_lines.size() == 6);
        assert(first.stdout_lines == second.stdout_lines);
    }
}

/// Lê um arquivo inteiro, linha a linha.
std::vector<std::string> read_lines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    return lines;
}

struct GridRun {
    int exit_code;
    std::string stderr_text;
};

/// Executa a grade de experimentos com uma configuração própria do teste.
/// @param traces  Nomes dos traces da grade (sem `.trace`).
/// @param present Traces que existem no diretório de traces do teste.
/// @param broken  Traces presentes com uma linha inválida, que fazem o simulador
///                falhar.
/// @param from_other_dir Executa a grade a partir do diretório do teste, não da
///                       raiz do repositório.
/// @return Código de saída e a saída de erro da grade.
GridRun run_grid(const std::vector<std::string>& traces,
                 const std::vector<std::string>& present,
                 const std::vector<std::string>& broken = {}, bool from_other_dir = false) {
    const std::string dir = kScratchDir + "/grid";
    const int setup_status =
        std::system(("rm -rf " + dir + " && mkdir -p " + dir + "/traces").c_str());
    assert(setup_status == 0);
    for (const std::string& name : present) {
        write_trace("grid/traces/" + name + ".trace",
                    trace_lines_for_pages(kSilberschatzPages));
    }
    for (const std::string& name : broken) {
        write_trace("grid/traces/" + name + ".trace", {"1000 R", "zzzz W"});
    }
    std::ofstream conf(dir + "/grid.conf");
    conf << "TRACES=\"";
    for (std::size_t i = 0; i < traces.size(); ++i) {
        conf << (i == 0 ? "" : " ") << traces[i];
    }
    conf << "\"\nFRAMES=\"3 4\"\nLRU_PAIRS=\"8:1 2:4\"\n";
    conf.close();

    const std::string err_path = dir + "/stderr.txt";
    // Fora da raiz, o simulador fica no padrão do script: é o caminho que ele
    // precisa achar sozinho, sem depender do diretório atual.
    const std::string command =
        from_other_dir
            ? "root=\"$(pwd)\" && cd " + dir + " && GRID_CONF=\"$root/" + dir +
                  "/grid.conf\" TRACES_DIR=\"$root/" + dir + "/traces\" RESULTS_DIR=\"$root/" +
                  dir + "/results\" bash \"$root/scripts/run_grid.sh\" > /dev/null 2> stderr.txt"
            : "GRID_CONF=" + dir + "/grid.conf TRACES_DIR=" + dir + "/traces RESULTS_DIR=" +
                  dir + "/results SIM=" + kSimBinary +
                  " bash scripts/run_grid.sh > /dev/null 2> " + err_path;
    const int status = std::system(command.c_str());
    std::ifstream err(err_path);
    std::ostringstream err_text;
    err_text << err.rdbuf();
    return {status, err_text.str()};
}

void test_grid_writes_one_csv_per_trace_with_every_simulation() {
    const GridRun run = run_grid({"alpha"}, {"alpha"});

    assert(run.exit_code == 0);
    const std::vector<std::string> csv = read_lines(kScratchDir + "/grid/results/alpha.csv");
    const std::vector<std::string> expected = {
        kCsvHeader,
        "alpha,fifo,3,,,20,15",
        "alpha,fifo,4,,,20,10",
        "alpha,opt,3,,,20,9",
        "alpha,opt,4,,,20,8",
    };
    // Cabeçalho único, FIFO e OPT primeiro, depois um bloco por par N/I.
    assert(csv.size() == expected.size() + 4);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        assert(csv[i] == expected[i]);
    }
    assert(csv[5].rfind("alpha,lru-approx,3,8,1,20,", 0) == 0);
    assert(csv[6].rfind("alpha,lru-approx,4,8,1,20,", 0) == 0);
    assert(csv[7].rfind("alpha,lru-approx,3,2,4,20,", 0) == 0);
    assert(csv[8].rfind("alpha,lru-approx,4,2,4,20,", 0) == 0);
}

void test_grid_warns_about_missing_trace_and_runs_the_others() {
    const GridRun run = run_grid({"alpha", "ghost", "beta"}, {"alpha", "beta"});

    assert(run.exit_code == 0);
    assert(run.stderr_text.find("ghost.trace") != std::string::npos);
    assert(read_lines(kScratchDir + "/grid/results/alpha.csv").size() == 9);
    assert(read_lines(kScratchDir + "/grid/results/beta.csv").size() == 9);
    std::ifstream ghost(kScratchDir + "/grid/results/ghost.csv");
    assert(!ghost.good());
}

void test_grid_failure_leaves_no_temporary_file_in_results() {
    const GridRun run = run_grid({"alpha", "broken"}, {"alpha"}, {"broken"});

    assert(run.exit_code != 0);
    const int no_tmp_status = std::system(
        ("test -z \"$(find " + kScratchDir + "/grid/results -name '*.tmp')\"").c_str());
    assert(no_tmp_status == 0);
}

void test_grid_runs_from_any_directory() {
    const GridRun run = run_grid({"alpha"}, {"alpha"}, {}, true);

    if (run.exit_code != 0) {
        std::cerr << "stderr: " << run.stderr_text << '\n';
    }
    assert(run.exit_code == 0);
    assert(read_lines(kScratchDir + "/grid/results/alpha.csv").size() == 9);
}

}  // namespace

int main() {
    test_policy_enough_frames_faults_equal_distinct_pages();
    test_opt_evicts_page_never_used_again();
    test_opt_never_worse_than_fifo();
    test_opt_is_deterministic();
    test_lru_approx_spares_recently_used_page_on_history_tie();
    test_lru_approx_full_tie_evicts_oldest_loaded_page();
    test_lru_approx_spares_page_loaded_since_last_aging();
    test_lru_approx_ages_exactly_on_multiple_of_interval();
    test_lru_approx_small_history_saturates_with_interval_one();
    test_opt_never_worse_than_lru_approx();
    test_lru_approx_is_deterministic();
    test_cli_fifo_silberschatz_three_frames();
    test_cli_opt_silberschatz_three_frames();
    test_cli_emits_one_line_per_frame_count();
    test_cli_addresses_in_same_page_are_same_page();
    test_cli_rejects_access_type_other_than_read_or_write();
    test_cli_rejects_non_hex_address();
    test_cli_rejects_address_wider_than_32_bits();
    test_cli_rejects_line_with_missing_fields();
    test_cli_rejects_line_with_extra_fields();
    test_cli_rejects_missing_trace();
    test_cli_rejects_unreadable_trace();
    test_cli_rejects_unknown_policy();
    test_cli_rejects_non_positive_frame_count();
    test_cli_rejects_missing_arguments();
    test_cli_lru_approx_fills_history_bits_and_aging_interval();
    test_cli_rejects_invalid_lru_approx_parameters();
    test_cli_accepts_history_bits_bounds();
    test_cli_is_deterministic_on_large_trace();
    test_grid_writes_one_csv_per_trace_with_every_simulation();
    test_grid_warns_about_missing_trace_and_runs_the_others();
    test_grid_failure_leaves_no_temporary_file_in_results();
    test_grid_runs_from_any_directory();
    std::cout << "all tests passed\n";
    return 0;
}
