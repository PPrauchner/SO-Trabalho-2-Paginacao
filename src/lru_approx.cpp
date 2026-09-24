/**
 * Implementação da política LRU aproximado.
 *
 * Responsabilidades:
 * - Manter, por frame, a página, o bit de referência, o histórico e a ordem de carga.
 * - Envelhecer as páginas residentes a cada intervalo de envelhecimento.
 * - Escolher a vítima por varredura linear dos frames (números de frames pequenos).
 */
#include "lru_approx.hpp"

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

/// Índice do frame vítima: menor histórico, depois bit de referência desligado,
/// depois carregado há mais tempo.
std::size_t choose_victim(const std::vector<Frame>& frames) {
    std::size_t victim = 0;
    for (std::size_t i = 1; i < frames.size(); ++i) {
        const Frame& a = frames[i];
        const Frame& b = frames[victim];
        if (std::make_tuple(a.history, a.ref_bit, a.load_order) <
            std::make_tuple(b.history, b.ref_bit, b.load_order)) {
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
    std::vector<Frame> frames;
    frames.reserve(frame_count);
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
