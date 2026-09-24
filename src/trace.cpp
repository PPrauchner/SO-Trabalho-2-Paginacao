/**
 * Implementação do leitor de trace.
 *
 * Responsabilidades:
 * - Abrir o arquivo e percorrê-lo uma única vez.
 * - Extrair a página de cada acesso.
 */
#include "trace.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::vector<uint32_t> read_trace(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open trace: " + path);
    }

    std::vector<uint32_t> pages;
    std::string line;
    uint64_t line_number = 0;
    while (std::getline(in, line)) {
        ++line_number;
        std::istringstream fields(line);
        uint32_t address = 0;
        if (!(fields >> std::hex >> address)) {
            throw std::runtime_error(path + ":" + std::to_string(line_number) +
                                     ": malformed access");
        }
        pages.push_back(address >> kPageOffsetBits);
    }
    return pages;
}
