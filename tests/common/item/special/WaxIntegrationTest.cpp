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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT OF LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/HoneycombItem.hpp"
#include "common/item/items/tool/AxeItem.hpp"
#include "common/item/tier/ItemTiers.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/SignBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {
namespace {

/**
 * @brief 测试用世界存根 - 支持 setBlockState/getBlockState 和事件捕获
 *
 * 继承自 BaseTestWorld，添加方块状态存储和 playEvent 捕获能力，
 * 用于测试 HoneycombItem 涂蜡和 AxeItem 除蜡行为。
 *
 * 注意：setBlockState 存储 BlockState 副本（而非指针），因为调用方
 * 传入的可能是临时 BlockState 对象（如 getWaxed() 返回值）。
 */
class WaxTestWorld final : public mc::test::BaseTestWorld {
public:
    WaxTestWorld() = default;

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override { m_events.push_back({eventId, pos, data}); }

    void playSound(const ResourceLocation& sound,
        sound::SoundCategory category,
        const Vector3& pos,
        f32 volume,
        f32 pitch) override
    {
        m_sounds.push_back({sound, category, pos, volume, pitch});
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override
    {
        if (entity != nullptr) {
            m_blockEntities[pos] = std::unique_ptr<BlockEntity>(entity);
        } else {
            m_blockEntities.erase(pos);
        }
    }

    [[nodiscard]] EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        (void)entity;
        return ++m_lastEntityId;
    }

    void addParticle(
        particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3& = Vector3(0, 0, 0), u32 = 1) override
    {
        // 测试中忽略粒子效果
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("WaxTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("WaxTestWorld::tickManager not implemented");
    }

    // 事件记录
    struct EventRecord {
        i32 eventId;
        BlockPos pos;
        i32 data;
    };

    struct SoundRecord {
        ResourceLocation sound;
        sound::SoundCategory category;
        Vector3 pos;
        f32 volume;
        f32 pitch;
    };

    [[nodiscard]] const std::vector<EventRecord>& events() const { return m_events; }
    [[nodiscard]] const std::vector<SoundRecord>& sounds() const { return m_sounds; }
    void clearEvents() { m_events.clear(); }
    void clearSounds() { m_sounds.clear(); }

private:
    // 存储 BlockState 副本，避免悬空指针问题
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::vector<EventRecord> m_events;
    std::vector<SoundRecord> m_sounds;
    EntityInstanceId m_lastEntityId = 0;
};

// ============================================================================
// 测试固件
// ============================================================================

class WaxIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    WaxTestWorld m_world;
};

// ============================================================================
// HoneycombItem::onItemUse 集成测试
// ============================================================================

TEST_F(WaxIntegrationTest, HoneycombOnItemUse_WaxesCopperBlock)
{
    // 在 (0, 64, 0) 放置铜块
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::COPPER_BLOCK->defaultState());

    // 创建蜜脾物品和上下文（无玩家 = 创造模式不消耗物品）
    ItemStack honeycombStack(Items::HONEYCOMB, 1);
    ItemUseContext context(m_world,
        nullptr,
        honeycombStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    // 使用蜜脾
    ActionResultType result = Items::HONEYCOMB->onItemUse(context);

    // 应该成功
    EXPECT_EQ(result, ActionResultType::Success);

    // 方块应变为涂蜡铜块
    const BlockState* newState = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->owner(), VanillaBlocks::WAXED_COPPER_BLOCK);

    // 应触发 WAX_ON 世界事件
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::WAX_ON);
    EXPECT_EQ(m_world.events()[0].pos, BlockPos(0, 64, 0));

    // 不应播放单独的音效（WAX_ON 事件已包含音效）
    EXPECT_TRUE(m_world.sounds().empty());
}

