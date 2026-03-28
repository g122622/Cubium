#include "ComparatorEntity.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

ComparatorEntity::ComparatorEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Comparator, pos)
    , m_outputSignal(0) {
}

bool ComparatorEntity::load(const nlohmann::json& data) {
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 加载输出信号强度
    if (data.contains("OutputSignal") && data["OutputSignal"].is_number()) {
        m_outputSignal = data["OutputSignal"].get<i32>();
        // 验证范围
        m_outputSignal = std::clamp(m_outputSignal, 0, 15);
    }

    return true;
}

void ComparatorEntity::save(nlohmann::json& data) const {
    BlockEntity::save(data);

    // 保存输出信号强度
    data["OutputSignal"] = m_outputSignal;
}

std::unique_ptr<BlockEntity> ComparatorEntity::clone() const {
    auto entity = std::make_unique<ComparatorEntity>(m_pos);
    entity->m_outputSignal = m_outputSignal;
    return entity;
}

void ComparatorEntity::setOutputSignal(i32 signal) {
    MC_ASSERT(signal >= 0 && signal <= 15);
    m_outputSignal = signal;
    setChanged();
}

} // namespace blockentity
} // namespace mc
