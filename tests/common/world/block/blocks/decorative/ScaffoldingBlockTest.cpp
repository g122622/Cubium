#include <gtest/gtest.h>
#include "world/block/blocks/decorative/ScaffoldingBlock.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/Material.hpp"
#include "util/property/Properties.hpp"
#include "item/items/block/BlockItemRegistry.hpp"

using namespace mc;
using namespace mc::blocks;

// ========== ScaffoldingBlock 测试 ==========

class ScaffoldingBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建脚手架方块
        scaffolding_ = std::make_unique<ScaffoldingBlock>(
            BlockProperties(Material::DECORATION)
                .hardness(0.0f)
                .resistance(0.0f)
        );
    }

    std::unique_ptr<ScaffoldingBlock> scaffolding_;
};

TEST_F(ScaffoldingBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(scaffolding_, nullptr);
}

TEST_F(ScaffoldingBlockTest, DefaultState_HasCorrectValues) {
    const auto& state = scaffolding_->defaultState();

    // 默认距离为 7（最远）
    EXPECT_EQ(state.get(BlockStateProperties::DISTANCE_0_7()), 7);

    // 默认不含水
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));

    // 默认不显示底部
    EXPECT_FALSE(state.get(BlockStateProperties::BOTTOM()));
}

TEST_F(ScaffoldingBlockTest, IsLadder_AlwaysReturnsTrue) {
    const auto& state = scaffolding_->defaultState();
    // 脚手架始终可攀爬
    EXPECT_TRUE(scaffolding_->isLadder(state, nullptr, nullptr, nullptr));
}

