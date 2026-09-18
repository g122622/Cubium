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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/HoneycombItem.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/ChestBlock.hpp"
#include "common/world/block/blocks/copper/CopperChestBlock.hpp"
#include "common/world/block/blocks/copper/IOxidizableBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "item/context/BlockItemUseContext.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

// 注意：不能同时 using namespace mc 和 using namespace mc::blocks，
// 因为 ChestEntity.hpp 在 mc:: 中前向声明了 ChestBlock，
// 与 mc::blocks::ChestBlock 冲突。
using namespace mc::blocks;
using mc::Block;
using mc::BlockEntity;
using mc::BlockEntityType;
using mc::BlockItemUseContext;
using mc::BlockPos;
using mc::BlockProperties;
using mc::BlockState;
using mc::BlockStateProperties;
using mc::BlockTags;
using mc::CollisionShape;
using mc::Direction;
using mc::f32;
using mc::i32;
using mc::Items;
using mc::ItemStack;
using mc::IWorld;
using mc::Material;
using mc::Mirror;
using mc::ResourceLocation;
using mc::Rotation;
using mc::u64;
using mc::VanillaBlocks;
using mc::Vector3;
using mc::fluid::Fluid;
using mc::fluid::FluidState;
using mc::item::items::HoneycombItem;
using mc::math::IRandom;
using mc::sound::SoundCategory;
using mc::test::BaseTestWorld;
using mc::world::tick::TickManager;
using namespace mc::Directions;
namespace SoundEvents = mc::SoundEvents;

namespace {

// ============================================================================
// 测试用世界 - 支持方块状态/方块实体存储、音效捕获
// 复用铜傀儡雕像测试中的 CopperGolemStatueTestWorld 模式
// ============================================================================

class CopperChestTestWorld final : public BaseTestWorld {
public:
    CopperChestTestWorld() { m_tickManagerPtr = std::make_unique<TickManager>(*this); }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    // 2-arg 重载（便于测试通过 BlockPos 设置）
    bool setBlockState(const BlockPos& pos, const BlockState* state)
    {
        return setBlockState(pos.x, pos.y, pos.z, state);
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] const BlockState* getBlockState(const BlockPos& pos) const
    {
        return getBlockState(pos.x, pos.y, pos.z);
    }

    [[nodiscard]] const FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr) {
                return fluidState;
            }
        }
        return &mc::fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override
    {
        if (entity != nullptr) {
            entity->setWorld(this);
            m_blockEntities[pos] = std::unique_ptr<BlockEntity>(entity);
        } else {
            m_blockEntities.erase(pos);
        }
    }

    void playSound(
        const ResourceLocation& sound, SoundCategory category, const Vector3& pos, f32 volume, f32 pitch) override
    {
        m_sounds.push_back({sound, category, pos, volume, pitch});
    }

    [[nodiscard]] TickManager& tickManager() override { return *m_tickManagerPtr; }
    [[nodiscard]] const TickManager& tickManager() const override { return *m_tickManagerPtr; }

    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }

    // ========== 测试辅助方法 ==========

    struct SoundRecord {
        ResourceLocation sound;
        SoundCategory category;
        Vector3 pos;
        f32 volume;
        f32 pitch;
    };

    [[nodiscard]] const std::vector<SoundRecord>& sounds() const { return m_sounds; }
    void clearSounds() { m_sounds.clear(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::vector<SoundRecord> m_sounds;
    std::unique_ptr<TickManager> m_tickManagerPtr;
    u64 m_seed = 0;
};

BlockItemUseContext makePlacementContext(IWorld& world, const BlockPos& pos, Direction face, f32 playerYaw)
{
    static const ItemStack EMPTY_STACK = ItemStack::EMPTY;
    return BlockItemUseContext(world,
        nullptr,
        EMPTY_STACK,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f),
        pos,
        face,
        playerYaw,
        0.0f);
}

} // namespace

// ============================================================================
// 铜箱子方块测试
// ============================================================================

class CopperChestBlockTestFixture : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockTags::initialize();
    }
};

// ---------- 注册 ----------

TEST_F(CopperChestBlockTestFixture, Registration_AllVariantsRegistered)
{
    ASSERT_NE(VanillaBlocks::COPPER_CHEST, nullptr);
    ASSERT_NE(VanillaBlocks::EXPOSED_COPPER_CHEST, nullptr);
    ASSERT_NE(VanillaBlocks::WEATHERED_COPPER_CHEST, nullptr);
    ASSERT_NE(VanillaBlocks::OXIDIZED_COPPER_CHEST, nullptr);
    ASSERT_NE(VanillaBlocks::WAXED_COPPER_CHEST, nullptr);
    ASSERT_NE(VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST, nullptr);
    ASSERT_NE(VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST, nullptr);
    ASSERT_NE(VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST, nullptr);
}

