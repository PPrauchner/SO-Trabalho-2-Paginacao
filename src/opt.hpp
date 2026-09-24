/**
 * Política OPT (Belady): a vítima é a página cujo próximo uso está mais distante
 * no futuro do trace — ou que nunca mais será usada.
 *
 * Responsabilidades:
 * - Simular um trace sob OPT e contar as falhas de página.
 *
 * É a única política que enxerga o futuro do trace; serve de limite inferior.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

/// Simula um trace sob OPT, com todos os frames começando vazios.
/// @param pages       Páginas acessadas, em ordem.
/// @param frame_count Número de frames disponíveis.
/// @return Número de falhas de página, incluindo as compulsórias.
uint64_t simulate_opt(const std::vector<uint32_t>& pages, std::size_t frame_count);