TEST_F(WaxIntegrationTest, HoneycombOnItemUse_PassesOnNonCopperBlock)
{
    // 在 (0, 64, 0) 放置石头
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::STONE->defaultState());

    ItemStack honeycombStack(Items::HONEYCOMB, 1);
    ItemUseContext context(m_world,
        nullptr,
        honeycombStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = Items::HONEYCOMB->onItemUse(context);

    // 石头不可涂蜡，应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);

    // 方块应保持不变
    const BlockState* state = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(&state->owner(), VanillaBlocks::STONE);

    // 不应触发任何事件或音效
    EXPECT_TRUE(m_world.events().empty());
    EXPECT_TRUE(m_world.sounds().empty());
}

TEST_F(WaxIntegrationTest, HoneycombOnItemUse_PassesOnAlreadyWaxedBlock)
{
    // 在 (0, 64, 0) 放置已涂蜡铜块
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::WAXED_COPPER_BLOCK->defaultState());

    ItemStack honeycombStack(Items::HONEYCOMB, 1);
    ItemUseContext context(m_world,
        nullptr,
        honeycombStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = Items::HONEYCOMB->onItemUse(context);

    // 已涂蜡铜块不可再次涂蜡，应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);

    // 方块应保持涂蜡铜块
    const BlockState* state = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(&state->owner(), VanillaBlocks::WAXED_COPPER_BLOCK);
}

TEST_F(WaxIntegrationTest, HoneycombOnItemUse_WaxesCutCopperStairs)
{
    // 在 (0, 64, 0) 放置切制铜楼梯
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::CUT_COPPER_STAIRS->defaultState());

    ItemStack honeycombStack(Items::HONEYCOMB, 1);
    ItemUseContext context(m_world,
        nullptr,
        honeycombStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = Items::HONEYCOMB->onItemUse(context);

    EXPECT_EQ(result, ActionResultType::Success);

    // 应变为涂蜡切制铜楼梯
    const BlockState* newState = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->owner(), VanillaBlocks::WAXED_CUT_COPPER_STAIRS);

    // 应触发 WAX_ON 事件
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::WAX_ON);
}

// ============================================================================
// AxeItem 除蜡集成测试
// ============================================================================

TEST_F(WaxIntegrationTest, AxeOnItemUse_DeWaxesWaxedCopperBlock)
{
    // 在 (0, 64, 0) 放置已涂蜡铜块
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::WAXED_COPPER_BLOCK->defaultState());

    // 创建铁斧（无玩家，避免 playSound 需要玩家的问题）
    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));
    ItemStack axeStack(&axe, 1);
    ItemUseContext context(m_world,
        nullptr,
        axeStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = axe.onItemUse(context);

    // 应该成功
    EXPECT_EQ(result, ActionResultType::Success);

    // 方块应变为未涂蜡铜块
    const BlockState* newState = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->owner(), VanillaBlocks::COPPER_BLOCK);

    // 应触发 WAX_OFF 世界事件
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::WAX_OFF);
    EXPECT_EQ(m_world.events()[0].pos, BlockPos(0, 64, 0));

    // 不应播放单独的音效（WAX_OFF 事件已包含音效，避免双重音效）
    EXPECT_TRUE(m_world.sounds().empty());
}

TEST_F(WaxIntegrationTest, AxeOnItemUse_DeWaxesWaxedExposedCopper)
{
    // 在 (0, 64, 0) 放置已涂蜡斑驳铜块
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::WAXED_EXPOSED_COPPER->defaultState());

    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));
    ItemStack axeStack(&axe, 1);
    ItemUseContext context(m_world,
        nullptr,
        axeStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = axe.onItemUse(context);

    EXPECT_EQ(result, ActionResultType::Success);

    // 应变为斑驳铜块（未涂蜡）
    const BlockState* newState = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->owner(), VanillaBlocks::EXPOSED_COPPER);

    // 应触发 WAX_OFF 事件
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::WAX_OFF);
}