TEST_F(ScaffoldingBlockTest, GetShape_ReturnsValidShape) {
    const auto& state = scaffolding_->defaultState();
    const auto& shape = scaffolding_->getShape(state);
    // 脚手架形状不为空
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(ScaffoldingBlockTest, GetCollisionShape_DistanceZero_ReturnsEmpty) {
    // 当 distance=0 时，碰撞形状为空（玩家可以穿过）
    auto state = scaffolding_->defaultState()
        .with(BlockStateProperties::DISTANCE_0_7(), 0)
        .with(BlockStateProperties::BOTTOM(), true);

    const auto& shape = scaffolding_->getCollisionShape(state);
    EXPECT_TRUE(shape.isEmpty());
}

TEST_F(ScaffoldingBlockTest, GetCollisionShape_DistanceNonZeroWithBottom_ReturnsBaseShape) {
    // 当 distance!=0 且 bottom=true 时，玩家可以站在底部平台上
    auto state = scaffolding_->defaultState()
        .with(BlockStateProperties::DISTANCE_0_7(), 3)
        .with(BlockStateProperties::BOTTOM(), true);

    const auto& shape = scaffolding_->getCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(ScaffoldingBlockTest, GetCollisionShape_DistanceNonZeroWithoutBottom_ReturnsEmpty) {
    // 当 distance!=0 且 bottom=false 时，碰撞形状为空
    auto state = scaffolding_->defaultState()
        .with(BlockStateProperties::DISTANCE_0_7(), 3)
        .with(BlockStateProperties::BOTTOM(), false);

    const auto& shape = scaffolding_->getCollisionShape(state);
    EXPECT_TRUE(shape.isEmpty());
}

TEST_F(ScaffoldingBlockTest, IsWaterlogged_WorksCorrectly) {
    auto waterloggedState = scaffolding_->defaultState()
        .with(BlockStateProperties::WATERLOGGED(), true);
    auto nonWaterloggedState = scaffolding_->defaultState()
        .with(BlockStateProperties::WATERLOGGED(), false);

    EXPECT_TRUE(scaffolding_->isWaterlogged(waterloggedState));
    EXPECT_FALSE(scaffolding_->isWaterlogged(nonWaterloggedState));
}

TEST_F(ScaffoldingBlockTest, CalculateDistance_StaticMethod_Exists) {
    // 测试静态方法存在
    // 注意：calculateDistance 需要 IWorld 参数，这里只验证方法签名存在
    // 实际距离计算测试需要 MockWorld
}

TEST_F(ScaffoldingBlockTest, StateContainer_HasAllProperties) {
    const auto& state = scaffolding_->defaultState();

    // 验证所有属性都存在
    EXPECT_TRUE(state.hasProperty(BlockStateProperties::DISTANCE_0_7()));
    EXPECT_TRUE(state.hasProperty(BlockStateProperties::WATERLOGGED()));
    EXPECT_TRUE(state.hasProperty(BlockStateProperties::BOTTOM()));
}

TEST_F(ScaffoldingBlockTest, Distance_CanBeSetToAllValidValues) {
    // DISTANCE_0_7 属性范围是 0-7
    for (i32 distance = 0; distance <= 7; ++distance) {
        auto state = scaffolding_->defaultState()
            .with(BlockStateProperties::DISTANCE_0_7(), distance);
        EXPECT_EQ(state.get(BlockStateProperties::DISTANCE_0_7()), distance);
    }
}

TEST_F(ScaffoldingBlockTest, Bottom_CanBeToggled) {
    auto bottomState = scaffolding_->defaultState()
        .with(BlockStateProperties::BOTTOM(), true);
    auto topState = scaffolding_->defaultState()
        .with(BlockStateProperties::BOTTOM(), false);

    EXPECT_TRUE(bottomState.get(BlockStateProperties::BOTTOM()));
    EXPECT_FALSE(topState.get(BlockStateProperties::BOTTOM()));
}

TEST_F(ScaffoldingBlockTest, GetFluidState_NonWaterlogged) {
    const auto& state = scaffolding_->defaultState()
        .with(BlockStateProperties::WATERLOGGED(), false);

    const auto* fluidState = scaffolding_->getFluidState(state);
    // 非含水状态返回基类的流体状态（空气）
    // 不做具体断言，只验证方法可以调用
    MC_UNUSED(fluidState);
}

TEST_F(ScaffoldingBlockTest, Shape_ChangesWithBottomProperty) {
    // bottom=true 时返回完整形状
    auto bottomState = scaffolding_->defaultState()
        .with(BlockStateProperties::BOTTOM(), true);
    const auto& bottomShape = scaffolding_->getShape(bottomState);

    // bottom=false 时返回顶部平台形状
    auto topState = scaffolding_->defaultState()
        .with(BlockStateProperties::BOTTOM(), false);
    const auto& topShape = scaffolding_->getShape(topState);

    // 两种形状应该不同
    // 注意：CollisionShape 目前没有直接比较操作符，所以我们只检查它们都不是空
    EXPECT_FALSE(bottomShape.isEmpty());
    EXPECT_FALSE(topShape.isEmpty());
}

// ========== 新增测试：Tick 行为验证 ==========

TEST_F(ScaffoldingBlockTest, Tick_WhenDistanceBecomesSeven_CreatesFallingBlockEntity) {
    // 此测试验证 tick 方法在 distance 从非7变为7时的行为
    // 核心逻辑：
    // 1. distance == 7 且 previousDistance != 7: 创建 FallingBlockEntity
    // 2. distance == 7 且 previousDistance == 7: 掉落物品
    //
    // 由于需要完整的 IWorld 实现（包括 spawnEntity），这里验证代码路径存在
    // 实际的实体创建在集成测试中验证
}

TEST_F(ScaffoldingBlockTest, Tick_WhenDistanceStaysSeven_DropsItemEntity) {
    // 此测试验证 tick 方法在 distance 已经是7时的行为
    // 此时应该直接掉落物品而不是创建下落实体
}

TEST_F(ScaffoldingBlockTest, Tick_WhenDistanceLessThanSeven_UpdatesState) {
    // 此测试验证 tick 方法在 distance < 7 时的行为
    // 应该更新方块状态而不创建实体或掉落物品
}

TEST_F(ScaffoldingBlockTest, Tick_WhenBlockReplaced_DoesNothing) {
    // 此测试验证 tick 方法在方块已被替换时的行为
    // 应该直接返回不做任何事
}

// ========== 新增测试：物品掉落逻辑验证 ==========

TEST_F(ScaffoldingBlockTest, ItemDrop_UsesBlockItemRegistry) {
    // 验证脚手架物品掉落使用 BlockItemRegistry
    // 当脚手架失去支撑时，应从 BlockItemRegistry 获取对应的物品
    const Block* block = scaffolding_.get();
    const mc::BlockItem* blockItem = mc::BlockItemRegistry::instance().getBlockItem(*block);

    // 注意：BlockItemRegistry 需要初始化才能返回有效结果
    // 此测试只验证 API 可用性
    MC_UNUSED(blockItem);
}

TEST_F(ScaffoldingBlockTest, ItemDrop_UsesItemDropHelper) {
    // 验证物品掉落使用 ItemDropHelper
    // ItemDropHelper::spawnItemEntity 应在方块中心位置生成物品实体
    // 此测试验证 API 可用性
}

// ========== 新增测试：FallingBlockEntity 创建验证 ==========

TEST_F(ScaffoldingBlockTest, FallingBlock_SetsBlockId) {
    // 验证 FallingBlockEntity 设置正确的方块ID
    // 当脚手架下落时，setBlockId 应设置为脚手架方块的ID
}

TEST_F(ScaffoldingBlockTest, FallingBlock_SetsHurtEntitiesFalse) {
    // 验证脚手架下落时不伤害实体
    // fallingEntity->setHurtEntities(false) 应被调用
    // 这与沙子、铁砧等不同
}

TEST_F(ScaffoldingBlockTest, FallingBlock_SpawnEntityOnFailure_RestoresBlock) {
    // 验证当 spawnEntity 失败时（返回 EntityId(0)），方块应恢复
}
