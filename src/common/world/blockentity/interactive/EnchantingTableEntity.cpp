/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "EnchantingTableEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

namespace {

/// 玩家触发附魔台翻书动画的水平半径（方块）。
constexpr f32 ENCHANTING_TABLE_PLAYER_RANGE = 3.0f;
/// 翻书动画判定中使用的平方半径，避免每帧开方。
constexpr f32 ENCHANTING_TABLE_PLAYER_RANGE_SQ = ENCHANTING_TABLE_PLAYER_RANGE * ENCHANTING_TABLE_PLAYER_RANGE;

/**
 * @brief 检测附魔台附近是否存在玩家。
 *
 * @param world 世界接口。
 * @param tablePos 附魔台位置。
 * @return true 表示半径内存在玩家，可驱动翻书动画展开。
 */
[[nodiscard]] bool hasNearbyPlayer(IWorld& world, const BlockPos& tablePos)
{
    const Vector3 center(
        static_cast<f32>(tablePos.x) + 0.5f, static_cast<f32>(tablePos.y) + 0.5f, static_cast<f32>(tablePos.z) + 0.5f);

    const std::vector<Entity*> nearbyEntities =
        world.getEntitiesInRange(center, ENCHANTING_TABLE_PLAYER_RANGE, nullptr);

    for (Entity* entity : nearbyEntities) {
        if (dynamic_cast<Player*>(entity) != nullptr) {
            const Vector3 playerPos = entity->position();
            const f32 dx = playerPos.x - center.x;
            const f32 dz = playerPos.z - center.z;
            const f32 horizontalDistSq = dx * dx + dz * dz;
            if (horizontalDistSq <= ENCHANTING_TABLE_PLAYER_RANGE_SQ) {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief 初始化附魔台书架偏移量列表。
 *
 * 生成附魔台周围2格范围内的所有候选书架位置偏移量。
 * 对应 MC 的 EnchantingTableBlock.BOOKSHELF_OFFSETS：
 * - x 范围 [-2, 2]，y 范围 [0, 1]，z 范围 [-2, 2]
 * - 仅保留 |x|==2 或 |z|==2 的位置（外圈，排除内圈3x3区域）
 * - 共30个偏移位置
 *
 * @return 书架候选偏移量列表
 */
[[nodiscard]] std::vector<BlockPos> initBookshelfOffsets()
{
    std::vector<BlockPos> offsets;
    offsets.reserve(30);
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 y = 0; y <= 1; ++y) {
            for (i32 z = -2; z <= 2; ++z) {
                if (std::abs(x) == 2 || std::abs(z) == 2) {
                    offsets.emplace_back(x, y, z);
                }
            }
        }
    }
    return offsets;
}

/// 附魔台周围候选书架位置的偏移量列表（相对于附魔台位置），共30个位置
static const std::vector<BlockPos> BOOKSHELF_OFFSETS = initBookshelfOffsets();

} // namespace

// ========== 构造函数 ==========

EnchantingTableEntity::EnchantingTableEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::EnchantingTable, pos)
    , m_randomSeed(static_cast<i32>(std::hash<BlockPos>{}(pos) % 10000))
{}

// ========== 方块实体接口 ==========

void EnchantingTableEntity::tick(IWorld& world)
{
    MC_UNUSED(world);
    // 附魔台不需要 tick 更新
}

bool EnchantingTableEntity::load(const nlohmann::json& data)
{
    BlockEntity::load(data);

    // 加载自定义名称
    if (data.contains("CustomName")) {
        m_customName = data["CustomName"].get<std::string>();
    }

    // 附魔力量在加载后重新计算
    return true;
}

void EnchantingTableEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    // 保存自定义名称
    if (!m_customName.empty()) {
        data["CustomName"] = m_customName;
    }
}

std::unique_ptr<BlockEntity> EnchantingTableEntity::clone() const
{
    auto entity = std::make_unique<EnchantingTableEntity>(m_pos);
    entity->m_customName = m_customName;
    entity->m_enchantPower = m_enchantPower;
    return entity;
}

// ========== 附魔力量 ==========

void EnchantingTableEntity::recalculateEnchantPower(IWorld& world)
{
    i32 power = 0;

    // 遍历所有候选书架位置
    // 对应 MC 的 EnchantingTableBlock.BOOKSHELF_OFFSETS 逻辑
    for (const BlockPos& offset : BOOKSHELF_OFFSETS) {
        if (isValidBookshelf(world, m_pos, offset)) {
            power++;
        }
    }

    // 最大附魔力量为15
    m_enchantPower = std::min(power, 15);

    // 标记方块实体已更改
    setChanged();
}

/*static*/ bool EnchantingTableEntity::isValidBookshelf(IWorld& world, const BlockPos& tablePos, const BlockPos& offset)
{
    // 书架位置：附魔台位置 + 偏移量
    BlockPos bookshelfPos = tablePos + offset;

    // 条件1：书架位置必须是附魔力量提供者（默认为书架）
    const BlockState* bookshelfState = world.getBlockState(bookshelfPos.x, bookshelfPos.y, bookshelfPos.z);
    if (bookshelfState == nullptr || !BlockTags::ENCHANTMENT_POWER_PROVIDER().contains(*bookshelfState)) {
        return false;
    }

    // 条件2：书架与附魔台之间的中间方块必须是可替换的（空气、草等）
    // 中间位置 = 附魔台位置 + (offset.x/2, offset.y, offset.z/2)
    // 由于offset.x和offset.z只能是-2, -1, 0, 1, 2，且只有|x|==2或|z|==2时才会到达这里
    // 整数除法：-2/2=-1, -1/2=0, 0/2=0, 1/2=0, 2/2=1
    BlockPos middlePos(tablePos.x + offset.x / 2, tablePos.y + offset.y, tablePos.z + offset.z / 2);

    const BlockState* middleState = world.getBlockState(middlePos.x, middlePos.y, middlePos.z);

    // 未加载的区块视为空气，不阻挡附魔力量
    if (middleState == nullptr) {
        return true;
    }

    // 使用canBeReplaced()判断中间方块是否可被替换（对应MC的ENCHANTMENT_POWER_TRANSMITTER标签）
    return middleState->canBeReplaced();
}

// ========== 自定义名称 ==========

void EnchantingTableEntity::setCustomName(const std::string& name)
{
    m_customName = name;
    setChanged();
}

// ========== 动画 ==========

void EnchantingTableEntity::updateAnimation(IWorld& world, f32 dt)
{
    m_prevBookRotation = m_bookRotation;
    m_prevBookOpen = m_bookOpen;
    m_prevBookPageAngle = m_bookPageAngle;

    m_time += dt;

    // 书本翻开动画
    // 玩家靠近时翻开，离开时合上
    const f32 targetOpen = hasNearbyPlayer(world, m_pos) ? 1.0f : 0.0f;

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