TEST_F(WaxIntegrationTest, AxeOnItemUse_PassesOnUnwaxedCopper)
{
    // 在 (0, 64, 0) 放置铜块（Unaffected等级，不可去皮、不可去氧化、不可除蜡）
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::COPPER_BLOCK->defaultState());

    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));
    ItemStack axeStack(&axe, 1);
    ItemUseContext context(m_world,
        nullptr,
        axeStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = axe.onItemUse(context);

    // Unaffected铜块不可去皮、不可去氧化（已是最低等级）、不可除蜡，应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);

    // 不应触发任何事件
    EXPECT_TRUE(m_world.events().empty());
    EXPECT_TRUE(m_world.sounds().empty());
}

TEST_F(WaxIntegrationTest, AxeOnItemUse_StripsLogBeforeDeWaxing)
{
    // 放置橡木原木，测试去皮优先于除蜡
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::OAK_LOG->defaultState());

    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));
    ItemStack axeStack(&axe, 1);
    ItemUseContext context(m_world,
        nullptr,
        axeStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = axe.onItemUse(context);

    EXPECT_EQ(result, ActionResultType::Success);

    // 应变为去皮橡木原木
    const BlockState* newState = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->owner(), VanillaBlocks::STRIPPED_OAK_LOG);

    // 去皮不触发 levelEvent（仅 playSound，但无玩家时不播放）
    EXPECT_TRUE(m_world.events().empty());
}

// ============================================================================
// 涂蜡→除蜡 往返测试
// ============================================================================

TEST_F(WaxIntegrationTest, WaxThenDeWax_RoundTrip)
{
    // 先涂蜡
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::COPPER_BLOCK->defaultState());

    ItemStack honeycombStack(Items::HONEYCOMB, 1);
    ItemUseContext waxContext(m_world,
        nullptr,
        honeycombStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType waxResult = Items::HONEYCOMB->onItemUse(waxContext);
    EXPECT_EQ(waxResult, ActionResultType::Success);

    // 验证涂蜡后是 WAXED_COPPER_BLOCK
    const BlockState* waxedState = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(waxedState, nullptr);
    EXPECT_EQ(&waxedState->owner(), VanillaBlocks::WAXED_COPPER_BLOCK);

    // 清除事件记录
    m_world.clearEvents();
    m_world.clearSounds();

    // 再除蜡
    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));
    ItemStack axeStack(&axe, 1);
    ItemUseContext dewaxContext(m_world,
        nullptr,
        axeStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType dewaxResult = axe.onItemUse(dewaxContext);
    EXPECT_EQ(dewaxResult, ActionResultType::Success);

    // 验证除蜡后恢复为 COPPER_BLOCK
    const BlockState* unwaxedState = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(unwaxedState, nullptr);
    EXPECT_EQ(&unwaxedState->owner(), VanillaBlocks::COPPER_BLOCK);

    // 应触发 WAX_OFF 事件
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::WAX_OFF);
}

// ============================================================================
// BeehiveBlock::dropHoneycomb 集成测试
// ============================================================================

TEST_F(WaxIntegrationTest, HoneycombItemIsRegistered)
{
    // 验证 HONEYCOMB 物品已正确注册（BeehiveBlock::dropHoneycomb 依赖此物品）
    ASSERT_NE(Items::HONEYCOMB, nullptr);
    EXPECT_EQ(Items::HONEYCOMB->itemLocation(), ResourceLocation("minecraft:honeycomb"));
    EXPECT_EQ(Items::HONEYCOMB->maxStackSize(), 64);
}

TEST_F(WaxIntegrationTest, HoneycombItemIsHoneycombItemType)
{
    // 验证 HONEYCOMB 是 HoneycombItem 实例
    auto* honeycombItem = dynamic_cast<item::items::HoneycombItem*>(Items::HONEYCOMB);
    ASSERT_NE(honeycombItem, nullptr);
}

