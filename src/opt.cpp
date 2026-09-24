/**
 * Implementação da política OPT em O(n log k).
 *
 * Responsabilidades:
 * - Pré-calcular o próximo uso de cada acesso numa passada de trás para frente.
 * - Manter as páginas residentes ordenadas por próximo uso, para achar a vítima.
 */
#include "opt.hpp"

#include <iterator>
#include <limits>
#include <set>
#include <unordered_map>
#include <utility>

namespace {

/// Próximo uso de uma página que nunca mais é acessada: mais distante que qualquer
/// posição do trace.
constexpr uint64_t kNeverUsedAgain = std::numeric_limits<uint64_t>::max();

/// Para cada acesso, a posição do próximo acesso à mesma página.
/// @param pages Páginas acessadas, em ordem.
/// @return next_use[i] = menor j > i com pages[j] == pages[i], ou kNeverUsedAgain.
std::vector<uint64_t> compute_next_use(const std::vector<uint32_t>& pages) {
    std::vector<uint64_t> next_use(pages.size());
    std::unordered_map<uint32_t, uint64_t> upcoming;
    for (uint64_t i = pages.size(); i-- > 0;) {
        auto it = upcoming.find(pages[i]);
        next_use[i] = it == upcoming.end() ? kNeverUsedAgain : it->second;
        upcoming[pages[i]] = i;
    }
    return next_use;
}

}  // namespace

uint64_t simulate_opt(const std::vector<uint32_t>& pages, std::size_t frame_count) {
    const std::vector<uint64_t> next_use = compute_next_use(pages);

    // Página residente → seu próximo uso; e o mesmo par ordenado por próximo uso.
    // Desempate (só entre páginas que nunca mais são usadas) pelo número da página,
    // o que mantém a simulação determinística sem alterar o número de falhas.
    std::unordered_map<uint32_t, uint64_t> resident;
    std::set<std::pair<uint64_t, uint32_t>> by_next_use;
    uint64_t page_faults = 0;

    for (uint64_t i = 0; i < pages.size(); ++i) {
        const uint32_t page = pages[i];
        auto it = resident.find(page);
        if (it != resident.end()) {
            by_next_use.erase({it->second, page});
            it->second = next_use[i];
            by_next_use.insert({next_use[i], page});
            continue;
        }
        ++page_faults;
        if (resident.size() == frame_count) {
            const auto victim = std::prev(by_next_use.end());
            resident.erase(victim->second);
            by_next_use.erase(victim);
        }
        resident.emplace(page, next_use[i]);
        by_next_use.insert({next_use[i], page});
    }
    return page_faults;
}
