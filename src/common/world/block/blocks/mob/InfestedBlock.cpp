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
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/EntitySpawnPlacementRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/arthropod/EndermiteEntity.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/gamerule/GameRules.hpp"

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

void InfestedBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 蠹虫生成逻辑已移至 spawnAfterBreak，因为 onBlockRemoved 无法获取破坏工具信息
    // onBlockRemoved 仍需保留空实现以阻止基类默认行为
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
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

    // 创建蠹虫实体
    auto silverfish = std::make_unique<SilverfishEntity>(EntityId(0));

    // 设置位置（方块中心）
    silverfish->setPosition(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f);
    silverfish->setRotation(0.0f, 0.0f);

    // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化
    entity::combat::DifficultyInstance difficultyInstance(world.difficulty());
    silverfish->finalizeSpawn(world, difficultyInstance, world::spawn::SpawnReason::Event);

    // 生成到世界
    world.spawnEntity(std::move(silverfish));

    // TODO: 生成爆炸粒子效果 (ParticleTypeId::Poof)
    // 粒子效果通过 ServerWorld::addParticle 广播给客户端
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