TEST_F(WaxIntegrationTest, DeWaxNoDoubleSound)
{
    // 回归测试：AxeItem 除蜡不应产生双重音效
    // 之前 AxeItem 除蜡同时调用了 playSound 和 playEvent(WAX_OFF)，
    // 但 WAX_OFF 事件（ID 3004）已包含音效和粒子，playSound 会导致双重音效。

    m_world.setBlockState(0, 64, 0, &VanillaBlocks::WAXED_COPPER_BLOCK->defaultState());

    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));
    ItemStack axeStack(&axe, 1);
    ItemUseContext context(m_world,
        nullptr,
        axeStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = axe.onItemUse(context);
    EXPECT_EQ(result, ActionResultType::Success);

    // 关键断言：除蜡不应播放单独音效，仅通过 playEvent(WAX_OFF) 触发音效+粒子
    EXPECT_TRUE(m_world.sounds().empty()) << "AxeItem de-waxing should not call playSound() - "
                                             "WAX_OFF event already includes audio";
    // 应有 WAX_OFF 事件
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::WAX_OFF);
}

// ============================================================================
// AxeItem 去氧化（刮削）集成测试
// ============================================================================

TEST_F(WaxIntegrationTest, AxeOnItemUse_ScrapesExposedCopper)
{
    // 在 (0, 64, 0) 放置斑驳铜块（Exposed等级）
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::EXPOSED_COPPER->defaultState());

    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));
    ItemStack axeStack(&axe, 1);
    ItemUseContext context(m_world,
        nullptr,
        axeStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = axe.onItemUse(context);

    // 应该成功
    EXPECT_EQ(result, ActionResultType::Success);

    // 方块应降级为铜块（Unaffected等级）
    const BlockState* newState = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->owner(), VanillaBlocks::COPPER_BLOCK);

    // 应触发 SCRAPE 世界事件
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::SCRAPE);
    EXPECT_EQ(m_world.events()[0].pos, BlockPos(0, 64, 0));
}

TEST_F(WaxIntegrationTest, AxeOnItemUse_ScrapesWeatheredCopper)
{
    // 在 (0, 64, 0) 放置锈蚀铜块（Weathered等级）
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::WEATHERED_COPPER->defaultState());

    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));
    ItemStack axeStack(&axe, 1);
    ItemUseContext context(m_world,
        nullptr,
        axeStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = axe.onItemUse(context);

    EXPECT_EQ(result, ActionResultType::Success);

    // 方块应降级为斑驳铜块（Exposed等级）
    const BlockState* newState = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->owner(), VanillaBlocks::EXPOSED_COPPER);

    // 应触发 SCRAPE 事件
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::SCRAPE);
}

TEST_F(WaxIntegrationTest, AxeOnItemUse_ScrapesOxidizedCopper)
{
    // 在 (0, 64, 0) 放置氧化铜块（Oxidized等级）
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::OXIDIZED_COPPER->defaultState());

    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));
    ItemStack axeStack(&axe, 1);
    ItemUseContext context(m_world,
        nullptr,
        axeStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = axe.onItemUse(context);

    EXPECT_EQ(result, ActionResultType::Success);

    // 方块应降级为锈蚀铜块（Weathered等级）
    const BlockState* newState = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->owner(), VanillaBlocks::WEATHERED_COPPER);

    // 应触发 SCRAPE 事件
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::SCRAPE);
}

TEST_F(WaxIntegrationTest, AxeOnItemUse_CannotScrapeUnaffectedCopper)
{
    // 在 (0, 64, 0) 放置铜块（Unaffected等级，已是最低等级，不可去氧化）
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::COPPER_BLOCK->defaultState());

    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));
    ItemStack axeStack(&axe, 1);
    ItemUseContext context(m_world,
        nullptr,
        axeStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = axe.onItemUse(context);

    // Unaffected等级没有上一级，应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);

    // 不应触发任何事件
    EXPECT_TRUE(m_world.events().empty());
    EXPECT_TRUE(m_world.sounds().empty());
}