// ---------- 默认状态 ----------

TEST_F(CopperChestBlockTestFixture, DefaultState_ContainsAllProperties)
{
    ASSERT_NE(VanillaBlocks::COPPER_CHEST, nullptr);

    const BlockState& state = VanillaBlocks::COPPER_CHEST->defaultState();

    // 默认朝北
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
    // 默认单箱
    EXPECT_EQ(state.get(BlockStateProperties::CHEST_TYPE()), BlockStateProperties::ChestType::Single);
    // 默认不含水
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(CopperChestBlockTestFixture, DefaultState_AllVariantsHaveChestType)
{
    // 验证 8 个铜箱子变体都拥有 CHEST_TYPE 属性
    std::array<Block*, 8> chests = {
        VanillaBlocks::COPPER_CHEST,
        VanillaBlocks::EXPOSED_COPPER_CHEST,
        VanillaBlocks::WEATHERED_COPPER_CHEST,
        VanillaBlocks::OXIDIZED_COPPER_CHEST,
        VanillaBlocks::WAXED_COPPER_CHEST,
        VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST,
        VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST,
        VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST,
    };

    for (Block* block : chests) {
        ASSERT_NE(block, nullptr);
        EXPECT_TRUE(block->defaultState().hasProperty(BlockStateProperties::CHEST_TYPE()))
            << "A copper chest variant is missing CHEST_TYPE property";
    }
}

// ---------- 方块实体 ----------

TEST_F(CopperChestBlockTestFixture, HasBlockEntity_ReturnsTrue)
{
    EXPECT_TRUE(VanillaBlocks::COPPER_CHEST->hasBlockEntity());
    EXPECT_TRUE(VanillaBlocks::EXPOSED_COPPER_CHEST->hasBlockEntity());
    EXPECT_TRUE(VanillaBlocks::WAXED_COPPER_CHEST->hasBlockEntity());
}

TEST_F(CopperChestBlockTestFixture, CreateBlockEntity_ReturnsChestEntity)
{
    const BlockPos pos(1, 2, 3);

    auto entity = VanillaBlocks::COPPER_CHEST->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Chest);
}

TEST_F(CopperChestBlockTestFixture, GetBlockEntityType_ReturnsChest)
{
    // getBlockEntityType 是 ChestBlock 的虚方法，铜箱子继承自 ChestBlock
    auto* base = dynamic_cast<const ChestBlock*>(VanillaBlocks::COPPER_CHEST);
    ASSERT_NE(base, nullptr);
    EXPECT_EQ(base->getBlockEntityType(), BlockEntityType::Chest);

    auto* exposed = dynamic_cast<const ChestBlock*>(VanillaBlocks::EXPOSED_COPPER_CHEST);
    ASSERT_NE(exposed, nullptr);
    EXPECT_EQ(exposed->getBlockEntityType(), BlockEntityType::Chest);

    auto* waxed = dynamic_cast<const ChestBlock*>(VanillaBlocks::WAXED_COPPER_CHEST);
    ASSERT_NE(waxed, nullptr);
    EXPECT_EQ(waxed->getBlockEntityType(), BlockEntityType::Chest);
}

// ---------- 氧化等级 ----------

TEST_F(CopperChestBlockTestFixture, OxidationLevel_BaseReturnsUnaffected)
{
    auto* base = dynamic_cast<const CopperChestBlock*>(VanillaBlocks::COPPER_CHEST);
    ASSERT_NE(base, nullptr);
    EXPECT_EQ(base->getOxidationLevel(), BlockStateProperties::OxidationLevel::Unaffected);
}

TEST_F(CopperChestBlockTestFixture, OxidationLevel_ExposedReturnsExposed)
{
    auto* exposed = dynamic_cast<const CopperChestBlock*>(VanillaBlocks::EXPOSED_COPPER_CHEST);
    ASSERT_NE(exposed, nullptr);
    EXPECT_EQ(exposed->getOxidationLevel(), BlockStateProperties::OxidationLevel::Exposed);
}

TEST_F(CopperChestBlockTestFixture, OxidationLevel_WeatheredReturnsWeathered)
{
    auto* weathered = dynamic_cast<const CopperChestBlock*>(VanillaBlocks::WEATHERED_COPPER_CHEST);
    ASSERT_NE(weathered, nullptr);
    EXPECT_EQ(weathered->getOxidationLevel(), BlockStateProperties::OxidationLevel::Weathered);
}

