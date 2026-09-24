/**
 * Testes do simulador: CLI de ponta a ponta e interface comum das políticas.
 *
 * Responsabilidades:
 * - Executar o binário build/sim sobre traces escritos à mão e conferir o CSV.
 * - Chamar as políticas em processo, sem arquivo envolvido.
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
#include <vector>

#include "fifo.hpp"
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

}  // namespace

int main() {
    test_policy_enough_frames_faults_equal_distinct_pages();
    test_opt_evicts_page_never_used_again();
    test_opt_never_worse_than_fifo();
    test_opt_is_deterministic();
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
    std::cout << "all tests passed\n";
    return 0;
}
