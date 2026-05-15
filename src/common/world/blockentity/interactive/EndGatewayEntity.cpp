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

#include "EndGatewayEntity.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/VanillaBlocks.hpp"

namespace mc {
namespace blockentity {

// ========== 构造函数 ==========

EndGatewayEntity::EndGatewayEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::EndGateway, pos)
{
}

// ========== 方块实体接口 ==========

void EndGatewayEntity::tick(IWorld& world)
{
    bool wasSpawning = isSpawning();
    bool wasCoolingDown = isCoolingDown();

    // 增加年龄
    ++m_age;

    // 冷却递减
    if (isCoolingDown()) {
        --m_teleportCooldown;
    }

    // 服务端逻辑：检测实体并传送
    if (!world.isClientSide()) {
        // 在冷却期间不执行传送
        if (!isCoolingDown()) {
            // 获取方块碰撞箱内的实体
            auto entities = world.getEntitiesInAABB(
                AxisAlignedBB::fromBlock(m_pos.x, m_pos.y, m_pos.z));

            if (!entities.empty()) {
                // 随机选择一个实体传送
                math::Random rng(static_cast<u64>(m_age));
                Entity& entity = *entities[rng.nextInt(static_cast<i32>(entities.size()))];

                // 传送实体
                teleportEntity(world, entity);
            }
        }

        // 每 2400 tick 自动触发冷却（用于外岛折跃门）
        if (m_age % AUTO_COOLDOWN_INTERVAL == 0L) {
            triggerCooldown(world);
        }
    }

    // 状态变化时标记修改
    if (wasSpawning != isSpawning() || wasCoolingDown != isCoolingDown()) {
        setChanged();
    }
}

bool EndGatewayEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    if (data.contains("Age") && data["Age"].is_number()) {
        m_age = data["Age"].get<i64>();
    }

    if (data.contains("TeleportCooldown") && data["TeleportCooldown"].is_number()) {
        m_teleportCooldown = data["TeleportCooldown"].get<i32>();
    }

    if (data.contains("ExitPortal")) {
        const auto& exitPortal = data["ExitPortal"];
        if (exitPortal.is_object() &&
            exitPortal.contains("X") && exitPortal.contains("Y") && exitPortal.contains("Z")) {
            i32 x = exitPortal["X"].get<i32>();
            i32 y = exitPortal["Y"].get<i32>();
            i32 z = exitPortal["Z"].get<i32>();
            m_exitPortal = BlockPos(x, y, z);
        }
    }

    if (data.contains("ExactTeleport") && data["ExactTeleport"].is_boolean()) {
        m_exactTeleport = data["ExactTeleport"].get<bool>();
    }

    return true;
}

void EndGatewayEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    data["Age"] = m_age;

    if (m_teleportCooldown > 0) {
        data["TeleportCooldown"] = m_teleportCooldown;
    }

    if (m_exitPortal.has_value()) {
        nlohmann::json exitPortal;
        exitPortal["X"] = m_exitPortal->x;
        exitPortal["Y"] = m_exitPortal->y;
        exitPortal["Z"] = m_exitPortal->z;
        data["ExitPortal"] = std::move(exitPortal);
    }

    if (m_exactTeleport) {
        data["ExactTeleport"] = m_exactTeleport;
    }
}

std::unique_ptr<BlockEntity> EndGatewayEntity::clone() const
{
    auto cloned = std::make_unique<EndGatewayEntity>(m_pos);
    cloned->m_age = m_age;
    cloned->m_teleportCooldown = m_teleportCooldown;
    cloned->m_exitPortal = m_exitPortal;
    cloned->m_exactTeleport = m_exactTeleport;
    return cloned;
}

// ========== 传送功能 ==========

void EndGatewayEntity::teleportEntity(IWorld& world, Entity& entity)
{
    // 检查是否在服务端
    if (world.isClientSide()) {
        return;
    }

    // 检查冷却
    if (isCoolingDown()) {
        return;
    }

    // 设置传送冷却
    m_teleportCooldown = TELEPORT_COOLDOWN;

    // 如果没有出口位置且在末地主岛，生成出口传送门
    if (!m_exitPortal.has_value()) {
        // 检查是否在末地维度
        // MC 1.16.5: 只在主岛折跃门生成出口传送门
        generateExitPortal(world);
    }

    // 如果有出口位置，执行传送
    if (m_exitPortal.has_value()) {
        BlockPos targetPos = m_exactTeleport ? m_exitPortal.value() : findExitPosition(world);

        // 执行传送
        // 传送到目标位置的中心
        entity.attemptTeleport(
            static_cast<f64>(targetPos.x) + 0.5,
            static_cast<f64>(targetPos.y),
            static_cast<f64>(targetPos.z) + 0.5,
            true); // 播放传送效果
    }

    // 触发冷却
    triggerCooldown(world);
}

void EndGatewayEntity::setExitPortal(const BlockPos& exitPos, bool exactTeleport)
{
    m_exitPortal = exitPos;
    m_exactTeleport = exactTeleport;
    setChanged();
}

