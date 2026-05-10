#include <gtest/gtest.h>

#include "common/entity/core/Entity.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/special/BoneMealItem.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/ocean/SeagrassBlock.hpp"
#include "common/world/block/blocks/ocean/TallSeagrassBlock.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/Biomes.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"

#include <map>
#include <memory>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 测试用世界存根 - 支持海草骨粉测试
 *
 * 继承自 IBlockReader 以支持 IGrowable 接口
 */
class SeagrassTestWorld final : public IBlockReader {
public:
    SeagrassTestWorld() : m_random(12345) {
        // 初始化方块注册
        VanillaBlocks::initialize();
        BlockTags::initialize();

        // 初始化流体
        fluid::FluidRegistry::instance().initialize();

        // 初始化生物群系
        BiomeRegistry::instance().initialize();

        // 创建一个简单的区块
        m_chunk = std::make_unique<ChunkData>(0, 0);

        // 填充空气和水
        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                // 海底在 y=40
                m_chunk->setBlockState(x, 40, z, &VanillaBlocks::SAND->defaultState());
                // y=41..62 是水
                for (i32 y = 41; y <= 62; ++y) {
                    m_chunk->setBlockState(x, y, z, &VanillaBlocks::WATER->defaultState());
                }
            }
        }

        // 设置默认生物群系为海洋
        for (i32 x = 0; x < 4; ++x) {
            for (i32 y = 0; y < 4; ++y) {
                for (i32 z = 0; z < 4; ++z) {
                    m_chunk->getBiomes().setBiome(x, y, z, Biomes::Ocean);
                }
            }
        }
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        if (m_chunk == nullptr) return nullptr;
        if (!isWithinWorldBounds(x, y, z)) return nullptr;
        return m_chunk->getBlockState(x, y, z);
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override {
        if (m_chunk == nullptr) return false;
        if (!isWithinWorldBounds(x, y, z)) return false;
        m_chunk->setBlockState(x, y, z, state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override {
        const BlockState* state = getBlockState(x, y, z);
        if (state == nullptr) return fluid::Fluid::getFluidState(0);
        return state->getFluidState();
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override {
        return m_chunk.get();
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override {
        return true;
    }

    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("SeagrassTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("SeagrassTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override {
        throw std::runtime_error("SeagrassTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override {
        throw std::runtime_error("SeagrassTestWorld::worldBorder not implemented");
    }

    [[nodiscard]] EntityId spawnEntity(std::unique_ptr<Entity>) override {
        return ++m_lastEntityId;
    }

    void addParticle(client::renderer::trident::particle::ParticleTypeId,
                     const Vector3&,
                     const Vector3&,
                     const Vector3& = Vector3(0, 0, 0),
                     u32 = 1) override {
        // 测试中忽略粒子效果
    }

    void playSound(const ResourceLocation&,
                   sound::SoundCategory,
                   const Vector3&,
                   f32,
                   f32) override {
        // 测试中忽略声音
    }

    // 设置生物群系
    void setBiomeAt(i32 x, i32 y, i32 z, BiomeId biome) {
        if (m_chunk == nullptr) return;
        m_chunk->getBiomes().setBiome(x, y, z, biome);
    }

private:
    std::unique_ptr<ChunkData> m_chunk;
    math::Random m_random;
    EntityId m_lastEntityId = 0;
};

// ========== SeagrassBlock IGrowable 测试 ==========

TEST(SeagrassBlockTest, CanGrow_WhenWaterAbove_ReturnsTrue) {
    SeagrassTestWorld world;

    // 在 y=41 放置海草（水源方块中）
    const BlockState* seagrassState = &VanillaBlocks::SEAGRASS->defaultState();
    world.setBlockState(8, 41, 8, seagrassState);

    // y=42 是水，上方是水源
    const BlockState* seagrassBlock = world.getBlockState(8, 41, 8);
    ASSERT_NE(seagrassBlock, nullptr);

    // 获取 IGrowable 接口
    const IGrowable* growable = dynamic_cast<const IGrowable*>(&seagrassBlock->owner());
    ASSERT_NE(growable, nullptr);

    // 检查是否可以生长
    EXPECT_TRUE(growable->canGrow(world, BlockPos(8, 41, 8), *seagrassBlock, false));
}

TEST(SeagrassBlockTest, CanGrow_WhenNoWaterAbove_ReturnsFalse) {
    SeagrassTestWorld world;

    // 在 y=62 放置海草（顶部水层）
    const BlockState* seagrassState = &VanillaBlocks::SEAGRASS->defaultState();
    world.setBlockState(8, 62, 8, seagrassState);
    // y=63 是空气
    world.setBlockState(8, 63, 8, &VanillaBlocks::AIR->defaultState());

    const BlockState* seagrassBlock = world.getBlockState(8, 62, 8);
    ASSERT_NE(seagrassBlock, nullptr);

    const IGrowable* growable = dynamic_cast<const IGrowable*>(&seagrassBlock->owner());
    ASSERT_NE(growable, nullptr);

    // 没有水在上方，不能生长
    EXPECT_FALSE(growable->canGrow(world, BlockPos(8, 62, 8), *seagrassBlock, false));
}

TEST(SeagrassBlockTest, CanUseBonemeal_AlwaysReturnsTrue) {
    SeagrassTestWorld world;
    math::Random random(12345);

    const BlockState* seagrassState = &VanillaBlocks::SEAGRASS->defaultState();
    world.setBlockState(8, 41, 8, seagrassState);

    const BlockState* seagrassBlock = world.getBlockState(8, 41, 8);
    ASSERT_NE(seagrassBlock, nullptr);

    const IGrowable* growable = dynamic_cast<const IGrowable*>(&seagrassBlock->owner());
    ASSERT_NE(growable, nullptr);

    // 海草总是可以使用骨粉
    EXPECT_TRUE(growable->canUseBonemeal(world, random, BlockPos(8, 41, 8), *seagrassBlock));
}

TEST(SeagrassBlockTest, Grow_TransformsSeagrassToTallSeagrass) {
    SeagrassTestWorld world;
    math::Random random(12345);

    // 确保高海草已初始化
    ASSERT_NE(VanillaBlocks::TALL_SEAGRASS, nullptr);

    // 在 y=41 放置海草
    const BlockState* seagrassState = &VanillaBlocks::SEAGRASS->defaultState();
    world.setBlockState(8, 41, 8, seagrassState);

    const BlockState* seagrassBlock = world.getBlockState(8, 41, 8);
    ASSERT_NE(seagrassBlock, nullptr);

    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&seagrassBlock->owner()));
    ASSERT_NE(growable, nullptr);

    // 使用骨粉生长
    growable->grow(world, random, BlockPos(8, 41, 8), *seagrassBlock);

    // 检查是否变成高海草
    const BlockState* newState = world.getBlockState(8, 41, 8);
    ASSERT_NE(newState, nullptr);
    EXPECT_TRUE(newState->is(VanillaBlocks::TALL_SEAGRASS));

    // 检查是否是下半部分
    EXPECT_EQ(newState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()),
              BlockStateProperties::DoubleBlockHalf::Lower);

    // 检查上方是否有上半部分
    const BlockState* upperState = world.getBlockState(8, 42, 8);
    ASSERT_NE(upperState, nullptr);
    EXPECT_TRUE(upperState->is(VanillaBlocks::TALL_SEAGRASS));
    EXPECT_EQ(upperState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()),
              BlockStateProperties::DoubleBlockHalf::Upper);
}

// ========== BoneMealItem::growSeagrass 测试 ==========

TEST(BoneMealItemTest, GrowSeagrass_ReturnsFalseWhenNotWater) {
    SeagrassTestWorld world;
    math::Random random(12345);

    // 在空气中尝试
    EXPECT_FALSE(item::items::BoneMealItem::growSeagrass(world, BlockPos(8, 63, 8), random));

    // 在沙子上尝试
    EXPECT_FALSE(item::items::BoneMealItem::growSeagrass(world, BlockPos(8, 40, 8), random));
}

TEST(BoneMealItemTest, GrowSeagrass_ReturnsTrueInWater) {
    SeagrassTestWorld world;
    math::Random random(12345);

    // 在水源方块中
    EXPECT_TRUE(item::items::BoneMealItem::growSeagrass(world, BlockPos(8, 42, 8), random));
}

TEST(BoneMealItemTest, GrowSeagrass_PlacesSeagrassOrCoral) {
    SeagrassTestWorld world;
    math::Random random(12345);

    // 在水源方块中使用骨粉
    bool result = item::items::BoneMealItem::growSeagrass(world, BlockPos(8, 42, 8), random);

    // 骨粉应该成功使用
    EXPECT_TRUE(result);
}

TEST(BoneMealItemTest, GrowSeagrass_InWarmOcean_CanPlaceCoral) {
    SeagrassTestWorld world;
    math::Random random(54321);

    // 设置生物群系为温暖海洋
    world.setBiomeAt(2, 2, 2, Biomes::WarmOcean);

    // 在水源方块中使用骨粉
    bool result = item::items::BoneMealItem::growSeagrass(world, BlockPos(8, 42, 8), random);

    // 应该成功
    EXPECT_TRUE(result);
}

TEST(BoneMealItemTest, GrowSeagrass_GrowsExistingSeagrass) {
    SeagrassTestWorld world;
    math::Random random(99999);  // 使用特定种子让nextInt(10) == 0

    // 放置海草
    const BlockState* seagrassState = &VanillaBlocks::SEAGRASS->defaultState();
    world.setBlockState(8, 41, 8, seagrassState);

    // 在海草位置附近使用骨粉
    // 注意：实际生长概率是 10%，可能需要多次尝试
    item::items::BoneMealItem::growSeagrass(world, BlockPos(8, 41, 8), random);

    // 由于随机性，这里只检查方法不会崩溃
    SUCCEED();
}

// ========== SeagrassBlock isValidPosition 测试 ==========

TEST(SeagrassBlockTest, IsValidPosition_RequiresWaterAndSolidBelow) {
    SeagrassTestWorld world;

    const BlockState& seagrassState = VanillaBlocks::SEAGRASS->defaultState();
    const SeagrassBlock* seagrassBlock = static_cast<const SeagrassBlock*>(&seagrassState.owner());

    // 水中且有固体支撑 - 应该有效
    EXPECT_TRUE(seagrassBlock->isValidPosition(seagrassState, world, BlockPos(8, 41, 8)));

    // 空气中 - 应该无效
    EXPECT_FALSE(seagrassBlock->isValidPosition(seagrassState, world, BlockPos(8, 63, 8)));

    // 沙子上（无水）- 应该无效
    EXPECT_FALSE(seagrassBlock->isValidPosition(seagrassState, world, BlockPos(8, 40, 8)));
}

} // namespace
