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
    return m_depth < MAX_DEPTH;
}

void RedstoneContext::pushDepth() {
    ++m_depth;
}

void RedstoneContext::popDepth() {
    if (m_depth > 0) {
        --m_depth;
    }
}

void RedstoneContext::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_updatingPositions.clear();
    m_depth = 0;
}

size_t RedstoneContext::updatingCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_updatingPositions.size();
}

} // namespace redstone
} // namespace world
} // namespace mc
