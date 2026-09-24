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
};

/// Executa o simulador com os argumentos dados e captura a saída padrão.
RunResult run_sim(const std::string& args) {
    const std::string out_path = kScratchDir + "/sim_stdout.txt";
    const std::string command = kSimBinary + " " + args + " > " + out_path;
    const int status = std::system(command.c_str());

    RunResult result{status, {}};
    std::ifstream in(out_path);
    std::string line;
    while (std::getline(in, line)) {
        result.stdout_lines.push_back(line);
    }
    return result;
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

void test_policy_enough_frames_faults_equal_distinct_pages() {
    // Páginas distintas: {7, 0, 1, 2, 3, 4} → 6.
    const std::size_t distinct_pages = 6;
    for (std::size_t frame_count = distinct_pages; frame_count <= distinct_pages + 4;
         ++frame_count) {
        assert(simulate_fifo(kSilberschatzPages, frame_count) == distinct_pages);
    }
}

}  // namespace

int main() {
    test_policy_enough_frames_faults_equal_distinct_pages();
    test_cli_fifo_silberschatz_three_frames();
    test_cli_emits_one_line_per_frame_count();
    test_cli_addresses_in_same_page_are_same_page();
    std::cout << "all tests passed\n";
    return 0;
}
