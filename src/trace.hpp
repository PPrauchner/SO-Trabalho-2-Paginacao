/**
 * Leitor de trace: converte um arquivo de acessos na sequência de páginas.
 *
 * Responsabilidades:
 * - Ler cada linha `<endereço hex 32 bits> <R|W>` do trace.
 * - Converter o endereço em página (endereço sem os 12 bits de deslocamento),
 *   ignorando o tipo do acesso.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

/// Tamanho da página em bits de deslocamento (4096 bytes).
constexpr uint32_t kPageOffsetBits = 12;

/// Lê o trace inteiro e devolve a página de cada acesso, em ordem.
/// @param path Caminho do arquivo de trace.
/// @return Páginas acessadas, uma por acesso.
/// @throws std::runtime_error se o arquivo não abrir ou uma linha for malformada.
std::vector<uint32_t> read_trace(const std::string& path);
