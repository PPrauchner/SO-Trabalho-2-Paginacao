/**
 * Política FIFO: a vítima é a página carregada há mais tempo.
 *
 * Responsabilidades:
 * - Simular um trace sob FIFO e contar as falhas de página.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

/// Simula um trace sob FIFO, com todos os frames começando vazios.
/// @param pages       Páginas acessadas, em ordem.
/// @param frame_count Número de frames disponíveis; ao menos 1.
/// @return Número de falhas de página, incluindo as compulsórias.
uint64_t simulate_fifo(const std::vector<uint32_t>& pages, std::size_t frame_count);
