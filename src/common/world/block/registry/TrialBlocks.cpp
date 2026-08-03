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
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BY NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "world/block/registry/TrialBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/trial/HeavyCoreBlock.hpp"
#include "world/block/blocks/trial/TrialBlocks.hpp"

namespace mc {
namespace block_registry {

// 试炼密室核心方块
Block* TrialBlocks::TRIAL_SPAWNER = nullptr;
Block* TrialBlocks::VAULT = nullptr;

// 合成器
Block* TrialBlocks::CRAFTER = nullptr;

// 重质核心
Block* TrialBlocks::HEAVY_CORE = nullptr;

void registerTrialBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 试炼密室方块注册
    // ============================================================================

    // 试炼刷怪笼 - 试炼密室的核心，根据玩家战斗生成怪物
    // TRIAL_SPAWNER_STATE + OMINOUS
    TrialBlocks::TRIAL_SPAWNER =
        &registry.registerBlock<blocks::TrialSpawnerBlock>(ResourceLocation("minecraft:trial_spawner"),
            BlockProperties(Material::ROCK)
                .hardness(50.0f)
                .resistance(1200.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(2)
                .requiresTool()
                .soundType(BlockSoundTypes::TRIAL_SPAWNER)
                .notSolid());

    // 宝库 - 试炼密室的奖励容器，需要特定钥匙才能打开
    // VAULT_STATE + FACING + OMINOUS
    TrialBlocks::VAULT = &registry.registerBlock<blocks::VaultBlock>(ResourceLocation("minecraft:vault"),
        BlockProperties(Material::ROCK)
            .hardness(50.0f)
            .resistance(1200.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .harvestLevel(2)
            .requiresTool()
            .soundType(BlockSoundTypes::VAULT)
            .notSolid());

    // 合成器 - 自动合成方块，可通过红石控制
    // FACING + TRIGGERED + CRAFTING
    TrialBlocks::CRAFTER = &registry.registerBlock<blocks::CrafterBlock>(ResourceLocation("minecraft:crafter"),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(3.5f)
            .harvestTool(HarvestTool::Pickaxe)
            .requiresTool()
            .soundType(BlockSoundTypes::CRAFTER));

    // 重质核心 - 重质盾牌的核心材料，支持含水
    // 材质为铁，硬度10，抗爆1200，碰撞箱为小型柱状
    TrialBlocks::HEAVY_CORE = &registry.registerBlock<blocks::HeavyCoreBlock>(ResourceLocation("minecraft:heavy_core"),
        BlockProperties(Material::IRON)
            .hardness(10.0f)
            .resistance(1200.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .requiresTool()
            .soundType(BlockSoundTypes::HEAVY_CORE)
            .notSolid());
}

} // namespace block_registry
} // namespace mc
