/**
 * Implementação da política LRU aproximado.
 *
 * Responsabilidades:
 * - Manter, por frame, a página, o bit de referência, o histórico e a ordem de carga.
 * - Envelhecer as páginas residentes a cada intervalo de envelhecimento.
 * - Escolher a vítima por varredura linear dos frames (números de frames pequenos).
 */
#include "lru_approx.hpp"

#include <cassert>
#include <tuple>
#include <unordered_map>

namespace {

/// Estado de um frame ocupado.
struct Frame {
    uint32_t page;
    bool ref_bit;
    uint32_t history;
    uint64_t load_order;  ///< Posição da carga; menor = carregada há mais tempo.
};

/// Índice do frame vítima: bit de referência desligado, depois menor histórico,
/// depois carregado há mais tempo. O bit vem antes do histórico por ser a informação
/// mais recente — senão a página carregada desde o último envelhecimento, de
/// histórico zerado, seria sempre a próxima vítima.
std::size_t choose_victim(const std::vector<Frame>& frames) {
    std::size_t victim = 0;
    for (std::size_t i = 1; i < frames.size(); ++i) {
        const Frame& a = frames[i];
        const Frame& b = frames[victim];
        if (std::make_tuple(a.ref_bit, a.history, a.load_order) <
            std::make_tuple(b.ref_bit, b.history, b.load_order)) {
            victim = i;
        }
    }
    return victim;
}

/// Envelhece todas as páginas residentes: o histórico desloca à direita, o bit de
/// referência entra como bit mais significativo e é desligado.
void age(std::vector<Frame>& frames, uint32_t history_bits) {
    const uint32_t msb = uint32_t{1} << (history_bits - 1);
    for (Frame& frame : frames) {
        frame.history = (frame.history >> 1) | (frame.ref_bit ? msb : 0);
        frame.ref_bit = false;
    }
}

}  // namespace

uint64_t simulate_lru_approx(const std::vector<uint32_t>& pages, std::size_t frame_count,
                             uint32_t history_bits, uint64_t aging_interval) {
    assert(frame_count >= 1);
    std::vector<Frame> frames;
    std::unordered_map<uint32_t, std::size_t> frame_of;  // página residente → frame
    uint64_t page_faults = 0;
    uint64_t access_count = 0;

    for (uint32_t page : pages) {
        auto it = frame_of.find(page);
        if (it != frame_of.end()) {
            frames[it->second].ref_bit = true;
        } else {
            ++page_faults;
            const Frame loaded{page, true, 0, access_count};
            if (frames.size() < frame_count) {
                frame_of[page] = frames.size();
                frames.push_back(loaded);
            } else {
                const std::size_t victim = choose_victim(frames);
                frame_of.erase(frames[victim].page);
                frame_of[page] = victim;
                frames[victim] = loaded;
            }
        }
        ++access_count;
        if (access_count % aging_interval == 0) {
            age(frames, history_bits);
        }
    }
    return page_faults;
}
