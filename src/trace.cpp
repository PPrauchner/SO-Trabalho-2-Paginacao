/**
 * Implementação do leitor de trace.
 *
 * Responsabilidades:
 * - Abrir o arquivo e percorrê-lo uma única vez.
 * - Validar cada linha (endereço hex de 32 bits, tipo R ou W, nada além disso).
 * - Extrair a página de cada acesso.
 */
#include "trace.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

/// Converte o campo de endereço: de 1 a 8 dígitos hexadecimais, nada mais.
/// @param field   Texto do campo.
/// @param address Endereço convertido, quando válido.
/// @return Se o campo é um endereço válido.
bool parse_address(const std::string& field, uint32_t& address) {
    if (field.empty() || field.size() > 8 ||
        field.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
        return false;
    }
    address = static_cast<uint32_t>(std::stoul(field, nullptr, 16));
    return true;
}

/// Extrai a página de uma linha `<endereço hex 32 bits> <R|W>`.
/// @param line Linha do trace.
/// @param page Página do acesso, quando a linha é válida.
/// @return Se a linha é um acesso válido.
bool parse_access(const std::string& line, uint32_t& page) {
    std::istringstream fields(line);
    std::string address_field;
    std::string type_field;
    std::string extra_field;
    uint32_t address = 0;
    if (!(fields >> address_field >> type_field) || (fields >> extra_field) ||
        !parse_address(address_field, address) ||
        (type_field != "R" && type_field != "W")) {
        return false;
    }
    page = address >> kPageOffsetBits;
    return true;
}

}  // namespace

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
        uint32_t page = 0;
        if (!parse_access(line, page)) {
            throw std::runtime_error(path + ":" + std::to_string(line_number) +
                                     ": malformed access (expected '<hex address> <R|W>'): " +
                                     line);
        }
        pages.push_back(page);
    }
    if (in.bad()) {
        throw std::runtime_error("cannot read trace: " + path);
    }
    return pages;
}