TEST_F(CopperChestBlockTestFixture, OxidationLevel_OxidizedReturnsOxidized)
{
    auto* oxidized = dynamic_cast<const CopperChestBlock*>(VanillaBlocks::OXIDIZED_COPPER_CHEST);
    ASSERT_NE(oxidized, nullptr);
    EXPECT_EQ(oxidized->getOxidationLevel(), BlockStateProperties::OxidationLevel::Oxidized);
}

TEST_F(CopperChestBlockTestFixture, OxidationLevel_WaxedVariantsRecordOxidationLevel)
{
    // 涂蜡变体也记录对应氧化等级（用于除蜡后恢复）
    auto* waxed = dynamic_cast<const CopperChestBlock*>(VanillaBlocks::WAXED_COPPER_CHEST);
    ASSERT_NE(waxed, nullptr);
    EXPECT_EQ(waxed->getOxidationLevel(), BlockStateProperties::OxidationLevel::Unaffected);

    auto* waxedExposed = dynamic_cast<const CopperChestBlock*>(VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST);
    ASSERT_NE(waxedExposed, nullptr);
    EXPECT_EQ(waxedExposed->getOxidationLevel(), BlockStateProperties::OxidationLevel::Exposed);

    auto* waxedWeathered = dynamic_cast<const CopperChestBlock*>(VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST);
    ASSERT_NE(waxedWeathered, nullptr);
    EXPECT_EQ(waxedWeathered->getOxidationLevel(), BlockStateProperties::OxidationLevel::Weathered);

    auto* waxedOxidized = dynamic_cast<const CopperChestBlock*>(VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST);
    ASSERT_NE(waxedOxidized, nullptr);
    EXPECT_EQ(waxedOxidized->getOxidationLevel(), BlockStateProperties::OxidationLevel::Oxidized);
}

// ---------- 氧化链前向 ----------

TEST_F(CopperChestBlockTestFixture, OxidationChain_ExposedNextIsWeathered)
{
    auto* exposed = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::EXPOSED_COPPER_CHEST);
    ASSERT_NE(exposed, nullptr);
    EXPECT_EQ(exposed->getNextOxidationBlock(), VanillaBlocks::WEATHERED_COPPER_CHEST);
}

TEST_F(CopperChestBlockTestFixture, OxidationChain_WeatheredNextIsOxidized)
{
    auto* weathered = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WEATHERED_COPPER_CHEST);
    ASSERT_NE(weathered, nullptr);
    EXPECT_EQ(weathered->getNextOxidationBlock(), VanillaBlocks::OXIDIZED_COPPER_CHEST);
}

TEST_F(CopperChestBlockTestFixture, OxidationChain_OxidizedNextIsNull)
{
    auto* oxidized = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::OXIDIZED_COPPER_CHEST);
    ASSERT_NE(oxidized, nullptr);
    EXPECT_EQ(oxidized->getNextOxidationBlock(), nullptr);
}

TEST_F(CopperChestBlockTestFixture, OxidationChain_BaseNotOxidizable)
{
    // 基础 copper_chest 是 CopperChestBlock（不实现 IOxidizableBlock），
    // 不参与氧化 tick，处于氧化链的 Unaffected 位置但不向前氧化
    auto* base = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::COPPER_CHEST);
    EXPECT_EQ(base, nullptr);
}

// ---------- 氧化链反向（斧头刮削） ----------

TEST_F(CopperChestBlockTestFixture, OxidationChain_ExposedPreviousIsBase)
{
    auto* exposed = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::EXPOSED_COPPER_CHEST);
    ASSERT_NE(exposed, nullptr);
    EXPECT_EQ(exposed->getPreviousOxidationBlock(), VanillaBlocks::COPPER_CHEST);
}

TEST_F(CopperChestBlockTestFixture, OxidationChain_WeatheredPreviousIsExposed)
{
    auto* weathered = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WEATHERED_COPPER_CHEST);
    ASSERT_NE(weathered, nullptr);
    EXPECT_EQ(weathered->getPreviousOxidationBlock(), VanillaBlocks::EXPOSED_COPPER_CHEST);
}

TEST_F(CopperChestBlockTestFixture, OxidationChain_OxidizedPreviousIsWeathered)
{
    auto* oxidized = dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::OXIDIZED_COPPER_CHEST);
    ASSERT_NE(oxidized, nullptr);
    EXPECT_EQ(oxidized->getPreviousOxidationBlock(), VanillaBlocks::WEATHERED_COPPER_CHEST);
}