TEST_F(WaxIntegrationTest, AxeOnItemUse_ScrapingPriorityOverDeWaxing)
{
    // MC Java 交互顺序：去皮 → 去氧化（刮削）→ 除蜡
    // 斑驳铜块（Exposed，未涂蜡）应被刮削而非除蜡
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::EXPOSED_COPPER->defaultState());

    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));
    ItemStack axeStack(&axe, 1);
    ItemUseContext context(m_world,
        nullptr,
        axeStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = axe.onItemUse(context);

    EXPECT_EQ(result, ActionResultType::Success);

    // 应降级为铜块（Unaffected），而非尝试除蜡
    const BlockState* newState = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->owner(), VanillaBlocks::COPPER_BLOCK);

    // 应触发 SCRAPE 事件（而非 WAX_OFF）
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::SCRAPE);
}

TEST_F(WaxIntegrationTest, AxeOnItemUse_DeWaxBeforeScrapeOnWaxedBlock)
{
    // 涂蜡斑驳铜块应该先被除蜡（因为刮削检查在涂蜡方块上不匹配）
    // 涂蜡方块不是 IOxidizableBlock，所以 getPreviousOxidationBlock 不会命中
    // 只有除蜡检查能匹配涂蜡方块
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::WAXED_EXPOSED_COPPER->defaultState());

    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));
    ItemStack axeStack(&axe, 1);
    ItemUseContext context(m_world,
        nullptr,
        axeStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = axe.onItemUse(context);

    EXPECT_EQ(result, ActionResultType::Success);

    // 应变为斑驳铜块（未涂蜡），而非进一步降级
    const BlockState* newState = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->owner(), VanillaBlocks::EXPOSED_COPPER);

    // 应触发 WAX_OFF 事件（除蜡，非刮削）
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::WAX_OFF);
}

TEST_F(WaxIntegrationTest, AxeOnItemUse_ScrapesCutCopperStairs)
{
    // 测试切制铜楼梯的刮削（验证 withPropertiesOf 保留楼梯朝向等属性）
    const BlockState& exposedStairState = VanillaBlocks::EXPOSED_CUT_COPPER_STAIRS->defaultState();
    m_world.setBlockState(0, 64, 0, &exposedStairState);

    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));
    ItemStack axeStack(&axe, 1);
    ItemUseContext context(m_world,
        nullptr,
        axeStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = axe.onItemUse(context);

    EXPECT_EQ(result, ActionResultType::Success);

    // 应降级为切制铜楼梯（Unaffected等级）
    const BlockState* newState = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(&newState->owner(), VanillaBlocks::CUT_COPPER_STAIRS);

    // 应触发 SCRAPE 事件
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::SCRAPE);
}

TEST_F(WaxIntegrationTest, ScrapeThenScrape_MultipleScrapingSteps)
{
    // 测试连续刮削：氧化铜 → 锈蚀 → 斑驳 → 铜
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::OXIDIZED_COPPER->defaultState());

    item::tool::AxeItem axe(item::tier::ItemTiers::IRON(), 6.0f, -3.0f, ItemProperties().maxDamage(250));

    // 第一次刮削：Oxidized → Weathered
    ItemStack axeStack1(&axe, 1);
    ItemUseContext context1(m_world,
        nullptr,
        axeStack1,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);
    EXPECT_EQ(axe.onItemUse(context1), ActionResultType::Success);
    EXPECT_EQ(&m_world.getBlockState(0, 64, 0)->owner(), VanillaBlocks::WEATHERED_COPPER);

    m_world.clearEvents();

    // 第二次刮削：Weathered → Exposed
    ItemStack axeStack2(&axe, 1);
    ItemUseContext context2(m_world,
        nullptr,
        axeStack2,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);
    EXPECT_EQ(axe.onItemUse(context2), ActionResultType::Success);
    EXPECT_EQ(&m_world.getBlockState(0, 64, 0)->owner(), VanillaBlocks::EXPOSED_COPPER);

    m_world.clearEvents();

    // 第三次刮削：Exposed → Copper (Unaffected)
    ItemStack axeStack3(&axe, 1);
    ItemUseContext context3(m_world,
        nullptr,
        axeStack3,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);
    EXPECT_EQ(axe.onItemUse(context3), ActionResultType::Success);
    EXPECT_EQ(&m_world.getBlockState(0, 64, 0)->owner(), VanillaBlocks::COPPER_BLOCK);

    m_world.clearEvents();

    // 第四次刮削：Copper (Unaffected) 无法再降级
    ItemStack axeStack4(&axe, 1);
    ItemUseContext context4(m_world,
        nullptr,
        axeStack4,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);
    EXPECT_EQ(axe.onItemUse(context4), ActionResultType::Pass);
}

