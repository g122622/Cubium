#include "EnchantingTableEntity.hpp"
#include "../../IWorld.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc {
namespace blockentity {

// ========== 构造函数 ==========

EnchantingTableEntity::EnchantingTableEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::EnchantingTable, pos)
    , m_randomSeed(static_cast<i32>(std::hash<BlockPos>{}(pos) % 10000)) {
}

// ========== 方块实体接口 ==========

void EnchantingTableEntity::tick(IWorld& world) {
    MC_UNUSED(world);
    // 附魔台不需要tick更新
}

bool EnchantingTableEntity::load(const nlohmann::json& data) {
    BlockEntity::load(data);

    // 加载自定义名称
    if (data.contains("CustomName")) {
        m_customName = data["CustomName"].get<String>();
    }

    // 附魔力量在加载后重新计算
    return true;
}

void EnchantingTableEntity::save(nlohmann::json& data) const {
    BlockEntity::save(data);

    // 保存自定义名称
    if (!m_customName.empty()) {
        data["CustomName"] = m_customName;
    }
}

std::unique_ptr<BlockEntity> EnchantingTableEntity::clone() const {
    auto entity = std::make_unique<EnchantingTableEntity>(m_pos);
    entity->m_customName = m_customName;
    entity->m_enchantPower = m_enchantPower;
    return entity;
}

// ========== 附魔力量 ==========

void EnchantingTableEntity::recalculateEnchantPower(IWorld& world) {
    m_enchantPower = 0;

    // 检查附魔台周围2格范围内的书架
    // 书架位置：距离附魔台水平距离2格，垂直距离0-1格
    for (i32 dx = -2; dx <= 2; ++dx) {
        for (i32 dz = -2; dz <= 2; ++dz) {
            // 跳过附魔台本身的位置
            if (std::abs(dx) == 0 && std::abs(dz) == 0) {
                continue;
            }

            // 只检查距离为2的位置（对角线距离sqrt(8)不算）
            if (std::abs(dx) == 2 || std::abs(dz) == 2) {
                // 检查两层高度：y+0 和 y+1
                for (i32 dy = 0; dy <= 1; ++dy) {
                    BlockPos bookshelfPos(
                        m_pos.x + dx,
                        m_pos.y + dy,
                        m_pos.z + dz
                    );

                    if (isValidBookshelf(world, bookshelfPos, m_pos)) {
                        m_enchantPower++;
                    }
                }
            }
        }
    }

    // 最大附魔力量为15
    m_enchantPower = std::min(m_enchantPower, 15);
}

bool EnchantingTableEntity::isValidBookshelf(IWorld& world,
                                              const BlockPos& bookshelfPos,
                                              const BlockPos& tablePos) {
    // 检查书架位置是否是书架方块
    const BlockState* bookshelfState = world.getBlockState(
        bookshelfPos.x, bookshelfPos.y, bookshelfPos.z);

    if (bookshelfState == nullptr || bookshelfState->isAir()) {
        return false;
    }

    // 检查是否是实际的书架方块类型
    if (&bookshelfState->getBlock() != VanillaBlocks::BOOKSHELF) {
        return false;
    }

    // 检查书架与附魔台之间的方块是否是空气
    // 中间位置在书架和附魔台之间
    i32 dx = bookshelfPos.x - tablePos.x;
    i32 dz = bookshelfPos.z - tablePos.z;

    // 计算中间位置（书架和附魔台之间的方块）
    // 书架在距离2的位置，中间方块在距离1的位置
    BlockPos middlePos(
        tablePos.x + (dx > 0 ? 1 : (dx < 0 ? -1 : 0)),
        bookshelfPos.y,  // 与书架同一高度
        tablePos.z + (dz > 0 ? 1 : (dz < 0 ? -1 : 0))
    );

    const BlockState* middleState = world.getBlockState(
        middlePos.x, middlePos.y, middlePos.z);

    // 中间必须是空气
    return middleState == nullptr || middleState->isAir();
}

// ========== 自定义名称 ==========

void EnchantingTableEntity::setCustomName(const String& name) {
    m_customName = name;
    setChanged();
}

// ========== 动画 ==========

void EnchantingTableEntity::updateAnimation(IWorld& world, f32 dt) {
    MC_UNUSED(world);

    m_prevBookRotation = m_bookRotation;
    m_prevBookOpen = m_bookOpen;
    m_prevBookPageAngle = m_bookPageAngle;

    m_time += dt;

    // 书本翻开动画
    // 玩家靠近时翻开，离开时合上
    // 简化实现：总是微微打开
    f32 targetOpen = 0.0f;

    // TODO: 检测玩家是否靠近
    // 如果有玩家在附近，设置 targetOpen = 1.0f

    // 平滑过渡
    f32 openSpeed = 0.1f;
    if (m_bookOpen < targetOpen) {
        m_bookOpen = std::min(m_bookOpen + openSpeed * dt * 60.0f, targetOpen);
    } else if (m_bookOpen > targetOpen) {
        m_bookOpen = std::max(m_bookOpen - openSpeed * dt * 60.0f, targetOpen);
    }

    // 书本翻转动画
    // 使用伪随机生成平滑的翻转动画
    f32 flip = std::sin(m_time * 0.5f + static_cast<f32>(m_randomSeed)) * 0.3f;
    m_bookRotation += flip * dt;

    // 书页角度动画
    f32 pageFlip = std::sin(m_time * 0.8f + static_cast<f32>(m_randomSeed) * 0.5f) * 0.5f;
    m_bookPageAngle = pageFlip * m_bookOpen;
}

} // namespace blockentity
} // namespace mc