// ---------- IOxidizableBlock 接口 ----------

TEST_F(CopperChestBlockTestFixture, DynamicCast_WeatheringVariantsAreOxidizable)
{
    // Exposed/Weathered/Oxidized 实现 IOxidizableBlock
    EXPECT_NE(dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::EXPOSED_COPPER_CHEST), nullptr);
    EXPECT_NE(dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WEATHERED_COPPER_CHEST), nullptr);
    EXPECT_NE(dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::OXIDIZED_COPPER_CHEST), nullptr);
}

TEST_F(CopperChestBlockTestFixture, DynamicCast_BaseAndWaxedNotOxidizable)
{
    // 基础 + 涂蜡变体不实现 IOxidizableBlock
    EXPECT_EQ(dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::COPPER_CHEST), nullptr);
    EXPECT_EQ(dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WAXED_COPPER_CHEST), nullptr);
    EXPECT_EQ(dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST), nullptr);
    EXPECT_EQ(dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST), nullptr);
    EXPECT_EQ(dynamic_cast<const IOxidizableBlock*>(VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST), nullptr);
}

// ---------- isWaxed ----------

TEST_F(CopperChestBlockTestFixture, IsWaxed_BaseReturnsFalse)
{
    auto* base = dynamic_cast<const CopperChestBlock*>(VanillaBlocks::COPPER_CHEST);
    ASSERT_NE(base, nullptr);
    EXPECT_FALSE(base->isWaxed());
}

TEST_F(CopperChestBlockTestFixture, IsWaxed_WaxedVariantsReturnTrue)
{
    auto* waxed = dynamic_cast<const CopperChestBlock*>(VanillaBlocks::WAXED_COPPER_CHEST);
    ASSERT_NE(waxed, nullptr);
    EXPECT_TRUE(waxed->isWaxed());

    auto* waxedExposed = dynamic_cast<const CopperChestBlock*>(VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST);
    ASSERT_NE(waxedExposed, nullptr);
    EXPECT_TRUE(waxedExposed->isWaxed());

    auto* waxedWeathered = dynamic_cast<const CopperChestBlock*>(VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST);
    ASSERT_NE(waxedWeathered, nullptr);
    EXPECT_TRUE(waxedWeathered->isWaxed());

    auto* waxedOxidized = dynamic_cast<const CopperChestBlock*>(VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST);
    ASSERT_NE(waxedOxidized, nullptr);
    EXPECT_TRUE(waxedOxidized->isWaxed());
}

// ---------- ticksRandomly ----------

TEST_F(CopperChestBlockTestFixture, TicksRandomly_ExposedReturnsTrue)
{
    EXPECT_TRUE(VanillaBlocks::EXPOSED_COPPER_CHEST->ticksRandomly());
}

TEST_F(CopperChestBlockTestFixture, TicksRandomly_WeatheredReturnsTrue)
{
    EXPECT_TRUE(VanillaBlocks::WEATHERED_COPPER_CHEST->ticksRandomly());
}

TEST_F(CopperChestBlockTestFixture, TicksRandomly_OxidizedReturnsFalse)
{
    // Oxidized 是最高氧化等级，不再氧化
    EXPECT_FALSE(VanillaBlocks::OXIDIZED_COPPER_CHEST->ticksRandomly());
}

TEST_F(CopperChestBlockTestFixture, TicksRandomly_BaseReturnsFalse)
{
    // 基础 copper_chest 不实现 IOxidizableBlock，不参与氧化 tick
    EXPECT_FALSE(VanillaBlocks::COPPER_CHEST->ticksRandomly());
}

TEST_F(CopperChestBlockTestFixture, TicksRandomly_WaxedVariantsReturnFalse)
{
    // 涂蜡变体不氧化
    EXPECT_FALSE(VanillaBlocks::WAXED_COPPER_CHEST->ticksRandomly());
    EXPECT_FALSE(VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST->ticksRandomly());
    EXPECT_FALSE(VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST->ticksRandomly());
    EXPECT_FALSE(VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST->ticksRandomly());
}

// ---------- shouldChangedStateKeepBlockEntity ----------

