#include "world/blockentity/storage/ShulkerBoxEntity.hpp"
#include "world/IWorld.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

// ========== ShulkerBoxEntity 实现 ==========

ShulkerBoxEntity::ShulkerBoxEntity(const BlockPos& pos)
    : LockableBlockEntity(BlockEntityType::ShulkerBox, pos)
    , m_inventory(SHULKER_BOX_SIZE) {
}

ShulkerBoxEntity::~ShulkerBoxEntity() = default;

f32 ShulkerBoxEntity::getProgress(f32 partialTick) const {
    return m_prevProgress + (m_progress - m_prevProgress) * partialTick;
}

bool ShulkerBoxEntity::openContainer(Player* player) {
    if (player == nullptr) {
        return false;
    }

    // 检查锁定状态
    if (isLocked()) {
        // TODO: 检查玩家手持物品是否匹配锁名称
        return false;
    }

    m_openCount++;
    if (m_openCount == 1) {
        m_animationStatus = AnimationStatus::Opening;
    }
    setChanged();
    return true;
}

void ShulkerBoxEntity::closeContainer(Player* player) {
    MC_UNUSED(player);

    if (m_openCount > 0) {
        m_openCount--;
        if (m_openCount == 0) {
            m_animationStatus = AnimationStatus::Closing;
        }
        setChanged();
    }
}

bool ShulkerBoxEntity::canOpen(IWorld& world) const {
    return checkCanOpen(world);
}

void ShulkerBoxEntity::tick(IWorld& world) {
    MC_UNUSED(world);

    // 更新动画
    updateAnimation(0.0f);
}

void ShulkerBoxEntity::updateAnimation(f32 partialTick) {
    MC_UNUSED(partialTick);

    m_prevProgress = m_progress;

    switch (m_animationStatus) {
        case AnimationStatus::Opening:
            m_progress += 0.1f;
            if (m_progress >= 1.0f) {
                m_progress = 1.0f;
                m_animationStatus = AnimationStatus::Opened;
            }
            break;

        case AnimationStatus::Closing:
            m_progress -= 0.1f;
            if (m_progress <= 0.0f) {
                m_progress = 0.0f;
                m_animationStatus = AnimationStatus::Closed;
            }
            break;

        case AnimationStatus::Opened:
            m_progress = 1.0f;
            break;

        case AnimationStatus::Closed:
        default:
            m_progress = 0.0f;
            break;
    }
}

bool ShulkerBoxEntity::checkCanOpen(IWorld& world) const {
    MC_UNUSED(world);
    // TODO: 检查上方是否有实体阻挡
    return true;
}

bool ShulkerBoxEntity::load(const nlohmann::json& data) {
    if (!LockableBlockEntity::load(data)) {
        return false;
    }

    // 加载物品
    if (data.contains("items")) {
        m_inventory.load(data["items"]);
    }

    return true;
}

void ShulkerBoxEntity::save(nlohmann::json& data) const {
    LockableBlockEntity::save(data);

    // 保存物品
    nlohmann::json itemsJson;
    m_inventory.save(itemsJson);
    data["items"] = itemsJson;
}

std::unique_ptr<BlockEntity> ShulkerBoxEntity::clone() const {
    auto clone = std::make_unique<ShulkerBoxEntity>(m_pos);
    clone->m_animationStatus = m_animationStatus;
    clone->m_progress = m_progress;
    clone->m_prevProgress = m_prevProgress;
    clone->m_openCount = m_openCount;
    // TODO: 复制物品
    return clone;
}

} // namespace blockentity
} // namespace mc
