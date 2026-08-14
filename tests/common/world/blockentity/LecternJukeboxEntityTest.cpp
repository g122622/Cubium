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
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/interactive/JukeboxEntity.hpp"
#include "world/blockentity/interactive/LecternEntity.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/chunk/data/ChunkData.hpp"
#include "world/tick/manager/TickManager.hpp"

using namespace mc;
using namespace mc::blockentity;

namespace {

/**
 * @brief 按资源路径懒注册测试用物品。
 * @param path 资源路径。
 * @return 已注册物品指针。
 */
Item* ensureTestItem(const char* path)
{
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }

    return &registry.registerItem(id, ItemProperties().maxStackSize(1));
}

class DummyWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("DummyWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("DummyWorld::tickManager not implemented");
    }
};

} // namespace

class LecternEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_book = ensureTestItem("book");
        m_writableBook = ensureTestItem("writable_book");
        m_writtenBook = ensureTestItem("written_book");
        m_enchantedBook = ensureTestItem("enchanted_book");
        m_stick = ensureTestItem("stick");
    }

    Item* m_book = nullptr;
    Item* m_writableBook = nullptr;
    Item* m_writtenBook = nullptr;
    Item* m_enchantedBook = nullptr;
    Item* m_stick = nullptr;
};

TEST_F(LecternEntityTest, SetBook_RejectsNonBookItems)
{
    LecternEntity entity(BlockPos(1, 2, 3));
    EXPECT_FALSE(entity.setBook(ItemStack(m_stick, 1)));
    EXPECT_FALSE(entity.hasBook());
}

TEST_F(LecternEntityTest, SetBook_AcceptsSupportedBookTypes)
{
    LecternEntity entity(BlockPos(1, 2, 3));
    EXPECT_TRUE(entity.setBook(ItemStack(m_book, 1)));
    EXPECT_TRUE(entity.setBook(ItemStack(m_writableBook, 1)));
    EXPECT_TRUE(entity.setBook(ItemStack(m_writtenBook, 1)));
    EXPECT_TRUE(entity.setBook(ItemStack(m_enchantedBook, 1)));
}

TEST_F(LecternEntityTest, WritableBook_HasExpectedPageAndComparatorRange)
{
    LecternEntity entity(BlockPos(1, 2, 3));
    ASSERT_TRUE(entity.setBook(ItemStack(m_writableBook, 1)));

    EXPECT_EQ(entity.getTotalPages(), 100);
    EXPECT_EQ(entity.getComparatorSignal(), 1);

    entity.setPage(99);
    EXPECT_EQ(entity.getPage(), 99);
    EXPECT_EQ(entity.getComparatorSignal(), 15);
}

TEST_F(LecternEntityTest, SaveLoad_PreservesBookAndPage)
{
    LecternEntity original(BlockPos(4, 5, 6));
    ASSERT_TRUE(original.setBook(ItemStack(m_writtenBook, 1)));
    original.setPage(24);

    nlohmann::json data;
    original.save(data);

    LecternEntity loaded(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(data));

    EXPECT_TRUE(loaded.hasBook());
    EXPECT_EQ(loaded.getPage(), 24);
    EXPECT_EQ(loaded.getBook().getItem(), m_writtenBook);
}

TEST_F(LecternEntityTest, Clone_CopiesBookAndProgress)
{
    LecternEntity original(BlockPos(7, 8, 9));
    ASSERT_TRUE(original.setBook(ItemStack(m_enchantedBook, 1)));
    original.openContainer();

    std::unique_ptr<BlockEntity> copyBase = original.clone();
    ASSERT_NE(copyBase, nullptr);
    ASSERT_EQ(copyBase->getType(), BlockEntityType::Lectern);

    auto* copy = static_cast<LecternEntity*>(copyBase.get());
    EXPECT_TRUE(copy->hasBook());
    EXPECT_EQ(copy->getBook().getItem(), m_enchantedBook);
    EXPECT_EQ(copy->getOpenCount(), 1);
}

class JukeboxEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_disc13 = ensureTestItem("music_disc_13");
        m_discCat = ensureTestItem("music_disc_cat");
        m_stick = ensureTestItem("stick");
    }

    Item* m_disc13 = nullptr;
    Item* m_discCat = nullptr;
    Item* m_stick = nullptr;
};

TEST_F(JukeboxEntityTest, StartPlaying_OnlyWorksForMusicDisc)
{
    DummyWorld world;
    JukeboxEntity entity(BlockPos(1, 2, 3));
    entity.setRecord(ItemStack(m_stick, 1));

    entity.startPlaying(world);
    EXPECT_FALSE(entity.isPlaying());
    EXPECT_EQ(entity.getComparatorSignal(), 0);

    entity.setRecord(ItemStack(m_disc13, 1));
    entity.startPlaying(world);
    EXPECT_TRUE(entity.isPlaying());
    EXPECT_GT(entity.getComparatorSignal(), 0);
}

TEST_F(JukeboxEntityTest, SaveLoadAndClone_PreserveRecordState)
{
    JukeboxEntity original(BlockPos(3, 4, 5));
    original.setRecord(ItemStack(m_discCat, 1));

    nlohmann::json data;
    original.save(data);

    JukeboxEntity loaded(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(data));
    EXPECT_TRUE(loaded.hasRecord());
    EXPECT_EQ(loaded.getRecord().getItem(), m_discCat);

    std::unique_ptr<BlockEntity> copyBase = loaded.clone();
    ASSERT_NE(copyBase, nullptr);
    ASSERT_EQ(copyBase->getType(), BlockEntityType::Jukebox);

    auto* copy = static_cast<JukeboxEntity*>(copyBase.get());
    EXPECT_TRUE(copy->hasRecord());
    EXPECT_EQ(copy->getRecord().getItem(), m_discCat);
}
