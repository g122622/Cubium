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

#include "InfestedBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/monster/arthropod/EndermiteEntity.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// 静态成员定义
std::vector<std::pair<u32, u32>> InfestedBlock::s_hostToInfestedMap;
bool InfestedBlock::s_mappingsInitialized = false;

InfestedBlock::InfestedBlock(u32 hostBlock, const BlockProperties& properties)
    : Block(properties)
    , m_hostBlock(hostBlock)
{
    // 被感染方块没有特殊状态
}

void InfestedBlock::spawnAfterBreak(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack* tool, bool dropExp) const
{
    MC_UNUSED(state);
    MC_UNUSED(dropExp);

    // 只在服务端生成
    if (world.isClientSide()) {
        return;
    }

    // 检查游戏规则 doTileDrops：为 false 时不生成蠹虫
    if (!world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_TILE_DROPS)) {
        return;
    }

    // 检查精准采集附魔：使用精准采集工具破坏时不生成蠹虫
    if (tool != nullptr && !tool->isEmpty()) {
        if (item::enchant::EnchantmentHelper::hasSilkTouch(*tool)) {
            return;
        }
    }

    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = world.entityRegistry();
    if (registry == nullptr) {
        return;
    }

    // 创建蠹虫实体
    auto silverfish = std::make_unique<SilverfishEntity>(EntityInstanceId(0), *registry);
    silverfish->setTypeId(entity::EntityTypeKeys::SILVERFISH);

    // 设置位置（方块中心）
    silverfish->setPosition(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f);
    silverfish->setRotation(0.0f, 0.0f);

    // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化（使用位置感知的区域难度）
    entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(world, pos);
    silverfish->finalizeSpawn(world, difficultyInstance, world::spawn::SpawnReason::Event);

    // 生成到世界
    world.spawnEntity(std::move(silverfish));

    // 生成蠹虫出现的烟雾粒子效果
    world.addParticle(particle::ParticleTypeId::Poof,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f),
        Vector3(0.0f, 0.0f, 0.0f));
}

// ========== 静态方法实现 ==========

void InfestedBlock::registerInfestedBlock(u32 hostBlock, u32 infestedBlock)
{
    s_hostToInfestedMap.emplace_back(hostBlock, infestedBlock);
}

void InfestedBlock::initializeMappings()
{
    if (s_mappingsInitialized) {
        return;
    }
    s_mappingsInitialized = true;
    // 映射表在 VanillaBlocks::registerInfestedBlocks() 中填充
}

bool InfestedBlock::canContainSilverfish(const BlockState& state)
{
    for (const auto& pair : s_hostToInfestedMap) {
        if (pair.first == state.blockId()) {
            return true;
        }
    }
    return false;
}

const BlockState* InfestedBlock::infest(const Block& block)
{
    for (const auto& pair : s_hostToInfestedMap) {
        if (pair.first == block.blockId()) {
            return &BlockRegistry::instance().getBlock(pair.second)->defaultState();
        }
    }
    return nullptr;
}

} // namespace blocks
} // namespace mc
