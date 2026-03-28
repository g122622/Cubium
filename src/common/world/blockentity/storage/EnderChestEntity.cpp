#include "world/blockentity/storage/EnderChestEntity.hpp"
#include "entity/Player.hpp"
#include "world/IWorld.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

// ========== EnderChestEntity 实现 ==========

EnderChestEntity::EnderChestEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::EnderChest, pos) {
}

EnderChestEntity::~EnderChestEntity() = default;

bool EnderChestEntity::openContainer(Player* player) {
    if (player == nullptr) {
        return false;
    }

    // TODO: 检查玩家是否有末影箱物品栏
    // TODO: 打开末影箱GUI

    m_openCount++;
    setChanged();
    return true;
}

void EnderChestEntity::closeContainer(Player* player) {
    MC_UNUSED(player);

    if (m_openCount > 0) {
        m_openCount--;
        setChanged();
    }
}

bool EnderChestEntity::canPlayerAccess(Player* player) const {
    MC_UNUSED(player);
    // 末影箱对所有玩家可访问，但每个玩家看到的是自己的物品
    return true;
}

void EnderChestEntity::updateLidAnimation(f32 partialTick) {
    MC_UNUSED(partialTick);

    // 更新盖子动画
    m_prevLidAngle = m_lidAngle;

    if (m_openCount > 0 && m_lidAngle < 1.0f) {
        m_lidAngle += 0.1f;
        if (m_lidAngle > 1.0f) {
            m_lidAngle = 1.0f;
        }
    } else if (m_openCount == 0 && m_lidAngle > 0.0f) {
        m_lidAngle -= 0.1f;
        if (m_lidAngle < 0.0f) {
            m_lidAngle = 0.0f;
        }
    }
}

void EnderChestEntity::tick(IWorld& world) {
    m_ticksSinceSync++;

    // 每10tick同步一次打开状态
    if (m_ticksSinceSync >= 10) {
        m_ticksSinceSync = 0;
        // TODO: 同步打开状态到客户端
    }

    // 更新动画
    updateLidAnimation(0.0f);
}

bool EnderChestEntity::load(const nlohmann::json& data) {
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 末影箱不存储物品，物品在玩家数据中
    if (data.contains("open_count")) {
        m_openCount = data["open_count"].get<i32>();
    }

    return true;
}

void EnderChestEntity::save(nlohmann::json& data) const {
    BlockEntity::save(data);

    data["open_count"] = m_openCount;
}

std::unique_ptr<BlockEntity> EnderChestEntity::clone() const {
    auto clone = std::make_unique<EnderChestEntity>(m_pos);
    clone->m_openCount = m_openCount;
    clone->m_lidAngle = m_lidAngle;
    clone->m_prevLidAngle = m_prevLidAngle;
    return clone;
}

} // namespace blockentity
} // namespace mc
