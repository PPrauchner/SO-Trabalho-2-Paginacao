/**
 * Implementação da política FIFO.
 *
 * Responsabilidades:
 * - Manter a ordem de carga das páginas residentes e o conjunto delas.
 */
#include "fifo.hpp"

#include <queue>
#include <unordered_set>

uint64_t simulate_fifo(const std::vector<uint32_t>& pages, std::size_t frame_count) {
    std::queue<uint32_t> load_order;
    std::unordered_set<uint32_t> resident;
    uint64_t page_faults = 0;

    for (uint32_t page : pages) {
        if (resident.count(page) != 0) {
            continue;
        }
        ++page_faults;
        if (resident.size() == frame_count) {
            resident.erase(load_order.front());
            load_order.pop();
        }
        resident.insert(page);
        load_order.push(page);
    }
    return page_faults;
}
