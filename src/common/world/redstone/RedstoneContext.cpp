#include "RedstoneContext.hpp"

namespace mc {
namespace world {
namespace redstone {

bool RedstoneContext::isUpdating(const BlockPos& pos) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_updatingPositions.count(pos) > 0;
}

void RedstoneContext::beginUpdate(const BlockPos& pos) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_updatingPositions.insert(pos);
}

void RedstoneContext::endUpdate(const BlockPos& pos) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_updatingPositions.erase(pos);
}

bool RedstoneContext::canPushDepth() const {
    return m_depth.load(std::memory_order_relaxed) < MAX_DEPTH;
}

void RedstoneContext::pushDepth() {
    m_depth.fetch_add(1, std::memory_order_relaxed);
}

void RedstoneContext::popDepth() {
    i32 current = m_depth.load(std::memory_order_relaxed);
    while (current > 0) {
        if (m_depth.compare_exchange_weak(current, current - 1, std::memory_order_relaxed)) {
            break;
        }
    }
}

void RedstoneContext::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_updatingPositions.clear();
    m_depth.store(0, std::memory_order_relaxed);
}

size_t RedstoneContext::updatingCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_updatingPositions.size();
}

} // namespace redstone
} // namespace world
} // namespace mc
