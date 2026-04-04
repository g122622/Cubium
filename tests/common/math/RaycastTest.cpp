#include <gtest/gtest.h>
#include "util/math/ray/Raycast.hpp"
#include "util/math/ray/Ray.hpp"
#include "core/BlockRaycastResult.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/fluid/Fluid.hpp"
#include "util/Direction.hpp"

using namespace mc;

/**
 * @brief 测试用方块读取器
 *
 * 模拟一个简单的方块世界用于测试
 */
class TestBlockReader : public IBlockReader {
public:
    // 存储方块：key = (x, y, z) 打包为 i64
    std::unordered_map<i64, const BlockState*> m_blocks;

    void clear() {
        m_blocks.clear();
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        i64 k = key(x, y, z);
        auto it = m_blocks.find(k);
        return it != m_blocks.end() ? it->second : nullptr;
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override {
        return y >= 0 && y < 256;
    }

    // IWorld 接口存根实现
    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override {
        m_blocks[key(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] i32 difficulty() const override { return 0; }

private:
    static i64 key(i32 x, i32 y, i32 z) {
        return static_cast<i64>(x) |
               (static_cast<i64>(y) << 16) |
               (static_cast<i64>(z) << 32);
    }
};

// 获取石头方块状态（需要通过BlockRegistry）
const BlockState* getStoneState() {
    static const BlockState* stoneState = nullptr;
    if (stoneState == nullptr) {
        // 从全局注册表获取石头方块
        auto* stone = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:stone"));
        stoneState = stone ? &stone->defaultState() : nullptr;
    }
    return stoneState;
}

// 注册测试需要的方块
void ensureTestBlocksRegistered() {
    static bool initialized = false;
    if (!initialized) {
        // 注册石头方块用于测试
        BlockRegistry::instance().registerBlock<SimpleBlock>(
            ResourceLocation("minecraft:stone"),
            BlockProperties(Material::ROCK).hardness(1.5f)
        );
        // 初始化原版方块，确保植被方块可用于射线测试。
        VanillaBlocks::initialize();
        initialized = true;
    }
}

const BlockState* getShortGrassState() {
    return VanillaBlocks::SHORT_GRASS ? &VanillaBlocks::SHORT_GRASS->defaultState() : nullptr;
}

const BlockState* getDandelionState() {
    return VanillaBlocks::DANDELION ? &VanillaBlocks::DANDELION->defaultState() : nullptr;
}

const BlockState* getSugarCaneState() {
    return VanillaBlocks::SUGAR_CANE ? &VanillaBlocks::SUGAR_CANE->defaultState() : nullptr;
}

/**
 * @brief 射线检测测试夹具
 */
class RaycastTest : public ::testing::Test {
protected:
    void SetUp() override {
        ensureTestBlocksRegistered();
    }

    TestBlockReader world;
};

/**
 * @brief 测试射线不命中任何方块
 */
TEST_F(RaycastTest, NoHit) {
    Ray ray(Vector3(0.0f, 64.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f).normalized());
    RaycastContext context(ray, 5.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isMiss());
    EXPECT_FALSE(result.isHit());
}

/**
 * @brief 测试射线直接命中前方方块
 */
TEST_F(RaycastTest, HitBlockDirectly) {
    world.setBlock(2, 64, 0, getStoneState());  // 在 (2, 64, 0) 放置石头

    // 从原点向X正方向射击
    Ray ray(Vector3(0.0f, 64.5f, 0.5f), Vector3(1.0f, 0.0f, 0.0f));
    RaycastContext context(ray, 5.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isHit());
    EXPECT_EQ(result.blockPos(), BlockPos(2, 64, 0));
    EXPECT_EQ(result.face(), Direction::West);  // 从西侧进入方块
}

/**
 * @brief 测试射线穿过空方块命中后方方块
 */
TEST_F(RaycastTest, HitBlockThroughAir) {
    world.setBlock(3, 64, 0, getStoneState());  // 在 (3, 64, 0) 放置石头
    // (1, 64, 0) 和 (2, 64, 0) 是空气

    // 从原点向X正方向射击
    Ray ray(Vector3(0.0f, 64.5f, 0.5f), Vector3(1.0f, 0.0f, 0.0f));
    RaycastContext context(ray, 5.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isHit());
    EXPECT_EQ(result.blockPos(), BlockPos(3, 64, 0));  // 应该命中(3, 64, 0)，而不是后面的方块
    EXPECT_EQ(result.face(), Direction::West);
}

/**
 * @brief 测试射线命中第一个方块而非后面的方块
 *
 * 这是测试修复的关键用例：确保射线检测返回第一个碰到的方块
 */
TEST_F(RaycastTest, HitFirstBlockNotSecond) {
    world.setBlock(2, 64, 0, getStoneState());  // 第一个方块
    world.setBlock(3, 64, 0, getStoneState());  // 第二个方块

    // 从原点向X正方向射击
    Ray ray(Vector3(0.0f, 64.5f, 0.5f), Vector3(1.0f, 0.0f, 0.0f));
    RaycastContext context(ray, 5.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isHit());
    EXPECT_EQ(result.blockPos(), BlockPos(2, 64, 0));  // 应该命中第一个方块
    EXPECT_EQ(result.face(), Direction::West);
}

/**
 * @brief 测试从不同方向命中方块
 */
TEST_F(RaycastTest, HitFromDifferentDirections) {
    world.setBlock(0, 64, 0, getStoneState());

    // 从东边射击
    {
        Ray ray(Vector3(2.0f, 64.5f, 0.5f), Vector3(-1.0f, 0.0f, 0.0f));
        RaycastContext context(ray, 5.0f);
        BlockRaycastResult result = raycastBlocks(context, world);

        EXPECT_TRUE(result.isHit());
        EXPECT_EQ(result.blockPos(), BlockPos(0, 64, 0));
        EXPECT_EQ(result.face(), Direction::East);  // 从东侧进入
    }

    // 从西边射击
    {
        Ray ray(Vector3(-2.0f, 64.5f, 0.5f), Vector3(1.0f, 0.0f, 0.0f));
        RaycastContext context(ray, 5.0f);
        BlockRaycastResult result = raycastBlocks(context, world);

        EXPECT_TRUE(result.isHit());
        EXPECT_EQ(result.blockPos(), BlockPos(0, 64, 0));
        EXPECT_EQ(result.face(), Direction::West);  // 从西侧进入
    }

    // 从上方射击
    {
        Ray ray(Vector3(0.5f, 67.0f, 0.5f), Vector3(0.0f, -1.0f, 0.0f));
        RaycastContext context(ray, 5.0f);
        BlockRaycastResult result = raycastBlocks(context, world);

        EXPECT_TRUE(result.isHit());
        EXPECT_EQ(result.blockPos(), BlockPos(0, 64, 0));
        EXPECT_EQ(result.face(), Direction::Up);  // 从上方进入
    }

    // 从下方射击
    {
        Ray ray(Vector3(0.5f, 62.0f, 0.5f), Vector3(0.0f, 1.0f, 0.0f));
        RaycastContext context(ray, 5.0f);
        BlockRaycastResult result = raycastBlocks(context, world);

        EXPECT_TRUE(result.isHit());
        EXPECT_EQ(result.blockPos(), BlockPos(0, 64, 0));
        EXPECT_EQ(result.face(), Direction::Down);  // 从下方进入
    }
}

/**
 * @brief 测试对角线方向命中
 */
TEST_F(RaycastTest, HitDiagonal) {
    world.setBlock(2, 64, 2, getStoneState());

    // 从(0, 64.5, 0)向(1, 0, 1)方向射击
    Vector3 dir(1.0f, 0.0f, 1.0f);
    dir = dir.normalized();
    Ray ray(Vector3(0.5f, 64.5f, 0.5f), dir);
    RaycastContext context(ray, 5.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isHit());
    EXPECT_EQ(result.blockPos(), BlockPos(2, 64, 2));
}

/**
 * @brief 测试起点在方块内部
 */
TEST_F(RaycastTest, StartInsideBlock) {
    world.setBlock(0, 64, 0, getStoneState());

    // 起点在方块内部
    Ray ray(Vector3(0.5f, 64.5f, 0.5f), Vector3(1.0f, 0.0f, 0.0f));
    RaycastContext context(ray, 5.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isHit());
    EXPECT_EQ(result.blockPos(), BlockPos(0, 64, 0));
    EXPECT_FLOAT_EQ(result.distance(), 0.0f);
}

/**
 * @brief 测试超出最大距离
 */
TEST_F(RaycastTest, ExceedMaxDistance) {
    world.setBlock(10, 64, 0, getStoneState());  // 在距离10的地方放方块

    // 最大距离5
    Ray ray(Vector3(0.0f, 64.5f, 0.5f), Vector3(1.0f, 0.0f, 0.0f));
    RaycastContext context(ray, 5.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isMiss());  // 超出最大距离，应该miss
}

/**
 * @brief 测试斜向射线命中第一个方块
 */
TEST_F(RaycastTest, DiagonalHitFirstBlock) {
    world.setBlock(2, 64, 2, getStoneState());
    world.setBlock(3, 64, 3, getStoneState());

    // 从(0, 64.5, 0)向XZ对角线方向射击
    Vector3 dir(1.0f, 0.0f, 1.0f);
    dir = dir.normalized();
    Ray ray(Vector3(0.5f, 64.5f, 0.5f), dir);
    RaycastContext context(ray, 10.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isHit());
    EXPECT_EQ(result.blockPos(), BlockPos(2, 64, 2));  // 应该命中第一个方块
}

/**
 * @brief 测试负方向射线
 */
TEST_F(RaycastTest, NegativeDirection) {
    world.setBlock(-3, 64, 0, getStoneState());

    // 向X负方向射击
    Ray ray(Vector3(0.0f, 64.5f, 0.5f), Vector3(-1.0f, 0.0f, 0.0f));
    RaycastContext context(ray, 5.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isHit());
    EXPECT_EQ(result.blockPos(), BlockPos(-3, 64, 0));
    EXPECT_EQ(result.face(), Direction::East);  // 从东侧进入
}

/**
 * @brief 测试边界情况：射线正好穿过方块边缘
 */
TEST_F(RaycastTest, EdgeCase_ExactBoundary) {
    world.setBlock(2, 64, 0, getStoneState());

    // 射线从 y=64.0（方块底部边缘）穿过
    Ray ray(Vector3(0.0f, 64.0f, 0.5f), Vector3(1.0f, 0.0f, 0.0f));
    RaycastContext context(ray, 5.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    // 由于浮点精度，可能命中(2,64,0)或(2,63,0)，取决于具体实现
    // 但不应该命中后面的方块
    if (result.isHit()) {
        EXPECT_TRUE(
            result.blockPos() == BlockPos(2, 64, 0) ||
            result.blockPos() == BlockPos(2, 63, 0)
        );
    }
}

/**
 * @brief 测试近距离命中
 */
TEST_F(RaycastTest, CloseRange) {
    world.setBlock(1, 64, 0, getStoneState());  // 紧邻的方块

    Ray ray(Vector3(0.5f, 64.5f, 0.5f), Vector3(1.0f, 0.0f, 0.0f));
    RaycastContext context(ray, 5.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isHit());
    EXPECT_EQ(result.blockPos(), BlockPos(1, 64, 0));
    EXPECT_LT(result.distance(), 1.0f);  // 距离应该小于1
}

/**
 * @brief 射线应命中短草等无碰撞植被方块
 */
TEST_F(RaycastTest, HitShortGrassBlock) {
    const BlockState* shortGrass = getShortGrassState();
    ASSERT_NE(shortGrass, nullptr);

    world.setBlock(2, 64, 0, shortGrass);

    Ray ray(Vector3(0.0f, 64.5f, 0.5f), Vector3(1.0f, 0.0f, 0.0f));
    RaycastContext context(ray, 5.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isHit());
    EXPECT_EQ(result.blockPos(), BlockPos(2, 64, 0));
}

/**
 * @brief 射线应命中花朵方块
 */
TEST_F(RaycastTest, HitFlowerBlock) {
    const BlockState* flower = getDandelionState();
    ASSERT_NE(flower, nullptr);

    world.setBlock(2, 64, 0, flower);

    // 花朵高度较低，使用较低射线高度确保穿过其形状。
    Ray ray(Vector3(0.0f, 64.2f, 0.5f), Vector3(1.0f, 0.0f, 0.0f));
    RaycastContext context(ray, 5.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isHit());
    EXPECT_EQ(result.blockPos(), BlockPos(2, 64, 0));
}

/**
 * @brief 射线应命中甘蔗方块
 */
TEST_F(RaycastTest, HitSugarCaneBlock) {
    const BlockState* sugarCane = getSugarCaneState();
    ASSERT_NE(sugarCane, nullptr);

    world.setBlock(2, 64, 0, sugarCane);

    Ray ray(Vector3(0.0f, 64.5f, 0.5f), Vector3(1.0f, 0.0f, 0.0f));
    RaycastContext context(ray, 5.0f);

    BlockRaycastResult result = raycastBlocks(context, world);

    EXPECT_TRUE(result.isHit());
    EXPECT_EQ(result.blockPos(), BlockPos(2, 64, 0));
}
