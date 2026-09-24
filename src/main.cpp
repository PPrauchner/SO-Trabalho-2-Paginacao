/**
 * CLI do simulador: lê um trace uma vez e emite uma linha de CSV por simulação.
 *
 * Responsabilidades:
 * - Interpretar os argumentos (trace, política, números de frames).
 * - Rodar uma simulação por número de frames sobre o mesmo trace em memória.
 * - Imprimir o cabeçalho e as linhas do CSV na saída padrão.
 *
 * Uso: sim <trace> <policy> <frame_count>...
 */
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "fifo.hpp"
#include "opt.hpp"
#include "trace.hpp"

namespace {

const char* const kUsage =
    "usage: sim <trace> <policy> <frame_count>...\n"
    "  policy: fifo\n";

/// Nome do trace no CSV: o nome do arquivo sem diretório e sem extensão.
std::string trace_name(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    const std::size_t dot = name.find_last_of('.');
    if (dot != std::string::npos && dot != 0) {
        name.erase(dot);
    }
    return name;
}

/// Converte um argumento em número de frames (inteiro positivo).
/// @throws std::invalid_argument se o argumento não for um inteiro positivo.
std::size_t parse_frame_count(const std::string& arg) {
    std::size_t consumed = 0;
    const unsigned long long value = std::stoull(arg, &consumed);
    if (consumed != arg.size() || value == 0 || arg[0] == '-') {
        throw std::invalid_argument("invalid frame count: " + arg);
    }
    return static_cast<std::size_t>(value);
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << kUsage;
        return 2;
    }
    const std::string trace_path = argv[1];
    const std::string policy = argv[2];
    if (policy != "fifo" && policy != "opt") {
        std::cerr << "unknown policy: " << policy << '\n' << kUsage;
        return 2;
    }

    std::vector<std::size_t> frame_counts;
    try {
        for (int i = 3; i < argc; ++i) {
            frame_counts.push_back(parse_frame_count(argv[i]));
        }
    } catch (const std::exception&) {
        std::cerr << "invalid frame count\n" << kUsage;
        return 2;
    }

    std::vector<uint32_t> pages;
    try {
        pages = read_trace(trace_path);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }

    const std::string name = trace_name(trace_path);
    std::cout << "trace,policy,frames,history_bits,aging_interval,accesses,page_faults\n";
    for (std::size_t frame_count : frame_counts) {
        const uint64_t page_faults = policy == "opt" ? simulate_opt(pages, frame_count)
                                                     : simulate_fifo(pages, frame_count);
        std::cout << name << ',' << policy << ',' << frame_count << ",,," << pages.size()
                  << ',' << page_faults << '\n';
    }
    return 0;
}
