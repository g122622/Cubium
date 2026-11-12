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

#include "common/world/block/registry/vanilla/TrialBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/base/sound/BlockSoundType.hpp"
#include "common/world/block/base/types/HarvestTool.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/BlockStatePredicates.hpp"
#include "world/block/blocks/trial/CrafterBlock.hpp"
#include "world/block/blocks/trial/HeavyCoreBlock.hpp"
#include "world/block/blocks/trial/TrialSpawnerBlock.hpp"
#include "world/block/blocks/trial/VaultBlock.hpp"

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
                .instrument(NoteBlockInstrument::BaseDrum)
                .resistance(50.0f)
                .hardness(50.0f)

                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(2)
                .requiresTool()
                // 原版 trial_spawner：isViewBlocking(Blocks::never)、isSuffocating 保持默认（满格碰撞 → 闷人）。
                .viewBlocking(block_registry::BlockStatePredicates::trialSpawnerNeverView())
                .soundType(BlockSoundTypes::TRIAL_SPAWNER)
                .notSolid()
                .noOcclusion());

    // 宝库 - 试炼密室的奖励容器，需要特定钥匙才能打开
    // VAULT_STATE + FACING + OMINOUS
    TrialBlocks::VAULT = &registry.registerBlock<blocks::VaultBlock>(ResourceLocation("minecraft:vault"),
        BlockProperties(Material::ROCK)
            .instrument(NoteBlockInstrument::BaseDrum)
            .resistance(50.0f)
            .hardness(50.0f)

            .harvestTool(HarvestTool::Pickaxe)
            .harvestLevel(2)
            .requiresTool()
            .soundType(BlockSoundTypes::VAULT)
            // 原版 vault 与 trial_spawner 相同：只覆写视线判定为 never。
            .viewBlocking(block_registry::BlockStatePredicates::trialSpawnerNeverView())
            .noOcclusion()
            // 宝库发 12 级光（原版注册值，由 block_light_emission 实测修正：此前写成 6）。
            .lightLevel(12)
            .notSolid());

    // 合成器 - 自动合成方块，可通过红石控制
    // FACING + TRIGGERED + CRAFTING
    TrialBlocks::CRAFTER = &registry.registerBlock<blocks::CrafterBlock>(ResourceLocation("minecraft:crafter"),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(3.5f)
            .harvestTool(HarvestTool::Pickaxe)
            .requiresTool()
            .soundType(BlockSoundTypes::STONE));

    // 重质核心 - 重质盾牌的核心材料，支持含水
    // 材质为铁，硬度10，抗爆1200，碰撞箱为小型柱状
    TrialBlocks::HEAVY_CORE = &registry.registerBlock<blocks::HeavyCoreBlock>(ResourceLocation("minecraft:heavy_core"),
        BlockProperties(Material::IRON)
            .instrument(NoteBlockInstrument::Snare)
            .hardness(10.0f)
            .resistance(1200.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .requiresTool()
            .soundType(BlockSoundTypes::HEAVY_CORE)
            .notSolid());
}

} // namespace block_registry
} // namespace mc