TEST_F(CopperChestBlockTestFixture, ShouldChangedStateKeepBlockEntity_AllVariantsReturnTrue)
{
    // 所有铜箱子变体（基础/Weathering/Waxed）都应保留方块实体
    const BlockState& baseState = VanillaBlocks::COPPER_CHEST->defaultState();
    EXPECT_TRUE(VanillaBlocks::COPPER_CHEST->shouldChangedStateKeepBlockEntity(baseState));

    const BlockState& exposedState = VanillaBlocks::EXPOSED_COPPER_CHEST->defaultState();
    EXPECT_TRUE(VanillaBlocks::EXPOSED_COPPER_CHEST->shouldChangedStateKeepBlockEntity(exposedState));

    const BlockState& waxedState = VanillaBlocks::WAXED_COPPER_CHEST->defaultState();
    EXPECT_TRUE(VanillaBlocks::WAXED_COPPER_CHEST->shouldChangedStateKeepBlockEntity(waxedState));
}

TEST_F(CopperChestBlockTestFixture, ShouldChangedStateKeepBlockEntity_NormalChestReturnsFalse)
{
    // 普通箱子不应保留方块实体（默认行为，方块类型变化时丢弃旧实体）
    ChestBlock normalChest(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    const BlockState& state = normalChest.defaultState();
    EXPECT_FALSE(normalChest.shouldChangedStateKeepBlockEntity(state));
}

// ---------- 双箱连接 chestCanConnectTo ----------

TEST_F(CopperChestBlockTestFixture, ChestCanConnectTo_CopperChestConnectsToExposed)
{
    // 铜箱子可以与不同氧化等级的铜箱子连接
    auto* base = dynamic_cast<const ChestBlock*>(VanillaBlocks::COPPER_CHEST);
    ASSERT_NE(base, nullptr);

    const BlockState& exposedState = VanillaBlocks::EXPOSED_COPPER_CHEST->defaultState();
    EXPECT_TRUE(base->chestCanConnectTo(exposedState));
}

TEST_F(CopperChestBlockTestFixture, ChestCanConnectTo_CopperChestConnectsToWaxed)
{
    // 铜箱子可以与涂蜡变体连接
    auto* base = dynamic_cast<const ChestBlock*>(VanillaBlocks::COPPER_CHEST);
    ASSERT_NE(base, nullptr);

    const BlockState& waxedState = VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST->defaultState();
    EXPECT_TRUE(base->chestCanConnectTo(waxedState));
}

TEST_F(CopperChestBlockTestFixture, ChestCanConnectTo_CopperChestDoesNotConnectToNormalChest)
{
    // 铜箱子不能与普通箱子连接
    auto* base = dynamic_cast<const ChestBlock*>(VanillaBlocks::COPPER_CHEST);
    ASSERT_NE(base, nullptr);

    ChestBlock normalChest(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    const BlockState& normalState = normalChest.defaultState();
    EXPECT_FALSE(base->chestCanConnectTo(normalState));
}

TEST_F(CopperChestBlockTestFixture, ChestCanConnectTo_NormalChestDoesNotConnectToCopperChest)
{
    // 普通箱子也不能与铜箱子连接（对称性）
    ChestBlock normalChest(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));

    const BlockState& copperState = VanillaBlocks::COPPER_CHEST->defaultState();
    EXPECT_FALSE(normalChest.chestCanConnectTo(copperState));
}

TEST_F(CopperChestBlockTestFixture, ChestCanConnectTo_NormalChestConnectsToSelf)
{
    // 普通箱子默认实现：与同类连接
    ChestBlock normalChest(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    const BlockState& normalState = normalChest.defaultState();
    EXPECT_TRUE(normalChest.chestCanConnectTo(normalState));
}

TEST_F(CopperChestBlockTestFixture, ChestCanConnectTo_CopperChestConnectsToSameVariant)
{
    // 铜箱子与同类变体连接
    auto* weathered = dynamic_cast<const ChestBlock*>(VanillaBlocks::WEATHERED_COPPER_CHEST);
    ASSERT_NE(weathered, nullptr);

    const BlockState& weatheredState = VanillaBlocks::WEATHERED_COPPER_CHEST->defaultState();
    EXPECT_TRUE(weathered->chestCanConnectTo(weatheredState));
}

// ---------- 涂蜡映射 ----------

TEST_F(CopperChestBlockTestFixture, WaxMapping_AllFourOxidationLevelsMapped)
{
    // 验证 4 组涂蜡映射：未涂蜡 -> 涂蜡
    const auto& waxables = HoneycombItem::getWaxablesMap();

    EXPECT_EQ(waxables.count(VanillaBlocks::COPPER_CHEST), 1u);
    EXPECT_EQ(waxables.at(VanillaBlocks::COPPER_CHEST), VanillaBlocks::WAXED_COPPER_CHEST);

    EXPECT_EQ(waxables.count(VanillaBlocks::EXPOSED_COPPER_CHEST), 1u);
    EXPECT_EQ(waxables.at(VanillaBlocks::EXPOSED_COPPER_CHEST), VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST);

    EXPECT_EQ(waxables.count(VanillaBlocks::WEATHERED_COPPER_CHEST), 1u);
    EXPECT_EQ(waxables.at(VanillaBlocks::WEATHERED_COPPER_CHEST), VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST);

    EXPECT_EQ(waxables.count(VanillaBlocks::OXIDIZED_COPPER_CHEST), 1u);
    EXPECT_EQ(waxables.at(VanillaBlocks::OXIDIZED_COPPER_CHEST), VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST);
}

TEST_F(CopperChestBlockTestFixture, WaxOffMapping_AllFourWaxedVariantsMapped)
{
    // 验证 4 组除蜡映射：涂蜡 -> 未涂蜡（自动反向构造）
    const auto& waxOff = HoneycombItem::getWaxOffMap();

    EXPECT_EQ(waxOff.count(VanillaBlocks::WAXED_COPPER_CHEST), 1u);
    EXPECT_EQ(waxOff.at(VanillaBlocks::WAXED_COPPER_CHEST), VanillaBlocks::COPPER_CHEST);

    EXPECT_EQ(waxOff.count(VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST), 1u);
    EXPECT_EQ(waxOff.at(VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST), VanillaBlocks::EXPOSED_COPPER_CHEST);

    EXPECT_EQ(waxOff.count(VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST), 1u);
    EXPECT_EQ(waxOff.at(VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST), VanillaBlocks::WEATHERED_COPPER_CHEST);

    EXPECT_EQ(waxOff.count(VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST), 1u);
    EXPECT_EQ(waxOff.at(VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST), VanillaBlocks::OXIDIZED_COPPER_CHEST);
}

TEST_F(CopperChestBlockTestFixture, GetWaxedOff_PreservesChestTypeAndFacing)
{
    // 验证除蜡后属性保留：FACING/CHEST_TYPE/WATERLOGGED 应通过 withPropertiesOf 保留
    const BlockState& waxedState = VanillaBlocks::WAXED_COPPER_CHEST->defaultState()
                                       .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
                                       .with(BlockStateProperties::CHEST_TYPE(), BlockStateProperties::ChestType::Left);

    auto result = HoneycombItem::getWaxedOff(waxedState);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(&result->getBlock(), VanillaBlocks::COPPER_CHEST);
    EXPECT_EQ(result->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
    EXPECT_EQ(result->get(BlockStateProperties::CHEST_TYPE()), BlockStateProperties::ChestType::Left);
}

TEST_F(CopperChestBlockTestFixture, GetWaxed_NormalChestReturnsNullopt)
{
    // 普通箱子不在涂蜡映射中
    ChestBlock normalChest(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    const BlockState& normalState = normalChest.defaultState();

    EXPECT_FALSE(HoneycombItem::getWaxed(normalState).has_value());
    EXPECT_FALSE(HoneycombItem::getWaxedOff(normalState).has_value());
}

// ---------- 标签 ----------

TEST_F(CopperChestBlockTestFixture, Tag_CopperChestsContainsAllVariants)
{
    // copper_chests 标签应包含全部 8 个铜箱子变体
    EXPECT_TRUE(BlockTags::COPPER_CHESTS().contains(VanillaBlocks::COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER_CHESTS().contains(VanillaBlocks::EXPOSED_COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER_CHESTS().contains(VanillaBlocks::WEATHERED_COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER_CHESTS().contains(VanillaBlocks::OXIDIZED_COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER_CHESTS().contains(VanillaBlocks::WAXED_COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER_CHESTS().contains(VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER_CHESTS().contains(VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER_CHESTS().contains(VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST->defaultState()));
}

TEST_F(CopperChestBlockTestFixture, Tag_CopperTagContainsAllVariants)
{
    // copper 标签应包含全部 8 个铜箱子变体
    EXPECT_TRUE(BlockTags::COPPER().contains(VanillaBlocks::COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER().contains(VanillaBlocks::EXPOSED_COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER().contains(VanillaBlocks::WEATHERED_COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER().contains(VanillaBlocks::OXIDIZED_COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER().contains(VanillaBlocks::WAXED_COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER().contains(VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER().contains(VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST->defaultState()));
    EXPECT_TRUE(BlockTags::COPPER().contains(VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST->defaultState()));
}

TEST_F(CopperChestBlockTestFixture, Tag_CopperChestsDoesNotContainNormalChest)
{
    // copper_chests 标签不应包含普通箱子
    ChestBlock normalChest(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    EXPECT_FALSE(BlockTags::COPPER_CHESTS().contains(normalChest.defaultState()));
}

// ---------- 开合音效 ----------

TEST_F(CopperChestBlockTestFixture, OpenSound_UnaffectedReturnsCopperChestOpen)
{
    auto* base = dynamic_cast<const ChestBlock*>(VanillaBlocks::COPPER_CHEST);
    ASSERT_NE(base, nullptr);
    EXPECT_EQ(base->getOpenSound(), SoundEvents::BLOCK_COPPER_CHEST_OPEN);
}

TEST_F(CopperChestBlockTestFixture, CloseSound_UnaffectedReturnsCopperChestClose)
{
    auto* base = dynamic_cast<const ChestBlock*>(VanillaBlocks::COPPER_CHEST);
    ASSERT_NE(base, nullptr);
    EXPECT_EQ(base->getCloseSound(), SoundEvents::BLOCK_COPPER_CHEST_CLOSE);
}

TEST_F(CopperChestBlockTestFixture, OpenSound_ExposedReturnsCopperChestOpen)
{
    // Exposed 等级复用 Unaffected 的 block.copper_chest.open/close 声音
    auto* exposed = dynamic_cast<const ChestBlock*>(VanillaBlocks::EXPOSED_COPPER_CHEST);
    ASSERT_NE(exposed, nullptr);
    EXPECT_EQ(exposed->getOpenSound(), SoundEvents::BLOCK_COPPER_CHEST_OPEN);
    EXPECT_EQ(exposed->getCloseSound(), SoundEvents::BLOCK_COPPER_CHEST_CLOSE);
}

TEST_F(CopperChestBlockTestFixture, OpenSound_WeatheredReturnsWeatheredOpen)
{
    auto* weathered = dynamic_cast<const ChestBlock*>(VanillaBlocks::WEATHERED_COPPER_CHEST);
    ASSERT_NE(weathered, nullptr);
    EXPECT_EQ(weathered->getOpenSound(), SoundEvents::BLOCK_COPPER_CHEST_WEATHERED_OPEN);
    EXPECT_EQ(weathered->getCloseSound(), SoundEvents::BLOCK_COPPER_CHEST_WEATHERED_CLOSE);
}

TEST_F(CopperChestBlockTestFixture, OpenSound_OxidizedReturnsOxidizedOpen)
{
    auto* oxidized = dynamic_cast<const ChestBlock*>(VanillaBlocks::OXIDIZED_COPPER_CHEST);
    ASSERT_NE(oxidized, nullptr);
    EXPECT_EQ(oxidized->getOpenSound(), SoundEvents::BLOCK_COPPER_CHEST_OXIDIZED_OPEN);
    EXPECT_EQ(oxidized->getCloseSound(), SoundEvents::BLOCK_COPPER_CHEST_OXIDIZED_CLOSE);
}

TEST_F(CopperChestBlockTestFixture, OpenSound_WaxedVariantsReuseCorrespondingLevel)
{
    // 涂蜡变体复用对应氧化等级的声音事件
    auto* waxed = dynamic_cast<const ChestBlock*>(VanillaBlocks::WAXED_COPPER_CHEST);
    ASSERT_NE(waxed, nullptr);
    EXPECT_EQ(waxed->getOpenSound(), SoundEvents::BLOCK_COPPER_CHEST_OPEN);
    EXPECT_EQ(waxed->getCloseSound(), SoundEvents::BLOCK_COPPER_CHEST_CLOSE);

    auto* waxedExposed = dynamic_cast<const ChestBlock*>(VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST);
    ASSERT_NE(waxedExposed, nullptr);
    EXPECT_EQ(waxedExposed->getOpenSound(), SoundEvents::BLOCK_COPPER_CHEST_OPEN);
    EXPECT_EQ(waxedExposed->getCloseSound(), SoundEvents::BLOCK_COPPER_CHEST_CLOSE);

    auto* waxedWeathered = dynamic_cast<const ChestBlock*>(VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST);
    ASSERT_NE(waxedWeathered, nullptr);
    EXPECT_EQ(waxedWeathered->getOpenSound(), SoundEvents::BLOCK_COPPER_CHEST_WEATHERED_OPEN);
    EXPECT_EQ(waxedWeathered->getCloseSound(), SoundEvents::BLOCK_COPPER_CHEST_WEATHERED_CLOSE);

    auto* waxedOxidized = dynamic_cast<const ChestBlock*>(VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST);
    ASSERT_NE(waxedOxidized, nullptr);
    EXPECT_EQ(waxedOxidized->getOpenSound(), SoundEvents::BLOCK_COPPER_CHEST_OXIDIZED_OPEN);
    EXPECT_EQ(waxedOxidized->getCloseSound(), SoundEvents::BLOCK_COPPER_CHEST_OXIDIZED_CLOSE);
}

TEST_F(CopperChestBlockTestFixture, OpenSound_NormalChestReturnsDefaultChestSound)
{
    // 普通箱子使用默认 BLOCK_CHEST_OPEN/CLOSE
    ChestBlock normalChest(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    EXPECT_EQ(normalChest.getOpenSound(), SoundEvents::BLOCK_CHEST_OPEN);
    EXPECT_EQ(normalChest.getCloseSound(), SoundEvents::BLOCK_CHEST_CLOSE);
}

// ---------- 放置逻辑 ----------

TEST_F(CopperChestBlockTestFixture, Placement_SingleChestWhenNoNeighbor)
{
    CopperChestTestWorld world;
    const BlockPos pos(5, 64, 5);

    BlockItemUseContext context = makePlacementContext(world, pos, Direction::Up, 0.0f);
    const BlockState state = VanillaBlocks::COPPER_CHEST->getStateForPlacement(context);

    // 无邻居时为单箱
    EXPECT_EQ(state.get(BlockStateProperties::CHEST_TYPE()), BlockStateProperties::ChestType::Single);
}

TEST_F(CopperChestBlockTestFixture, Placement_DoubleChestTakesLowerOxidationLevel)
{
    // 测试 getLeastOxidizedChestOfConnectedBlocks 逻辑：
    // 在已存在的 Oxidized 铜箱子旁放置 Unaffected 铜箱子，
    // 合并后的方块类型应取较低氧化等级（Unaffected）
    //
    // 几何说明：
    //   - existingPos=(5,64,5) 朝南（South），其 LEFT 方向 = rotateY(South) = West
    //   - newPos=(6,64,5) 在 existingPos 的 East 侧
    //   - 新箱子也朝南，检查 West 邻居时 dir==West==rotateY(South) → chestType=Left
    CopperChestTestWorld world;
    const BlockPos existingPos(5, 64, 5);
    const BlockPos newPos(6, 64, 5);

    // 在 existingPos 放置 Oxidized 铜箱子（朝南，使其 East 侧可与新箱子合并）
    const BlockState existingState =
        VanillaBlocks::OXIDIZED_COPPER_CHEST->defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
            .with(BlockStateProperties::CHEST_TYPE(), BlockStateProperties::ChestType::Single);
    world.setBlockState(existingPos, &existingState);

    // 在 newPos 放置 Unaffected 铜箱子：玩家朝北（yaw=180），FACING=opposite(North)=South
    BlockItemUseContext context = makePlacementContext(world, newPos, Direction::Up, 180.0f);
    const BlockState newState = VanillaBlocks::COPPER_CHEST->getStateForPlacement(context);

    // 验证合并后的方块类型为 Unaffected（较低氧化等级）
    EXPECT_EQ(&newState.getBlock(), VanillaBlocks::COPPER_CHEST);
    EXPECT_NE(newState.get(BlockStateProperties::CHEST_TYPE()), BlockStateProperties::ChestType::Single);
}

// ---------- 旋转/镜像 ----------

TEST_F(CopperChestBlockTestFixture, Rotate_ChangesFacing)
{
    const BlockState& state = VanillaBlocks::COPPER_CHEST->defaultState();
    const BlockState& rotated = VanillaBlocks::COPPER_CHEST->rotate(state, Rotation::Clockwise90);

    EXPECT_EQ(rotated.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

TEST_F(CopperChestBlockTestFixture, Mirror_SwapsLeftRight)
{
    const BlockState leftState = VanillaBlocks::COPPER_CHEST->defaultState().with(
        BlockStateProperties::CHEST_TYPE(), BlockStateProperties::ChestType::Left);
    const BlockState& mirrored = VanillaBlocks::COPPER_CHEST->mirror(leftState, Mirror::LeftRight);

    // 镜像后 LEFT 变 RIGHT
    EXPECT_EQ(mirrored.get(BlockStateProperties::CHEST_TYPE()), BlockStateProperties::ChestType::Right);
}