// ============================================================================
// HoneycombItem 告示牌涂蜡集成测试
// ============================================================================

TEST_F(WaxIntegrationTest, HoneycombOnItemUse_WaxesSignEntity)
{
    // 在 (0, 64, 0) 放置告示牌并创建 SignEntity
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::OAK_SIGN->defaultState());
    auto signEntity = std::make_unique<blockentity::SignEntity>(BlockPos(0, 64, 0));
    signEntity->setLineFromLegacy(0, "Hello World");
    blockentity::SignEntity* signPtr = signEntity.get();
    m_world.setBlockEntity(BlockPos(0, 64, 0), signEntity.release());

    // 使用蜜脾
    ItemStack honeycombStack(Items::HONEYCOMB, 1);
    ItemUseContext context(m_world,
        nullptr,
        honeycombStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = Items::HONEYCOMB->onItemUse(context);

    // 应该成功
    EXPECT_EQ(result, ActionResultType::Success);

    // SignEntity 应被涂蜡
    ASSERT_NE(signPtr, nullptr);
    EXPECT_TRUE(signPtr->isWaxed());

    // 应触发 WAX_ON 世界事件
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::WAX_ON);
    EXPECT_EQ(m_world.events()[0].pos, BlockPos(0, 64, 0));
}

TEST_F(WaxIntegrationTest, HoneycombOnItemUse_SignAlreadyWaxed_ReturnsPass)
{
    // 在 (0, 64, 0) 放置告示牌并创建已涂蜡的 SignEntity
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::OAK_SIGN->defaultState());
    auto signEntity = std::make_unique<blockentity::SignEntity>(BlockPos(0, 64, 0));
    signEntity->setWaxed(true);
    m_world.setBlockEntity(BlockPos(0, 64, 0), signEntity.release());

    // 使用蜜脾
    ItemStack honeycombStack(Items::HONEYCOMB, 1);
    ItemUseContext context(m_world,
        nullptr,
        honeycombStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = Items::HONEYCOMB->onItemUse(context);

    // 已涂蜡的告示牌应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);

    // 不应触发任何事件
    EXPECT_TRUE(m_world.events().empty());
}

