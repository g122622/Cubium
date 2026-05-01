#include <gtest/gtest.h>

#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/interactive/JukeboxEntity.hpp"
#include "world/blockentity/interactive/LecternEntity.hpp"
#include "world/chunk/ChunkData.hpp"
#include "world/tick/manager/TickManager.hpp"
#include "util/math/random/Random.hpp"

using namespace mc;
using namespace mc::blockentity;

namespace {

/**
 * @brief 按资源路径懒注册测试用物品。
 * @param path 资源路径。
 * @return 已注册物品指针。
 */
Item* ensureTestItem(const char* path) {
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }

    return &registry.registerItem(id, ItemProperties().maxStackSize(1));
}

class DummyWorld final : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlock(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 0; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("DummyWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("DummyWorld::tickManager not implemented");
    }

    // Random interface (stubbed for tests)
    [[nodiscard]] math::Random& getRandom() override {
        throw std::runtime_error("DummyWorld::getRandom not implemented");
    }
    [[nodiscard]] const math::Random& getRandom() const override {
        throw std::runtime_error("DummyWorld::getRandom not implemented");
    }
};

} // namespace

class LecternEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
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

TEST_F(LecternEntityTest, SetBook_RejectsNonBookItems) {
    LecternEntity entity(BlockPos(1, 2, 3));
    EXPECT_FALSE(entity.setBook(ItemStack(m_stick, 1)));
    EXPECT_FALSE(entity.hasBook());
}

TEST_F(LecternEntityTest, SetBook_AcceptsSupportedBookTypes) {
    LecternEntity entity(BlockPos(1, 2, 3));
    EXPECT_TRUE(entity.setBook(ItemStack(m_book, 1)));
    EXPECT_TRUE(entity.setBook(ItemStack(m_writableBook, 1)));
    EXPECT_TRUE(entity.setBook(ItemStack(m_writtenBook, 1)));
    EXPECT_TRUE(entity.setBook(ItemStack(m_enchantedBook, 1)));
}

TEST_F(LecternEntityTest, WritableBook_HasExpectedPageAndComparatorRange) {
    LecternEntity entity(BlockPos(1, 2, 3));
    ASSERT_TRUE(entity.setBook(ItemStack(m_writableBook, 1)));

    EXPECT_EQ(entity.getTotalPages(), 100);
    EXPECT_EQ(entity.getComparatorSignal(), 1);

    entity.setPage(99);
    EXPECT_EQ(entity.getPage(), 99);
    EXPECT_EQ(entity.getComparatorSignal(), 15);
}

TEST_F(LecternEntityTest, SaveLoad_PreservesBookAndPage) {
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

TEST_F(LecternEntityTest, Clone_CopiesBookAndProgress) {
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
    void SetUp() override {
        Items::initialize();
        m_disc13 = ensureTestItem("music_disc_13");
        m_discCat = ensureTestItem("music_disc_cat");
        m_stick = ensureTestItem("stick");
    }

    Item* m_disc13 = nullptr;
    Item* m_discCat = nullptr;
    Item* m_stick = nullptr;
};

TEST_F(JukeboxEntityTest, StartPlaying_OnlyWorksForMusicDisc) {
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

TEST_F(JukeboxEntityTest, SaveLoadAndClone_PreserveRecordState) {
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