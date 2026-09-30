/**
 * Política LRU aproximado (envelhecimento): a vítima é a página de bit de referência
 * desligado, desempatando pelo menor histórico e, depois, pela carregada há mais
 * tempo.
 *
 * Responsabilidades:
 * - Simular um trace sob LRU aproximado e contar as falhas de página.
 *
 * Não enxerga o futuro do trace: cada decisão usa só os acessos já feitos.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

/// Menor e maior número de bits de histórico aceitos.
constexpr uint32_t kMinHistoryBits = 1;
constexpr uint32_t kMaxHistoryBits = 32;

/// Simula um trace sob LRU aproximado, com todos os frames começando vazios.
/// @param pages          Páginas acessadas, em ordem.
/// @param frame_count    Número de frames disponíveis; ao menos 1.
/// @param history_bits   Bits de histórico (N), entre kMinHistoryBits e kMaxHistoryBits.
/// @param aging_interval Intervalo de envelhecimento (I), em acessos; ao menos 1.
/// @return Número de falhas de página, incluindo as compulsórias.
uint64_t simulate_lru_approx(const std::vector<uint32_t>& pages, std::size_t frame_count,
                             uint32_t history_bits, uint64_t aging_interval);
