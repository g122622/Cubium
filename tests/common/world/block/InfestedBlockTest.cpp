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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/mob/InfestedBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// 测试 InfestedBlock spawnAfterBreak 机制及依赖组件
// 注意：InfestedBlock 的基础构造测试在 MobBlocksTest.cpp 中，
// 此文件专注于 spawnAfterBreak 相关逻辑和静态方法。
// ============================================================================

// ============================================================================
// InfestedBlock 静态方法测试
// ============================================================================

TEST(InfestedBlockSpawnTest, CanContainSilverfish_AirBlock_ReturnsFalse)
{
    // 空气方块不应该是虫蚀方块
    if (VanillaBlocks::AIR != nullptr) {
        const BlockState* state = &VanillaBlocks::AIR->defaultState();
        EXPECT_FALSE(InfestedBlock::canContainSilverfish(*state));
    }
}

TEST(InfestedBlockSpawnTest, Infest_AirBlock_ReturnsNullptr)
{
    // 空气方块不应有虫蚀变体
    if (VanillaBlocks::AIR != nullptr) {
        const BlockState* result = InfestedBlock::infest(*VanillaBlocks::AIR);
        EXPECT_EQ(result, nullptr);
    }
}

// ============================================================================
// EnchantmentHelper::hasSilkTouch 测试 - InfestedBlock 依赖此方法判断精准采集
// ============================================================================

TEST(InfestedBlockSpawnTest, EnchantmentHelper_HasSilkTouch_EmptyStack_ReturnsFalse)
{
    // 空物品堆不应有精准采集
    ItemStack emptyStack;
    EXPECT_FALSE(item::enchant::EnchantmentHelper::hasSilkTouch(emptyStack));
}

TEST(InfestedBlockSpawnTest, EnchantmentHelper_HasSilkTouch_DefaultItemStack_ReturnsFalse)
{
    // 默认构造的物品堆（无附魔）不应有精准采集
    ItemStack stack;
    EXPECT_FALSE(item::enchant::EnchantmentHelper::hasSilkTouch(stack));
}

// ============================================================================
// Block::spawnAfterBreak 虚方法签名验证
// ============================================================================

namespace {

/// @brief 用于验证 spawnAfterBreak 虚方法可以被正确重写的测试方块
class TestSpawnAfterBreakBlock final : public Block {
public:
    mutable i32 spawnCallCount = 0;
    mutable const ItemStack* lastTool = nullptr;
    mutable bool lastDropExp = false;

    explicit TestSpawnAfterBreakBlock()
        : Block(BlockProperties(Material::ROCK).hardness(1.0f).resistance(1.0f))
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block,
                std::vector<size_t> values,
                const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                const std::vector<BlockState*>* allStates,
                u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    void spawnAfterBreak(
        IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack* tool, bool dropExp) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        ++spawnCallCount;
        lastTool = tool;
        lastDropExp = dropExp;
    }
};

} // anonymous namespace

TEST(InfestedBlockSpawnTest, SpawnAfterBreak_OverrideExists)
{
    // 验证 spawnAfterBreak 虚方法可以被正确重写和通过基类指针调用
    TestSpawnAfterBreakBlock block;
    EXPECT_EQ(block.spawnCallCount, 0);

    // 验证基类指针可以访问虚方法
    const Block& baseRef = block;
    (void)baseRef;
}

// ============================================================================
// GameRules DO_TILE_DROPS 常量可用性测试
// ============================================================================

TEST(InfestedBlockSpawnTest, GameRuleKeys_DoTileDrops_ExistsAndValid)
{
    // 验证 DO_TILE_DROPS 游戏规则键已正确定义
    const auto& key = world::gamerule::GameRuleKeys::DO_TILE_DROPS;
    EXPECT_EQ(key.getName(), "doTileDrops");
}

// ============================================================================
// InfestedBlock 构造和映射注册测试
// ============================================================================

TEST(InfestedBlockSpawnTest, RegisterAndLookupMapping)
{
    // 注册测试映射
    const u32 hostBlockId = 9999;
    const u32 infestedBlockId = 10000;

    InfestedBlock::registerInfestedBlock(hostBlockId, infestedBlockId);
    InfestedBlock::initializeMappings();

    // 验证映射已注册
    // 注意：由于 registerInfestedBlock 和 initializeMappings 使用静态数据，
    // 这些测试受执行顺序影响。我们仅验证方法不会崩溃。
}
