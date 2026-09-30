/**
 * CLI do simulador: lê um trace uma vez e emite uma linha de CSV por simulação.
 *
 * Responsabilidades:
 * - Interpretar os argumentos (trace, política, N e I do LRU aproximado, números
 *   de frames).
 * - Rodar uma simulação por número de frames sobre o mesmo trace em memória.
 * - Imprimir o cabeçalho e as linhas do CSV na saída padrão.
 *
 * Uso: sim <trace> fifo|opt <frame_count>...
 *      sim <trace> lru-approx <history_bits> <aging_interval> <frame_count>...
 */
#include <cctype>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "fifo.hpp"
#include "lru_approx.hpp"
#include "opt.hpp"
#include "trace.hpp"

namespace {

const char* const kUsage =
    "usage: sim <trace> fifo|opt <frame_count>...\n"
    "       sim <trace> lru-approx <history_bits> <aging_interval> <frame_count>...\n"
    "  history_bits: 1-32; aging_interval: accesses between agings, >= 1\n";

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

/// Converte um argumento em inteiro positivo, só com dígitos decimais.
/// @throws std::invalid_argument se o argumento não for um inteiro positivo.
uint64_t parse_positive(const std::string& arg) {
    // O stoull pula espaços e aceita sinal (" -1" viraria ULLONG_MAX): exige um
    // dígito logo no início.
    if (arg.empty() || !std::isdigit(static_cast<unsigned char>(arg[0]))) {
        throw std::invalid_argument("not a positive integer: " + arg);
    }
    std::size_t consumed = 0;
    const unsigned long long value = std::stoull(arg, &consumed);
    if (consumed != arg.size() || value == 0) {
        throw std::invalid_argument("not a positive integer: " + arg);
    }
    return static_cast<uint64_t>(value);
}

/// Converte um argumento em bits de histórico (N), entre kMinHistoryBits e
/// kMaxHistoryBits.
/// @throws std::invalid_argument se o argumento estiver fora da faixa.
uint32_t parse_history_bits(const std::string& arg) {
    const uint64_t bits = parse_positive(arg);
    if (bits < kMinHistoryBits || bits > kMaxHistoryBits) {
        throw std::invalid_argument("history bits out of range: " + arg);
    }
    return static_cast<uint32_t>(bits);
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << kUsage;
        return 2;
    }
    const std::string trace_path = argv[1];
    const std::string policy = argv[2];
    const bool is_lru_approx = policy == "lru-approx";
    if (policy != "fifo" && policy != "opt" && !is_lru_approx) {
        std::cerr << "unknown policy: " << policy << '\n' << kUsage;
        return 2;
    }

    // N e I só existem no LRU aproximado e vêm antes dos números de frames.
    int first_frame_arg = 3;
    uint32_t history_bits = 0;
    uint64_t aging_interval = 0;
    if (is_lru_approx) {
        first_frame_arg = 5;
        if (argc <= first_frame_arg) {
            std::cerr << kUsage;
            return 2;
        }
        try {
            history_bits = parse_history_bits(argv[3]);
            aging_interval = parse_positive(argv[4]);
        } catch (const std::exception&) {
            std::cerr << "invalid history bits (N) or aging interval (I)\n" << kUsage;
            return 2;
        }
    }

    std::vector<std::size_t> frame_counts;
    try {
        for (int i = first_frame_arg; i < argc; ++i) {
            frame_counts.push_back(static_cast<std::size_t>(parse_positive(argv[i])));
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
    // N e I ficam vazios no CSV para as políticas que não os usam.
    const std::string lru_params =
        is_lru_approx ? std::to_string(history_bits) + ',' + std::to_string(aging_interval)
                      : ",";
    std::cout << "trace,policy,frames,history_bits,aging_interval,accesses,page_faults\n";
    for (std::size_t frame_count : frame_counts) {
        uint64_t page_faults = 0;
        if (is_lru_approx) {
            page_faults =
                simulate_lru_approx(pages, frame_count, history_bits, aging_interval);
        } else if (policy == "opt") {
            page_faults = simulate_opt(pages, frame_count);
        } else {
            page_faults = simulate_fifo(pages, frame_count);
        }
        std::cout << name << ',' << policy << ',' << frame_count << ',' << lru_params
                  << ',' << pages.size() << ',' << page_faults << '\n';
    }
    return 0;
}