TEST_F(WaxIntegrationTest, HoneycombOnItemUse_SignWithoutBlockEntity_ReturnsPass)
{
    // 在 (0, 64, 0) 放置告示牌但不创建 SignEntity
    m_world.setBlockState(0, 64, 0, &VanillaBlocks::OAK_SIGN->defaultState());

    // 使用蜜脾
    ItemStack honeycombStack(Items::HONEYCOMB, 1);
    ItemUseContext context(m_world,
        nullptr,
        honeycombStack,
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = Items::HONEYCOMB->onItemUse(context);

    // 没有 BlockEntity，应跳过告示牌涂蜡，尝试铜块涂蜡
    // 告示牌不是铜块，应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);
}

TEST_F(WaxIntegrationTest, HoneycombOnItemUse_WaxedSignPreventsTextModification)
{
    // 创建并涂蜡 SignEntity
    auto signEntity = std::make_unique<blockentity::SignEntity>(BlockPos(0, 64, 0));
    signEntity->setLineFromLegacy(0, "Original");
    signEntity->setWaxed(true);
    m_world.setBlockEntity(BlockPos(0, 64, 0), signEntity.release());

    // 获取 SignEntity 指针
    BlockEntity* be = m_world.getBlockEntity(BlockPos(0, 64, 0));
    ASSERT_NE(be, nullptr);
    auto* sign = static_cast<blockentity::SignEntity*>(be);

    // 涂蜡后不应允许修改文字
    EXPECT_FALSE(sign->setLineFromLegacy(0, "Modified"));
    EXPECT_EQ(sign->getLineText(0), "Original");

    EXPECT_FALSE(sign->setLine(0, std::make_unique<text::StringTextComponent>("New")));
    EXPECT_EQ(sign->getLineText(0), "Original");

    sign->clearLines();
    EXPECT_EQ(sign->getLineText(0), "Original");
}

// ============================================================================
// AbstractSignBlock::onBlockActivated 蜜脾涂蜡集成测试
// ============================================================================

TEST_F(WaxIntegrationTest, SignBlock_OnBlockActivated_WaxesSignWithHoneycomb)
{
    // 在 (1, 64, 0) 放置告示牌并创建 SignEntity
    m_world.setBlockState(1, 64, 0, &VanillaBlocks::OAK_SIGN->defaultState());
    auto signEntity = std::make_unique<blockentity::SignEntity>(BlockPos(1, 64, 0));
    blockentity::SignEntity* signPtr = signEntity.get();
    m_world.setBlockEntity(BlockPos(1, 64, 0), signEntity.release());

    // 设置告示牌文本
    signPtr->setLineFromLegacy(0, "Hello");

    // 创建玩家并设置手持蜜脾
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.inventory().getSelectedStackRef() = ItemStack(Items::HONEYCOMB, 5);

    // 调用 onBlockActivated（直接使用方块指针，因为 onBlockActivated 不是 const 方法）
    const BlockState& state = VanillaBlocks::OAK_SIGN->defaultState();
    BlockRaycastResult hit;
    auto result =
        VanillaBlocks::OAK_SIGN->onBlockActivated(state, m_world, BlockPos(1, 64, 0), player, Hand::MainHand, hit);

    // 应该成功
    EXPECT_EQ(result, ActionResultType::Success);

    // SignEntity 应被涂蜡
    EXPECT_TRUE(signPtr->isWaxed());

    // 应触发 WAX_ON 世界事件
    ASSERT_EQ(m_world.events().size(), 1u);
    EXPECT_EQ(m_world.events()[0].eventId, world::WorldEvents::WAX_ON);
    EXPECT_EQ(m_world.events()[0].pos, BlockPos(1, 64, 0));

    // 非创造模式下应消耗一个蜜脾
    ItemStack& heldItem = player.inventory().getSelectedStackRef();
    EXPECT_EQ(heldItem.getCount(), 4);
}

TEST_F(WaxIntegrationTest, SignBlock_OnBlockActivated_CreativeModeDoesNotConsumeHoneycomb)
{
    // 在 (1, 64, 0) 放置告示牌并创建 SignEntity
    m_world.setBlockState(1, 64, 0, &VanillaBlocks::OAK_SIGN->defaultState());
    auto signEntity = std::make_unique<blockentity::SignEntity>(BlockPos(1, 64, 0));
    m_world.setBlockEntity(BlockPos(1, 64, 0), signEntity.release());

    // 创建创造模式玩家并设置手持蜜脾
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.abilities().creativeMode = true;
    player.inventory().getSelectedStackRef() = ItemStack(Items::HONEYCOMB, 5);

    // 调用 onBlockActivated
    const BlockState& state = VanillaBlocks::OAK_SIGN->defaultState();
    BlockRaycastResult hit;
    auto result =
        VanillaBlocks::OAK_SIGN->onBlockActivated(state, m_world, BlockPos(1, 64, 0), player, Hand::MainHand, hit);

    // 应该成功
    EXPECT_EQ(result, ActionResultType::Success);

    // 创造模式下不应消耗蜜脾
    ItemStack& heldItem = player.inventory().getSelectedStackRef();
    EXPECT_EQ(heldItem.getCount(), 5);
}

TEST_F(WaxIntegrationTest, SignBlock_OnBlockActivated_AlreadyWaxedReturnsConsume)
{
    // 在 (1, 64, 0) 放置告示牌并创建已涂蜡的 SignEntity
    m_world.setBlockState(1, 64, 0, &VanillaBlocks::OAK_SIGN->defaultState());
    auto signEntity = std::make_unique<blockentity::SignEntity>(BlockPos(1, 64, 0));
    signEntity->setWaxed(true);
    m_world.setBlockEntity(BlockPos(1, 64, 0), signEntity.release());

    // 创建玩家并设置手持蜜脾
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.inventory().getSelectedStackRef() = ItemStack(Items::HONEYCOMB, 5);

    // 调用 onBlockActivated
    const BlockState& state = VanillaBlocks::OAK_SIGN->defaultState();
    BlockRaycastResult hit;
    auto result =
        VanillaBlocks::OAK_SIGN->onBlockActivated(state, m_world, BlockPos(1, 64, 0), player, Hand::MainHand, hit);

    // 已涂蜡告示牌应返回 Consume（防止蜜脾被放置）
    EXPECT_EQ(result, ActionResultType::Consume);

    // 不应消耗蜜脾
    ItemStack& heldItem = player.inventory().getSelectedStackRef();
    EXPECT_EQ(heldItem.getCount(), 5);

    // 不应触发事件
    EXPECT_TRUE(m_world.events().empty());
}

TEST_F(WaxIntegrationTest, SignBlock_OnBlockActivated_NoBlockEntityReturnsPass)
{
    // 在 (1, 64, 0) 放置告示牌但不创建 SignEntity
    m_world.setBlockState(1, 64, 0, &VanillaBlocks::OAK_SIGN->defaultState());

    // 创建玩家并设置手持蜜脾
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.inventory().getSelectedStackRef() = ItemStack(Items::HONEYCOMB, 5);

    // 调用 onBlockActivated（直接使用方块指针，因为 onBlockActivated 不是 const 方法）
    const BlockState& state = VanillaBlocks::OAK_SIGN->defaultState();
    BlockRaycastResult hit;
    auto result =
        VanillaBlocks::OAK_SIGN->onBlockActivated(state, m_world, BlockPos(1, 64, 0), player, Hand::MainHand, hit);

    // 没有 BlockEntity，应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);

    // 不应消耗蜜脾
    ItemStack& heldItem = player.inventory().getSelectedStackRef();
    EXPECT_EQ(heldItem.getCount(), 5);
}

TEST_F(WaxIntegrationTest, SignBlock_OnBlockActivated_NonHoneycombItemExecutesCommand)
{
    // 在 (1, 64, 0) 放置告示牌并创建 SignEntity
    m_world.setBlockState(1, 64, 0, &VanillaBlocks::OAK_SIGN->defaultState());
    auto signEntity = std::make_unique<blockentity::SignEntity>(BlockPos(1, 64, 0));
    m_world.setBlockEntity(BlockPos(1, 64, 0), signEntity.release());

    // 创建玩家但不手持蜜脾（空手）
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 调用 onBlockActivated（直接使用方块指针，因为 onBlockActivated 不是 const 方法）
    const BlockState& state = VanillaBlocks::OAK_SIGN->defaultState();
    BlockRaycastResult hit;
    auto result =
        VanillaBlocks::OAK_SIGN->onBlockActivated(state, m_world, BlockPos(1, 64, 0), player, Hand::MainHand, hit);

    // 空手交互告示牌应返回 Success（执行命令交互）
    EXPECT_EQ(result, ActionResultType::Success);

    // 不应触发 WAX_ON 事件
    EXPECT_TRUE(m_world.events().empty());
}

} // namespace
} // namespace mc
