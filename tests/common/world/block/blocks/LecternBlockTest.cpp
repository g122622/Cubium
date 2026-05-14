#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "core/Constants.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/blocks/functional/LecternBlock.hpp"
#include "world/blockentity/interactive/LecternEntity.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/chunk/ChunkData.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blocks;

namespace {

class LecternTestWorld final : public test::BaseTestWorld {
public:
    using IWorld::getBlockState;

public:
    explicit LecternTestWorld(LecternBlock& block)
        : m_state(block.defaultState())
    {}

    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return &m_state; }

    bool setBlockState(i32, i32, i32, const BlockState* state) override
    {
        if (state == nullptr) {
            return false;
        }
        m_state = *state;
        ++m_setBlockCalls;
        return true;
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_entities.find(pos);
        return it == m_entities.end() ? nullptr : it->second.get();
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_entities.find(pos);
        return it == m_entities.end() ? nullptr : it->second.get();
    }

    void setOwnedBlockEntity(std::unique_ptr<BlockEntity> entity)
    {
        const BlockPos pos = entity->getPos();
        m_entities[pos] = std::move(entity);
    }

    void scheduleBlockTick(const BlockPos&,
        Block&,
        i32 delay,
        world::tick::TickPriority priority = world::tick::TickPriority::Normal) override
    {
        m_lastScheduledDelay = delay;
        m_lastScheduledPriority = priority;
        ++m_scheduleCalls;
    }

    [[nodiscard]] i32 setBlockCalls() const { return m_setBlockCalls; }
    [[nodiscard]] i32 scheduleCalls() const { return m_scheduleCalls; }
    [[nodiscard]] i32 lastScheduledDelay() const { return m_lastScheduledDelay; }
    [[nodiscard]] world::tick::TickPriority lastScheduledPriority() const { return m_lastScheduledPriority; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("LecternTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("LecternTestWorld::tickManager not implemented");
    }

private:
    BlockState m_state;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_entities;
    i32 m_setBlockCalls = 0;
    i32 m_scheduleCalls = 0;
    i32 m_lastScheduledDelay = -1;
    world::tick::TickPriority m_lastScheduledPriority = world::tick::TickPriority::Normal;
};

Item* ensureTestItem(const char* path)
{
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }

    return &registry.registerItem(id, ItemProperties().maxStackSize(1));
}

} // namespace

class LecternBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_book = ensureTestItem("book");
        m_stick = ensureTestItem("stick");
        m_block = std::make_unique<LecternBlock>(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    }

    Item* m_book = nullptr;
    Item* m_stick = nullptr;
    std::unique_ptr<LecternBlock> m_block;
};

TEST_F(LecternBlockTest, TryPlaceBook_RejectsNonBook)
{
    BlockState state = m_block->defaultState();
    LecternTestWorld world(*m_block);
    const BlockPos pos(1, 2, 3);

    ASSERT_FALSE(LecternBlock::tryPlaceBook(world, pos, state, m_stick->itemId()));
    EXPECT_EQ(world.setBlockCalls(), 0);
}

TEST_F(LecternBlockTest, TryPlaceBook_AcceptsBookAndSetsState)
{
    BlockState state = m_block->defaultState();
    LecternTestWorld world(*m_block);
    const BlockPos pos(1, 2, 3);

    ASSERT_TRUE(LecternBlock::tryPlaceBook(world, pos, state, m_book->itemId()));
    EXPECT_EQ(world.setBlockCalls(), 1);

    const BlockState* applied = world.getBlockState(pos);
    ASSERT_NE(applied, nullptr);
    EXPECT_TRUE(applied->get(BlockStateProperties::HAS_BOOK()));
}

TEST_F(LecternBlockTest, ComparatorInput_UsesLecternEntitySignal)
{
    BlockState state = m_block->defaultState().with(BlockStateProperties::HAS_BOOK(), true);
    LecternTestWorld world(*m_block);
    const BlockPos pos(4, 5, 6);

    auto lecternEntity = std::make_unique<blockentity::LecternEntity>(pos);
    ASSERT_TRUE(lecternEntity->setBook(ItemStack(m_book, 1)));
    lecternEntity->setPage(0);
    world.setOwnedBlockEntity(std::move(lecternEntity));

    const int signal = m_block->getComparatorInputOverride(state, world, pos);
    EXPECT_EQ(signal, 1);
}

TEST_F(LecternBlockTest, Pulse_SetsPoweredAndSchedulesTick)
{
    BlockState state = m_block->defaultState().with(BlockStateProperties::HAS_BOOK(), true);
    LecternTestWorld world(*m_block);
    const BlockPos pos(8, 9, 10);

    LecternBlock::pulse(world, pos, state);

    EXPECT_EQ(world.setBlockCalls(), 1);
    EXPECT_EQ(world.scheduleCalls(), 1);
    EXPECT_EQ(world.lastScheduledDelay(), 2);
    EXPECT_EQ(world.lastScheduledPriority(), world::tick::TickPriority::High);
}
