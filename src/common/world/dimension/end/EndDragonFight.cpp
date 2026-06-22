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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "EndDragonFight.hpp"

#include "common/util/math/MathConstants.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/interactive/EndGatewayEntity.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/dimension/teleport/Teleporter.hpp"

#include <algorithm>

#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// Data 序列化
// ============================================================================

EndDragonFight::Data EndDragonFight::Data::fromJson(const nlohmann::json& json)
{
    Data data;
    data.needsStateScanning = json.value("NeedsStateScanning", true);
    data.dragonKilled = json.value("DragonKilled", false);
    data.previouslyKilled = json.value("PreviouslyKilled", false);

    if (json.contains("Gateways") && json["Gateways"].is_array()) {
        std::vector<i32> gateways;
        gateways.reserve(json["Gateways"].size());
        for (const auto& val : json["Gateways"]) {
            gateways.push_back(val.get<i32>());
        }
        data.gateways = std::move(gateways);
    }
    // nullopt 表示首次初始化，需要从世界种子生成折跃门列表

    return data;
}

[[nodiscard]] nlohmann::json EndDragonFight::Data::toJson() const
{
    nlohmann::json json;
    json["NeedsStateScanning"] = needsStateScanning;
    json["DragonKilled"] = dragonKilled;
    json["PreviouslyKilled"] = previouslyKilled;

    if (gateways.has_value()) {
        json["Gateways"] = *gateways;
    }

    return json;
}

// ============================================================================
// 构造函数
// ============================================================================

EndDragonFight::EndDragonFight(u64 worldSeed, const std::optional<Data>& data)
    : m_worldSeed(worldSeed)
{
    if (data.has_value()) {
        _loadData(*data);
    } else {
        // 新世界：初始化折跃门列表
        m_gateways.reserve(GATEWAY_COUNT);
        for (i32 i = 0; i < GATEWAY_COUNT; ++i) {
            m_gateways.push_back(i);
        }

        // 使用世界种子创建随机数生成器并打乱
        math::Random rng(worldSeed);
        rng.shuffle(m_gateways);

        // 新世界首次创建，不需要扫描旧世界状态
        m_needsStateScanning = false;
        m_dragonKilled = false;
        m_previouslyKilled = false;
    }
}

// ============================================================================
// 核心逻辑
// ============================================================================

void EndDragonFight::tick(IWorld& world)
{
    // 状态扫描：首次加载旧存档时检查出口传送门
    if (m_needsStateScanning) {
        if (_isArenaLoaded(world)) {
            _scanState(world);
            m_needsStateScanning = false;
        }
    }
}

void EndDragonFight::setDragonKilled(IWorld& world)
{
    // 1. 创建激活态出口传送门（讲台）
    EndTeleporter::createExitPortal(world, BlockPos(0, 0, 0), true);

    // 2. 生成一个末地折跃门（如果还有剩余）
    _spawnNewGateway(world);

    // 3. 首次击杀时在祭坛顶部放置龙蛋
    if (!m_previouslyKilled) {
        _placeDragonEgg(world);
    }

    // 4. 更新状态标志
    m_previouslyKilled = true;
    m_dragonKilled = true;
}

// ============================================================================
// 数据保存
// ============================================================================

EndDragonFight::Data EndDragonFight::saveData() const
{
    Data data;
    data.needsStateScanning = m_needsStateScanning;
    data.dragonKilled = m_dragonKilled;
    data.previouslyKilled = m_previouslyKilled;
    // gateways 始终保存（即使为空列表），区别于加载时的 nullopt
    data.gateways = m_gateways;
    return data;
}

// ============================================================================
// 私有方法
// ============================================================================

void EndDragonFight::_loadData(const Data& data)
{
    m_needsStateScanning = data.needsStateScanning;
    m_dragonKilled = data.dragonKilled;
    m_previouslyKilled = data.previouslyKilled;

    if (data.gateways.has_value()) {
        // 从存档恢复折跃门列表
        m_gateways = *data.gateways;
    } else {
        // 旧存档没有折跃门数据，从世界种子重新生成
        m_gateways.reserve(GATEWAY_COUNT);
        for (i32 i = 0; i < GATEWAY_COUNT; ++i) {
            m_gateways.push_back(i);
        }
        math::Random rng(m_worldSeed);
        rng.shuffle(m_gateways);
    }
}

