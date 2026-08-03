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

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/ChunkSection.hpp"
#include <cmath>
#include <memory>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

// ========== 构造函数 ==========

EndGatewayEntity::EndGatewayEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::EndGateway, pos)
{}

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
            auto entities = world.getEntitiesInAABB(AxisAlignedBB::fromBlock(m_pos.x, m_pos.y, m_pos.z));

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
        if (exitPortal.is_object() && exitPortal.contains("X") && exitPortal.contains("Y") &&
            exitPortal.contains("Z")) {
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
        _generateExitPortal(world);
    }

    // 如果有出口位置，执行传送
    if (m_exitPortal.has_value()) {
        BlockPos targetPos = m_exactTeleport ? m_exitPortal.value() : _findExitPosition(world);

        // 执行传送
        // 传送到目标位置的中心
        entity.attemptTeleport(static_cast<f64>(targetPos.x) + 0.5,
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
    return 1.0f -
        math::clamp(
            (static_cast<f32>(m_teleportCooldown) - partialTicks) / static_cast<f32>(TRIGGER_COOLDOWN), 0.0f, 1.0f);
}

void EndGatewayEntity::triggerCooldown(IWorld& world)
{
    if (!world.isClientSide()) {
        m_teleportCooldown = TRIGGER_COOLDOWN;
        // 通过 blockEvent 同步冷却动画到客户端
        const BlockState* state = world.getBlockState(m_pos);
        if (state != nullptr) {
            world.blockEvent(m_pos, state->getBlock(), 1, 0);
        }
        setChanged();
    }
}

bool EndGatewayEntity::triggerEvent(i32 id, i32 type)
{
    if (id == 1) {
        m_teleportCooldown = TRIGGER_COOLDOWN;
        return true;
    }
    return false;
}

// ========== 私有方法 ==========

BlockPos EndGatewayEntity::_findExitPosition(IWorld& world) const
{
    if (!m_exitPortal.has_value()) {
        return m_pos.up();
    }

    // 在出口传送门上方寻找安全位置
    BlockPos searchCenter = m_exitPortal.value().up(2);
    BlockPos highestBlock = _findHighestBlock(world, searchCenter, 5, false);

    // 返回最高方块上方
    return highestBlock.up();
}

bool EndGatewayEntity::_isChunkEmpty(const world::chunk::ChunkData* chunk)
{
    // 对应 MC Java 的 TheEndGatewayBlockEntity.isChunkEmpty：
    // 区块中所有区段均为空（getHighestFilledSectionIndex == -1）时视为空区块
    if (chunk == nullptr) {
        return true;
    }

    for (i32 sectionIdx = 0; sectionIdx < world::CHUNK_SECTIONS; ++sectionIdx) {
        const world::chunk::ChunkSection* section = chunk->getSection(sectionIdx);
        if (section != nullptr && !section->isEmpty()) {
            return false;
        }
    }
    return true;
}

void EndGatewayEntity::_generateExitPortal(IWorld& world)
{
    // 从主岛向外约 1024 格生成出口传送门

    // 计算方向向量（从原点指向当前位置）并归一化
    f64 length = std::sqrt(static_cast<f64>(m_pos.x * m_pos.x + m_pos.z * m_pos.z));
    if (length < 1.0) {
        length = 1.0;
    }

    f64 dirX = static_cast<f64>(m_pos.x) / length;
    f64 dirZ = static_cast<f64>(m_pos.z) / length;

    // 沿方向搜索合适的区块
    // 先沿方向前进 1024 格，然后跳过非空区块（回退），再跳过空区块（前进）
    // 与 MC Java 的 TheEndGatewayBlockEntity.findExitPortalXZPosTentative 一致
    // 使用 getOrLoadChunk 同步加载区块以判断是否为空，完整复刻原版行为
    f64 vecX = dirX * 1024.0;
    f64 vecZ = dirZ * 1024.0;

    // 回退跳过非空区块（主岛区域）
    for (i32 i = 0; i < 16; ++i) {
        i32 chunkX = world::toChunkCoord(static_cast<i32>(std::floor(vecX)));
        i32 chunkZ = world::toChunkCoord(static_cast<i32>(std::floor(vecZ)));
        const world::chunk::ChunkData* chunk = world.getOrLoadChunk(chunkX, chunkZ);

        if (!_isChunkEmpty(chunk)) {
            // 非空区块，继续回退
            vecX -= dirX * static_cast<f64>(world::CHUNK_WIDTH);
            vecZ -= dirZ * static_cast<f64>(world::CHUNK_WIDTH);
        } else {
            break;
        }
    }

    // 前进跳过空区块（寻找外岛边缘）
    for (i32 i = 0; i < 16; ++i) {
        i32 chunkX = world::toChunkCoord(static_cast<i32>(std::floor(vecX)));
        i32 chunkZ = world::toChunkCoord(static_cast<i32>(std::floor(vecZ)));
        const world::chunk::ChunkData* chunk = world.getOrLoadChunk(chunkX, chunkZ);

        if (_isChunkEmpty(chunk)) {
            // 空区块，继续前进
            vecX += dirX * static_cast<f64>(world::CHUNK_WIDTH);
            vecZ += dirZ * static_cast<f64>(world::CHUNK_WIDTH);
        } else {
            break;
        }
    }

    i32 targetX = static_cast<i32>(vecX + 0.5);
    i32 targetZ = static_cast<i32>(vecZ + 0.5);

    // 末地外岛的默认生成高度为75，与 MC Java 的 TheEndGatewayBlockEntity.findOrCreateValidTeleportPos 一致
    BlockPos targetPos(targetX, 75, targetZ);

    // 查找最高方块
    BlockPos groundPos = _findHighestBlock(world, targetPos, 16, true);

    // 在地面之上 10 格放置折跃门
    m_exitPortal = groundPos.up(10);

    // 创建折跃门结构
    createGatewayStructure(world, m_exitPortal.value());

    // 在出口折跃门的方块实体上设置返回位置，指向当前折跃门
    // 与 MC Java 的 TheEndGatewayBlockEntity.getPortalPosition 一致
    const BlockState* exitState = world.getBlockState(m_exitPortal.value());
    if (exitState != nullptr && &exitState->getBlock() == VanillaBlocks::END_GATEWAY) {
        BlockEntity* exitBe = world.getBlockEntity(m_exitPortal.value());
        if (exitBe != nullptr && exitBe->getType() == BlockEntityType::EndGateway) {
            auto* exitGateway = static_cast<EndGatewayEntity*>(exitBe);
            exitGateway->setExitPortal(m_pos, false);
        }
    }

    setChanged();
}

void EndGatewayEntity::createGatewayStructure(IWorld& world, const BlockPos& pos)
{
    // 折跃门结构：3x5x3 的基岩框架，中心为折跃门方块
    // 结构以 pos 为中心，范围从 pos + (-1, -2, -1) 到 pos + (1, 2, 1)
    //
    // 顶/底盖层（dy = ±2）：仅中心列为基岩
    //   . . .
    //   . B .
    //   . . .
    //
    // 十字臂层（dy = ±1）：十字形基岩框架
    //   . B .
    //   B B B
    //   . B .
    //
    // 中心层（dy = 0）：中心为折跃门方块，其余为空气
    //   . . .
    //   . G .
    //   . . .

    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    const BlockState* endGateway = VanillaBlocks::getState(VanillaBlocks::END_GATEWAY);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    if (bedrock == nullptr || endGateway == nullptr || air == nullptr) {
        return;
    }

    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dy = -2; dy <= 2; ++dy) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                BlockPos blockPos(pos.x + dx, pos.y + dy, pos.z + dz);
                bool sameX = (dx == 0);
                bool sameY = (dy == 0);
                bool sameZ = (dz == 0);
                bool isTopBottomCap = (dy == 2 || dy == -2);

                if (sameX && sameY && sameZ) {
                    // 中心位置：末地折跃门方块
                    world.setBlockState(blockPos, endGateway);
                } else if (sameY) {
                    // 与中心同层但不在中心列：空气（通道）
                    world.setBlockState(blockPos, air);
                } else if (isTopBottomCap && sameX && sameZ) {
                    // 顶/底盖的中心列：基岩
                    world.setBlockState(blockPos, bedrock);
                } else if ((sameX || sameZ) && !isTopBottomCap) {
                    // 侧面十字臂（非顶底盖）：基岩
                    world.setBlockState(blockPos, bedrock);
                } else {
                    // 四角（非中心Y 且非十字臂）：空气
                    world.setBlockState(blockPos, air);
                }
            }
        }
    }
}

BlockPos EndGatewayEntity::_findHighestBlock(IWorld& world, const BlockPos& center, i32 radius, bool allowBedrock)
{
    BlockPos result = center;
    i32 highestY = -1;

    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            // 跳过中心（除非允许基岩），与 MC Java 的 findTallestBlock 一致
            if (dx == 0 && dz == 0 && !allowBedrock) {
                continue;
            }

            // 从顶部向下搜索，只记录比当前最高点更高的方块
            for (i32 y = world::MAX_BUILD_HEIGHT - 1; y > highestY; --y) {
                BlockPos checkPos(center.x + dx, y, center.z + dz);
                const BlockState* state = world.getBlockState(checkPos);

                if (state != nullptr && !state->isAir()) {
                    const Block& block = state->getBlock();
                    // 与 MC Java 的 isCollisionShapeFullBlock 一致：碰撞形状为完整方块
                    const CollisionShape& shape = block.getCollisionShape(*state);
                    if (shape.isFullBlock()) {
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

    // 如果没找到任何方块，返回中心位置（与 MC Java 的回退逻辑一致）
    return result;
}

} // namespace blockentity
} // namespace mc