// ========== 状态查询 ==========

f32 EndGatewayEntity::getSpawnPercent(f32 partialTicks) const
{
    return math::clamp(static_cast<f32>(m_age + partialTicks) / static_cast<f32>(SPAWN_DURATION), 0.0f, 1.0f);
}

f32 EndGatewayEntity::getCooldownPercent(f32 partialTicks) const
{
    return 1.0f - math::clamp((static_cast<f32>(m_teleportCooldown) - partialTicks) / static_cast<f32>(TRIGGER_COOLDOWN), 0.0f, 1.0f);
}

void EndGatewayEntity::triggerCooldown(IWorld& world)
{
    if (!world.isClientSide()) {
        m_teleportCooldown = TRIGGER_COOLDOWN;
        // MC 1.16.5: world.addBlockEvent(pos, block, 1, 0)
        // 这会通知客户端播放冷却动画
        setChanged();
    }
}

bool EndGatewayEntity::receiveClientEvent(i32 id, i32 type)
{
    if (id == 1) {
        m_teleportCooldown = TRIGGER_COOLDOWN;
        return true;
    }
    return false;
}

// ========== 私有方法 ==========

BlockPos EndGatewayEntity::findExitPosition(IWorld& world) const
{
    if (!m_exitPortal.has_value()) {
        return m_pos.up();
    }

    // 在出口传送门上方寻找安全位置
    BlockPos searchCenter = m_exitPortal.value().up(2);
    BlockPos highestBlock = findHighestBlock(world, searchCenter, 5, false);

    // 返回最高方块上方
    return highestBlock.up();
}

void EndGatewayEntity::generateExitPortal(IWorld& world)
{
    // MC 1.16.5: 从主岛向外约 1024 格生成出口传送门
    // 这是一个简化的实现
    // 使用位置坐标作为随机种子
    math::Random rng(static_cast<u64>(static_cast<i64>(m_pos.x) * 3129871LL + static_cast<i64>(m_pos.z) * 116129781LL));

    // 计算方向向量（从原点指向当前位置）并归一化
    f64 length = std::sqrt(static_cast<f64>(m_pos.x * m_pos.x + m_pos.z * m_pos.z));
    if (length < 1.0) {
        length = 1.0;
    }

    f64 dirX = static_cast<f64>(m_pos.x) / length;
    f64 dirZ = static_cast<f64>(m_pos.z) / length;

    // 缩放到 1024 格距离
    f64 distance = 1024.0 + rng.nextDouble() * 256.0;
    i32 targetX = static_cast<i32>(dirX * distance);
    i32 targetZ = static_cast<i32>(dirZ * distance);

    // 寻找合适的高度
    BlockPos targetPos(targetX, 75, targetZ);

    // 查找最高方块
    BlockPos groundPos = findHighestBlock(world, targetPos, 16, true);

    // 在地面之上 10 格放置折跃门
    m_exitPortal = groundPos.up(10);

    // 创建折跃门结构
    createGatewayStructure(world, m_exitPortal.value());

    setChanged();
}

void EndGatewayEntity::createGatewayStructure(IWorld& world, const BlockPos& pos)
{
    // MC 1.16.5: 折跃门结构是一个单独的折跃门方块
    // 周围是基岩框架

    // 获取方块
    Block* bedrockBlock = VanillaBlocks::BEDROCK;
    Block* endGatewayBlock = VanillaBlocks::END_GATEWAY;

    if (bedrockBlock == nullptr || endGatewayBlock == nullptr) {
        return;
    }

    // 获取默认方块状态
    const BlockState& bedrock = bedrockBlock->defaultState();
    const BlockState& endGateway = endGatewayBlock->defaultState();

    // 简化版：只放置折跃门方块本身
    // 完整实现应该在周围放置基岩框架
    world.setBlockState(pos, &endGateway);
}

BlockPos EndGatewayEntity::findHighestBlock(IWorld& world, const BlockPos& center, i32 radius, bool allowBedrock)
{
    BlockPos result = center;
    i32 highestY = -1;

    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            // 跳过中心（除非允许基岩）
            if (dx == 0 && dz == 0 && !allowBedrock) {
                continue;
            }

            // 从顶部向下搜索
            for (i32 y = 255; y > highestY; --y) {
                BlockPos checkPos(center.x + dx, y, center.z + dz);
                const BlockState* state = world.getBlockState(checkPos);

                if (state != nullptr) {
                    const Block& block = state->getBlock();
                    // 检查是否有碰撞箱（非透明方块）
                    if (!block.isAir(*state)) {
                        const CollisionShape& shape = block.getCollisionShape(*state);
                        if (!shape.isEmpty()) {
                            // 如果不允许基岩，跳过基岩
                            if (!allowBedrock && &block == VanillaBlocks::BEDROCK) {
                                continue;
                            }

                            highestY = y;
                            result = checkPos;
                            break;
                        }
                    }
                }
            }
        }
    }

    return result;
}

} // namespace blockentity
} // namespace mc