void EndDragonFight::_scanState(IWorld& world)
{
    spdlog::info("EndDragonFight: Scanning for legacy world dragon fight state...");

    const bool hasActivePortal = _hasActiveExitPortal(world);

    if (hasActivePortal) {
        spdlog::info("EndDragonFight: Found active exit portal - dragon has been killed before.");
        m_previouslyKilled = true;
    } else {
        spdlog::info("EndDragonFight: No active exit portal found - dragon has not been killed yet.");
        m_previouslyKilled = false;

        // 检查是否存在讲台结构，如果不存在则创建非激活讲台
        // 讲台位于原点 (0, 0, 0)，通过 getHeight 获取表面高度后检查基岩
        const i32 surfaceY = world.getHeight(0, 0);
        const BlockState* surfaceBlock = world.getBlockState(0, surfaceY - 1, 0);
        bool hasPodium = (surfaceBlock != nullptr && surfaceBlock->is(VanillaBlocks::BEDROCK));

        if (!hasPodium) {
            // 未找到讲台结构，创建非激活讲台（不含传送门方块）
            spdlog::info("EndDragonFight: No exit portal structure found, creating inactive portal.");
            EndTeleporter::createExitPortal(world, BlockPos(0, 0, 0), false);
        }
    }

    // 检查世界中是否存在末影龙实体
    // TODO: 当前实体系统尚不支持按类型查询实体，暂时无法检测末影龙是否存活。
    // 当实体系统完善后，应在此处检查末影龙实体是否存在来更新 dragonKilled 状态。
    // MC Java 逻辑：
    //   - 如果没有末影龙实体：dragonKilled = true
    //   - 如果有末影龙实体：dragonUUID = enderdragon.getUUID(); dragonKilled = false;
    //     - 若同时无活跃传送门，则丢弃该龙（enderdragon.discard()），因为无传送门的龙是无效状态
    //   - 最终安全检查：如果 !previouslyKilled && dragonKilled，则 dragonKilled = false
}

bool EndDragonFight::_hasActiveExitPortal(IWorld& world)
{
    // 扫描原点周围区块，查找 END_PORTAL 方块
    // 活跃出口传送门包含 END_PORTAL 方块，只有龙被击杀后才会存在
    const BlockState* endPortalState = VanillaBlocks::getState(VanillaBlocks::END_PORTAL);
    if (endPortalState == nullptr) {
        return false;
    }

    for (ChunkCoord cx = -ARENA_CHUNK_RADIUS; cx <= ARENA_CHUNK_RADIUS; ++cx) {
        for (ChunkCoord cz = -ARENA_CHUNK_RADIUS; cz <= ARENA_CHUNK_RADIUS; ++cz) {
            const ChunkData* chunk = world.getChunk(cx, cz);
            if (chunk == nullptr) {
                continue;
            }

            // 出口传送门位于原点附近，Y 坐标通常在 0~75 之间
            // 仅扫描可能存在传送门的高度范围以提升性能
            for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; ++y) {
                for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
                        const BlockState* state = chunk->getBlockState(x, y, z);
                        if (state == endPortalState) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool EndDragonFight::_isArenaLoaded(IWorld& world)
{
    // 检查原点周围的区块是否已加载
    for (ChunkCoord cx = -ARENA_CHUNK_RADIUS; cx <= ARENA_CHUNK_RADIUS; ++cx) {
        for (ChunkCoord cz = -ARENA_CHUNK_RADIUS; cz <= ARENA_CHUNK_RADIUS; ++cz) {
            if (!world.hasChunk(cx, cz)) {
                return false;
            }
        }
    }
    return true;
}

void EndDragonFight::_spawnNewGateway(IWorld& world)
{
    if (m_gateways.empty()) {
        return;
    }

    // 从列表尾部取出索引
    const i32 gatewayIndex = m_gateways.back();
    m_gateways.pop_back();

    // 计算折跃门位置
    const f64 angle =
        2.0 * (-math::PI_DOUBLE + (math::PI_DOUBLE / static_cast<f64>(GATEWAY_COUNT)) * static_cast<f64>(gatewayIndex));
    const i32 x = static_cast<i32>(std::floor(static_cast<f64>(GATEWAY_DISTANCE) * std::cos(angle)));
    const i32 z = static_cast<i32>(std::floor(static_cast<f64>(GATEWAY_DISTANCE) * std::sin(angle)));

    const BlockPos gatewayPos(x, GATEWAY_Y, z);

    _spawnNewGatewayAt(world, gatewayPos);
}

void EndDragonFight::_spawnNewGatewayAt(IWorld& world, const BlockPos& pos)
{
    // 播放折跃门生成效果
    world.playEvent(world::WorldEvents::GATEWAY_SPAWN_EFFECTS, pos, 0);

    // 直接放置折跃门结构（3x5x3 基岩十字框架 + END_GATEWAY 方块）
    blockentity::EndGatewayEntity::createGatewayStructure(world, pos);
}

void EndDragonFight::_placeDragonEgg(IWorld& world)
{
    // 获取 X=0, Z=0 处的高度（MOTION_BLOCKING 语义）
    const i32 topY = world.getHeight(0, 0);

    // 龙蛋放置在最高阻挡运动方块的上方
    // world.getHeight() 返回最高方块Y+1，所以 topY 就是龙蛋应放置的Y坐标
    const BlockState* dragonEggState = VanillaBlocks::getState(VanillaBlocks::DRAGON_EGG);
    if (dragonEggState != nullptr) {
        world.setBlockState(BlockPos(0, topY, 0), dragonEggState);
    }
}

} // namespace mc
