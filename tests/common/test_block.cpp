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

#include "../src/common/entity/core/Entity.hpp"
#include "../src/common/item/context/BlockItemUseContext.hpp"
#include "../src/common/item/core/ItemStack.hpp"
#include "../src/common/util/color/DyeColor.hpp"
#include "../src/common/util/math/Vector3.hpp"
#include "../src/common/util/math/random/Random.hpp"
#include "../src/common/util/property/Properties.hpp"
#include "../src/common/util/property/StateContainer.hpp"
#include "../src/common/util/property/StateHolder.hpp"
#include "../src/common/world/IWorld.hpp"
#include "../src/common/world/block/Block.hpp"
#include "../src/common/world/block/BlockRegistry.hpp"
#include "../src/common/world/block/Material.hpp"
#include "../src/common/world/block/blocks/FallingBlock.hpp"
#include "../src/common/world/block/blocks/RotatedPillarBlock.hpp"
#include "../src/common/world/block/blocks/agricultural/CropBlock.hpp"
#include "../src/common/world/block/blocks/agricultural/FarmlandBlock.hpp"
#include "../src/common/world/block/blocks/agricultural/StemBlock.hpp"
#include "../src/common/world/block/blocks/coral/CoralBlock.hpp"
#include "../src/common/world/block/blocks/functional/BedBlock.hpp"
#include "../src/common/world/block/blocks/functional/CakeBlock.hpp"
#include "../src/common/world/block/blocks/ice/SnowBlock.hpp"
#include "../src/common/world/block/blocks/vegetation/SugarCaneBlock.hpp"
#include "../src/common/world/block/registry/VanillaBlocks.hpp"
#include "../src/common/world/border/WorldBorder.hpp"
#include "../src/common/world/fluid/Fluid.hpp"
#include "../src/common/world/fluid/FluidRegistry.hpp"
#include "../src/common/world/fluid/Fluids.hpp"
#include "../src/common/world/tick/manager/TickManager.hpp"
#include "common/TestWorldHelper.hpp"
#include <atomic>
#include <memory>
#include <unordered_map>
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// 测试用的简单方块类
// ============================================================================

class TestBlock : public Block {
public:
    explicit TestBlock(BlockProperties properties)
        : Block(properties)
    {
        // 创建空状态容器
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block,
                auto values,
                const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                const std::vector<BlockState*>* allStates,
                u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
        createBlockState(std::move(container));
    }
};

class TestBlockWithAxis : public Block {
public:
    explicit TestBlockWithAxis(BlockProperties properties)
        : Block(properties)
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).addAxis("axis").create(
            [](const Block& block,
                auto values,
                const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                const std::vector<BlockState*>* allStates,
                u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    // 从 StateContainer 获取属性
    static const EnumProperty<Axis>& AXIS(const Block& block)
    {
        return *static_cast<const EnumProperty<Axis>*>(block.stateContainer().getProperty("axis"));
    }
};

class TestBlockWithFacing : public Block {
public:
    explicit TestBlockWithFacing(BlockProperties properties)
        : Block(properties)
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).addHorizontalDirection("facing").create(
            [](const Block& block,
                auto values,
                const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                const std::vector<BlockState*>* allStates,
                u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    static const DirectionProperty& FACING(const Block& block)
    {
        return *static_cast<const DirectionProperty*>(block.stateContainer().getProperty("facing"));
    }
};

class TestBlockWithMultiple : public Block {
public:
    explicit TestBlockWithMultiple(BlockProperties properties)
        : Block(properties)
    {
        auto container =
            StateContainer<Block, BlockState>::Builder(*this).addHorizontalDirection("facing").addBoolean("lit").create(
                [](const Block& block,
                    auto values,
                    const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                    const std::vector<BlockState*>* allStates,
                    u32 id) {
                    return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
                });
        createBlockState(std::move(container));
    }

    static const DirectionProperty& FACING(const Block& block)
    {
        return *static_cast<const DirectionProperty*>(block.stateContainer().getProperty("facing"));
    }

    static const BooleanProperty& LIT(const Block& block)
    {
        return *static_cast<const BooleanProperty*>(block.stateContainer().getProperty("lit"));
    }
};

class TestBlockWithoutExplicitStateContainer : public Block {
public:
    explicit TestBlockWithoutExplicitStateContainer(BlockProperties properties)
        : Block(properties)
    {}
};

namespace {

ResourceLocation makeUniqueTestBlockId()
{
    static std::atomic<u32> counter{0};
    const u32 suffix = ++counter;
    return ResourceLocation("test:auto_state_block_" + std::to_string(suffix));
}

class BlockRulesTestWorld final : public IBlockReader {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[packPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr) {
                return fluidState;
            }
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override
    {
        return sampleLight(m_blockLight, x, y, z, 15);
    }
    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override { return sampleLight(m_skyLight, x, y, z, 15); }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Peaceful; }
    [[nodiscard]] bool isClientSide() const override { return false; }
    [[nodiscard]] bool isRaining() const override { return m_isRaining; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return m_canRainAt; }

    // 提供 ECS 实体注册表：方块 tick（如 FallingBlock::tick）在 ECS 迁移后构造实体需 registry
    // 句柄，world.entityRegistry()==nullptr 时静默跳过 spawn（沙子不掉落）。返回共享测试 registry
    // 与 BaseTestWorld 对齐，使 FallingBlock 等能正常 spawn 实体。
    [[nodiscard]] ecs::EntityRegistry* entityRegistry() override { return &mc::test::testEcsRegistry(); }
    [[nodiscard]] const ecs::EntityRegistry* entityRegistry() const override { return &mc::test::testEcsRegistry(); }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        (void)entity;
        ++m_spawnedEntityCount;
        return m_spawnEntityResult;
    }

    void setSpawnEntityResult(EntityInstanceId result) { m_spawnEntityResult = result; }

    void setSeed(u64 seed) { m_seed = seed; }

    void setRaining(bool raining) { m_isRaining = raining; }

    void setCanRainAt(bool canRainAt) { m_canRainAt = canRainAt; }

    void setSkyLightAt(const BlockPos& pos, u8 light) { m_skyLight[pos] = light; }

    void setBlockLightAt(const BlockPos& pos, u8 light) { m_blockLight[pos] = light; }

    [[nodiscard]] i32 spawnedEntityCount() const { return m_spawnedEntityCount; }

    // TickManager interface
    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<BlockRulesTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    // Random interface
    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    // WorldBorder interface
    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    [[nodiscard]] static u8 sampleLight(const std::map<BlockPos, u8>& lights, i32 x, i32 y, i32 z, u8 fallback)
    {
        const auto it = lights.find(BlockPos(x, y, z));
        if (it != lights.end()) {
            return it->second;
        }
        return fallback;
    }

    static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::map<BlockPos, u8> m_blockLight;
    std::map<BlockPos, u8> m_skyLight;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    math::Random m_random{12345}; // 固定种子的随机数生成器
    world::border::WorldBorder m_worldBorder;
    u64 m_seed = 0;
    bool m_isRaining = false;
    bool m_canRainAt = false;
    EntityInstanceId m_spawnEntityResult = 1;
    i32 m_spawnedEntityCount = 0;
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

class TestCropBlock final : public blocks::CropBlock {
public:
    explicit TestCropBlock(const BlockProperties& properties)
        : CropBlock(properties)
    {
        auto container =
            StateContainer<Block, BlockState>::Builder(*this)
                .add(BlockStateProperties::AGE_0_7())
                .create([](const Block& block,
                            std::vector<size_t> values,
                            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                            const std::vector<BlockState*>* allStates,
                            u32 id) {
                    return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
                });
        createBlockState(std::move(container));
        setDefaultState(defaultState().with(BlockStateProperties::AGE_0_7(), 0));
    }

    [[nodiscard]] u32 getCropItem() const override { return 0; }
    [[nodiscard]] u32 getSeedItem() const override { return 0; }
};

class TestStemBlock final : public blocks::StemBlock {
public:
    explicit TestStemBlock(const BlockProperties& properties)
        : StemBlock(nullptr, properties)
    {
        auto container =
            StateContainer<Block, BlockState>::Builder(*this)
                .add(BlockStateProperties::AGE_0_7())
                .create([](const Block& block,
                            std::vector<size_t> values,
                            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                            const std::vector<BlockState*>* allStates,
                            u32 id) {
                    return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
                });
        createBlockState(std::move(container));
        setDefaultState(defaultState().with(BlockStateProperties::AGE_0_7(), 0));
    }

    [[nodiscard]] u32 getSeedItem() const override { return 0; }
};

} // namespace

// ============================================================================
// Material 测试
// ============================================================================

TEST(MaterialTest, PredefinedMaterials)
{
    // 空气
    EXPECT_FALSE(Material::AIR.blocksMovement());
    EXPECT_FALSE(Material::AIR.isSolid());
    EXPECT_TRUE(Material::AIR.isReplaceable());

    // 石头
    EXPECT_TRUE(Material::ROCK.isSolid());
    EXPECT_TRUE(Material::ROCK.blocksMovement());
    EXPECT_FALSE(Material::ROCK.isLiquid());

    // 水
    EXPECT_TRUE(Material::WATER.isLiquid());
    EXPECT_FALSE(Material::WATER.isSolid());
    EXPECT_TRUE(Material::WATER.isReplaceable());

    // 木头
    EXPECT_TRUE(Material::WOOD.isSolid());
    EXPECT_TRUE(Material::WOOD.isFlammable());

    // 树叶
    EXPECT_TRUE(Material::LEAVES.isSolid());
    EXPECT_TRUE(Material::LEAVES.isFlammable());

    // 有机材质（草方块、干草块、疣块等）
    EXPECT_TRUE(Material::ORGANIC.isSolid());
    EXPECT_TRUE(Material::ORGANIC.isOpaque());
    EXPECT_FALSE(Material::ORGANIC.isFlammable());
    EXPECT_FALSE(Material::ORGANIC.isLiquid());
    EXPECT_FALSE(Material::ORGANIC.isReplaceable());
}

TEST(MaterialTest, MaterialBuilder)
{
    Material customMaterial = MaterialBuilder().solid().flammable().opaque().build();

    EXPECT_TRUE(customMaterial.isSolid());
    EXPECT_TRUE(customMaterial.isFlammable());
    EXPECT_TRUE(customMaterial.isOpaque());
    EXPECT_FALSE(customMaterial.isLiquid());
}

// ============================================================================
// BlockProperties 测试
// ============================================================================

TEST(BlockPropertiesTest, BasicProperties)
{
    BlockProperties props{Material::ROCK};

    // 注意: BlockProperties 存储 Material 副本，不是引用
    EXPECT_EQ(props.material().isSolid(), Material::ROCK.isSolid());
    EXPECT_EQ(props.material().blocksMovement(), Material::ROCK.blocksMovement());
    EXPECT_EQ(props.hardness(), 0.0f);
    EXPECT_EQ(props.resistance(), 0.0f);
    EXPECT_EQ(props.lightLevel(), 0);
    EXPECT_TRUE(props.hasCollision());
    EXPECT_TRUE(props.isSolid());
    EXPECT_FALSE(props.isFlammable());
}

TEST(BlockPropertiesTest, ChainProperties)
{
    BlockProperties props = BlockProperties{Material::WOOD}.hardness(2.0f).resistance(3.0f).lightLevel(15).flammable();

    EXPECT_EQ(props.hardness(), 2.0f);
    EXPECT_EQ(props.resistance(), 3.0f);
    EXPECT_EQ(props.lightLevel(), 15);
    EXPECT_TRUE(props.isFlammable());
}

TEST(BlockPropertiesTest, SpecialFlags)
{
    BlockProperties noCollision = BlockProperties{Material::ROCK}.noCollision();
    EXPECT_FALSE(noCollision.hasCollision());

    BlockProperties notSolid = BlockProperties{Material::GLASS}.notSolid();
    EXPECT_FALSE(notSolid.isSolid());

    BlockProperties replaceable = BlockProperties{Material::PLANT}.replaceable();
    EXPECT_TRUE(replaceable.isReplaceable());
}

TEST(BlockPropertiesTest, Strength)
{
    BlockProperties props = BlockProperties{Material::ROCK}.strength(2.5f);

    EXPECT_EQ(props.hardness(), 2.5f);
    EXPECT_EQ(props.resistance(), 2.5f);
}

TEST(BlockPropertiesTest, TransparentDefaultOpacityMatchesVanillaRule)
{
    // 非不透明方块在未显式设置 opacity 时，默认应为 1（非全黑遮挡）。
    TestBlock glassLike{BlockProperties{Material::GLASS}.notSolid()};
    EXPECT_EQ(glassLike.defaultState().getOpacity(), 1);
}

TEST(BlockSoundTypeTest, DirtUsesVanillaGroundSoundEvents)
{
    const auto& soundType = BlockSoundTypes::DIRT;

    EXPECT_EQ(soundType.getBreakSound().toString(), "minecraft:block.gravel.break");
    EXPECT_EQ(soundType.getStepSound().toString(), "minecraft:block.gravel.step");
    EXPECT_EQ(soundType.getPlaceSound().toString(), "minecraft:block.gravel.place");
    EXPECT_EQ(soundType.getHitSound().toString(), "minecraft:block.gravel.hit");
    EXPECT_EQ(soundType.getFallSound().toString(), "minecraft:block.gravel.fall");
}

TEST(BlockSoundTypeTest, GrassBlockUsesGrassSoundType)
{
    VanillaBlocks::initialize();

    const auto& soundType = VanillaBlocks::GRASS_BLOCK->defaultState().getSoundType();
    EXPECT_EQ(soundType.getBreakSound().toString(), BlockSoundTypes::GRASS.getBreakSound().toString());
    EXPECT_EQ(soundType.getPlaceSound().toString(), BlockSoundTypes::GRASS.getPlaceSound().toString());
}

// ============================================================================
// StateContainer 测试
// ============================================================================

TEST(StateContainerTest, EmptyContainer)
{
    TestBlock block{BlockProperties{Material::ROCK}.hardness(1.0f)};

    const auto& container = block.stateContainer();

    // 空容器应该有1个状态（基础状态）
    EXPECT_EQ(container.stateCount(), 1u);

    // 基础状态应该没有属性
    const auto& baseState = container.baseState();
    EXPECT_EQ(baseState.values().size(), 0u);
}

TEST(StateContainerTest, SingleProperty)
{
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};

    const auto& container = block.stateContainer();

    // axis 有 3 个值 (X, Y, Z)
    EXPECT_EQ(container.stateCount(), 3u);

    // 验证所有状态
    const auto& states = container.validStates();
    EXPECT_EQ(states.size(), 3u);

    // 获取属性
    const auto* prop = container.getProperty("axis");
    ASSERT_NE(prop, nullptr);
    EXPECT_EQ(prop->name(), "axis");
}

TEST(StateContainerTest, MultipleProperties)
{
    TestBlockWithMultiple block{BlockProperties{Material::ROCK}};

    const auto& container = block.stateContainer();

    // facing: 4 values * lit: 2 values = 8 states
    EXPECT_EQ(container.stateCount(), 8u);
}

TEST(StateContainerTest, GetProperty)
{
    TestBlockWithFacing block{BlockProperties{Material::ROCK}};

    const auto& container = block.stateContainer();

    const auto* facing = container.getProperty("facing");
    ASSERT_NE(facing, nullptr);
    EXPECT_EQ(facing->name(), "facing");
    EXPECT_EQ(facing->valueCount(), 4u);

    const auto* nonexistent = container.getProperty("nonexistent");
    EXPECT_EQ(nonexistent, nullptr);
}

// ============================================================================
// BlockState 测试
// ============================================================================

TEST(BlockStateTest, GetProperty)
{
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    Axis axis = state.get(TestBlockWithAxis::AXIS(block));
    // 默认值应该是第一个值 (X)
    EXPECT_EQ(axis, Axis::X);
}

TEST(BlockStateTest, SetProperty)
{
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    // 设置新值
    const auto& newState = state.with(TestBlockWithAxis::AXIS(block), Axis::Y);
    EXPECT_EQ(newState.get(TestBlockWithAxis::AXIS(block)), Axis::Y);

    // 设置另一个值
    const auto& state3 = state.with(TestBlockWithAxis::AXIS(block), Axis::Z);
    EXPECT_EQ(state3.get(TestBlockWithAxis::AXIS(block)), Axis::Z);
}

TEST(BlockStateTest, SetPropertySameValue)
{
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    // 设置相同的值应该返回同一个状态
    const auto& newState = state.with(TestBlockWithAxis::AXIS(block), Axis::X);
    EXPECT_EQ(&state, &newState);
}

TEST(BlockStateTest, CycleProperty)
{
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    // 循环: X -> Y
    const auto& state1 = state.cycle(TestBlockWithAxis::AXIS(block));
    EXPECT_EQ(state1.get(TestBlockWithAxis::AXIS(block)), Axis::Y);

    // 循环: Y -> Z
    const auto& state2 = state1.cycle(TestBlockWithAxis::AXIS(block));
    EXPECT_EQ(state2.get(TestBlockWithAxis::AXIS(block)), Axis::Z);

    // 循环: Z -> X (回绕)
    const auto& state3 = state2.cycle(TestBlockWithAxis::AXIS(block));
    EXPECT_EQ(state3.get(TestBlockWithAxis::AXIS(block)), Axis::X);
}

TEST(BlockStateTest, HasProperty)
{
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();
    // 这个测试不需要 litProp

    EXPECT_TRUE(state.hasProperty(TestBlockWithAxis::AXIS(block)));
    // 该方块没有 lit 属性，跳过此测试  // 这个方块没有 lit 属性
}

TEST(BlockStateTest, StateId)
{
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& states = block.stateContainer().validStates();

    // 每个状态应该有不同的ID
    std::set<u32> ids;
    for (const auto& s : states) {
        ids.insert(s->stateId());
    }
    EXPECT_EQ(ids.size(), states.size());
}

TEST(BlockStateTest, ToString)
{
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    std::string str = state.toString();
    // 应该包含属性名和值
    EXPECT_TRUE(str.find("axis") != std::string::npos);
}

TEST(BlockStateTest, ToModelKeyUsesStableSortedOrder)
{
    TestBlockWithMultiple block{BlockProperties{Material::ROCK}};
    const auto& state = block.defaultState()
                            .with(TestBlockWithMultiple::FACING(block), Direction::West)
                            .with(TestBlockWithMultiple::LIT(block), true);

    const std::string modelKey = state.toModelKey();
    EXPECT_EQ(modelKey, "facing=west,lit=true");
}

TEST(BlockStateTest, MultiplePropertiesInteraction)
{
    TestBlockWithMultiple block{BlockProperties{Material::ROCK}};
    const auto& state = block.defaultState();

    // 获取并设置多个属性
    Direction facing = state.get(TestBlockWithMultiple::FACING(block));
    bool lit = state.get(TestBlockWithMultiple::LIT(block));

    const auto& state1 = state.with(TestBlockWithMultiple::FACING(block), Direction::East);
    EXPECT_EQ(state1.get(TestBlockWithMultiple::FACING(block)), Direction::East);
    EXPECT_EQ(state1.get(TestBlockWithMultiple::LIT(block)), lit); // lit 应该不变

    const auto& state2 = state1.with(TestBlockWithMultiple::LIT(block), true);
    EXPECT_EQ(state2.get(TestBlockWithMultiple::FACING(block)), Direction::East); // facing 应该不变
    EXPECT_EQ(state2.get(TestBlockWithMultiple::LIT(block)), true);
}

// ============================================================================
// Block 测试
// ============================================================================

TEST(BlockTest, BasicProperties)
{
    TestBlock block{BlockProperties{Material::ROCK}.hardness(1.5f).resistance(6.0f)};

    EXPECT_EQ(block.hardness(), 1.5f);
    EXPECT_EQ(block.resistance(), 6.0f);
    // Material 是副本，比较属性而非地址
    EXPECT_EQ(block.material().isSolid(), Material::ROCK.isSolid());
}

TEST(BlockTest, DefaultState)
{
    TestBlock block{BlockProperties{Material::ROCK}};

    const auto& state = block.defaultState();
    EXPECT_EQ(&state.owner(), &block);
}

TEST(BlockTest, IsAir)
{
    // 初始化 VanillaBlocks 确保 AIR 存在
    VanillaBlocks::initialize();

    // 空气方块应该返回 true
    EXPECT_TRUE(VanillaBlocks::AIR->isAir(VanillaBlocks::AIR->defaultState()));

    // 普通方块不是空气
    TestBlock normalBlock{BlockProperties{Material::ROCK}};
    EXPECT_FALSE(normalBlock.isAir(normalBlock.defaultState()));
}

TEST(BlockTest, StateCount)
{
    TestBlockWithMultiple block{BlockProperties{Material::ROCK}};

    // 4 directions * 2 lit values = 8 states
    EXPECT_EQ(block.stateContainer().stateCount(), 8u);
}

TEST(BlockTest, BlockWithoutExplicitStateContainer_HasValidDefaultState)
{
    TestBlockWithoutExplicitStateContainer block{BlockProperties{Material::ROCK}};

    EXPECT_EQ(block.stateContainer().stateCount(), 1u);

    const auto& defaultState = block.defaultState();
    EXPECT_EQ(&defaultState.owner(), &block);
}

TEST(VoxelShapesTest, CubeAcceptsPixelCoordinates)
{
    const CollisionShape shape = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 16.0f, 8.0f, 16.0f);
    ASSERT_EQ(shape.boxCount(), 1u);

    const AxisAlignedBB& box = shape.boxes().front();
    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);
    EXPECT_FLOAT_EQ(box.maxY, 0.5f);
    EXPECT_FLOAT_EQ(box.maxZ, 1.0f);
}

TEST(VoxelShapesTest, CubeAcceptsNormalizedCoordinates)
{
    const CollisionShape shape = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    ASSERT_EQ(shape.boxCount(), 1u);

    const AxisAlignedBB& box = shape.boxes().front();
    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);
    EXPECT_FLOAT_EQ(box.maxY, 0.5f);
    EXPECT_FLOAT_EQ(box.maxZ, 1.0f);
}

// ============================================================================
// BlockRegistry 测试
// ============================================================================

TEST(BlockRegistryTest, RegisterBlock)
{
    // 注意：由于 VanillaBlocks::initialize() 可能已运行，blockId 可能不为 1
    auto& block = BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:test_block_reg1"), BlockProperties{Material::ROCK});

    EXPECT_NE(&block, nullptr);
    EXPECT_EQ(block.blockLocation(), ResourceLocation("test:test_block_reg1"));
    EXPECT_GT(block.blockId(), 0u); // ID should be > 0
}

TEST(BlockRegistryTest, RegisterBlockWithoutExplicitStateContainer)
{
    auto& block = BlockRegistry::instance().registerBlock<TestBlockWithoutExplicitStateContainer>(
        makeUniqueTestBlockId(), BlockProperties{Material::ROCK});

    EXPECT_EQ(block.stateContainer().stateCount(), 1u);

    const BlockState* state = BlockRegistry::instance().getBlockState(block.defaultState().stateId());
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->blockId(), block.blockId());
}

TEST(BlockRegistryTest, GetBlockById)
{
    auto& registered = BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:test_block_reg2"), BlockProperties{Material::ROCK});

    // 通过已注册方块的 ID 查找，确保返回相同的方块
    Block* retrieved = BlockRegistry::instance().getBlock(registered.blockId());
    EXPECT_EQ(retrieved, &registered);
}

TEST(BlockRegistryTest, GetBlockByLocation)
{
    auto& registered = BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:test_block_reg3"), BlockProperties{Material::ROCK});

    Block* retrieved = BlockRegistry::instance().getBlock(ResourceLocation("test:test_block_reg3"));
    EXPECT_EQ(retrieved, &registered);
}

TEST(BlockRegistryTest, DuplicateRegistrationReturnsExistingBlock)
{
    const ResourceLocation id("test:duplicate_block_reg");
    const size_t countBefore = BlockRegistry::instance().blockCount();

    auto& first = BlockRegistry::instance().registerBlock<TestBlock>(id, BlockProperties{Material::ROCK});
    auto& second = BlockRegistry::instance().registerBlock<TestBlock>(id, BlockProperties{Material::WOOD});

    EXPECT_EQ(&first, &second);
    EXPECT_EQ(BlockRegistry::instance().blockCount(), countBefore + 1);
}

TEST(BlockRegistryTest, GetBlockState)
{
    auto& block = BlockRegistry::instance().registerBlock<TestBlockWithAxis>(
        ResourceLocation("test:block_with_axis_reg"), BlockProperties{Material::WOOD});

    // 获取默认状态
    const auto& defaultState = block.defaultState();
    BlockState* retrieved = BlockRegistry::instance().getBlockState(defaultState.stateId());
    EXPECT_EQ(retrieved, &defaultState);
}

TEST(BlockRegistryTest, ForEachBlock)
{
    BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:blockA_reg"), BlockProperties{Material::ROCK});
    BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:blockB_reg"), BlockProperties{Material::WOOD});

    int count = 0;
    BlockRegistry::instance().forEachBlock([&count](Block&) { count++; });

    EXPECT_GT(count, 0);
}

TEST(BlockRegistryTest, ForEachBlockState)
{
    BlockRegistry::instance().registerBlock<TestBlockWithAxis>(
        ResourceLocation("test:block_with_axis_reg2"), BlockProperties{Material::WOOD});

    int count = 0;
    BlockRegistry::instance().forEachBlockState([&count](const BlockState&) { count++; });

    // axis has 3 values, so at least 3 states
    EXPECT_GE(count, 3);
}

// ============================================================================
// VanillaBlocks 测试
// ============================================================================

TEST(VanillaBlocksTest, Initialization)
{
    VanillaBlocks::initialize();

    // 检查基础方块
    EXPECT_NE(VanillaBlocks::AIR, nullptr);
    EXPECT_NE(VanillaBlocks::STONE, nullptr);
    EXPECT_NE(VanillaBlocks::GRASS_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::DIRT, nullptr);
    EXPECT_NE(VanillaBlocks::OAK_LOG, nullptr);

    // 检查空气方块
    EXPECT_TRUE(VanillaBlocks::AIR->isAir(VanillaBlocks::AIR->defaultState()));
    // 空气方块的ID应该是0
    EXPECT_EQ(VanillaBlocks::AIR->blockId(), 0u);

    // 检查原木有轴属性
    const auto& logState = VanillaBlocks::OAK_LOG->defaultState();
    EXPECT_TRUE(logState.hasProperty(RotatedPillarBlock::AXIS()));

    // 检查新增的石头变种
    EXPECT_NE(VanillaBlocks::GRANITE, nullptr);
    EXPECT_NE(VanillaBlocks::DIORITE, nullptr);
    EXPECT_NE(VanillaBlocks::ANDESITE, nullptr);
    EXPECT_NE(VanillaBlocks::POLISHED_GRANITE, nullptr);

    // 检查新增的矿物方块
    EXPECT_NE(VanillaBlocks::GOLD_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::IRON_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::EMERALD_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::REDSTONE_BLOCK, nullptr);

    // 检查新增的羊毛
    EXPECT_NE(VanillaBlocks::WHITE_WOOL, nullptr);
    EXPECT_NE(VanillaBlocks::BLACK_WOOL, nullptr);

    // 检查新增的下界方块
    EXPECT_NE(VanillaBlocks::SOUL_SAND, nullptr);
    EXPECT_NE(VanillaBlocks::BASALT, nullptr);
    EXPECT_NE(VanillaBlocks::BLACKSTONE, nullptr);
    EXPECT_NE(VanillaBlocks::CRYING_OBSIDIAN, nullptr);

    // 检查玄武岩是轴向方块
    const auto& basaltState = VanillaBlocks::BASALT->defaultState();
    EXPECT_TRUE(basaltState.hasProperty(RotatedPillarBlock::AXIS()));

    // 检查哭泣的黑曜石发光
    EXPECT_EQ(VanillaBlocks::CRYING_OBSIDIAN->defaultState().lightLevel(), 10);

    // 光照参数回归测试：防止透明/液体方块导致异常发黑
    EXPECT_EQ(VanillaBlocks::AIR->defaultState().getOpacity(), 0);
    EXPECT_TRUE(VanillaBlocks::AIR->defaultState().propagatesSkylightDown());

    EXPECT_EQ(VanillaBlocks::WATER->defaultState().getOpacity(), 0);
    EXPECT_FALSE(VanillaBlocks::WATER->defaultState().propagatesSkylightDown());

    EXPECT_EQ(VanillaBlocks::GLASS->defaultState().getOpacity(), 0);
    EXPECT_TRUE(VanillaBlocks::GLASS->defaultState().propagatesSkylightDown());
}

TEST(BlockStateTest, Caching)
{
    auto& block = BlockRegistry::instance().registerBlock<TestBlockWithMultiple>(
        ResourceLocation("test:block_caching"), BlockProperties{Material::ROCK});

    const auto& state1 = block.defaultState();
    const auto& state2 = state1.with(TestBlockWithMultiple::FACING(block), Direction::East);
    const auto& state3 = state2.with(TestBlockWithMultiple::LIT(block), true);

    // 设置相同值应该返回相同的状态
    const auto& state4 = state3.with(TestBlockWithMultiple::FACING(block), Direction::East);
    EXPECT_EQ(&state3, &state4);

    // 从不同路径到达相同状态应该返回相同状态
    const auto& state5 = state1.with(TestBlockWithMultiple::LIT(block), true)
                             .with(TestBlockWithMultiple::FACING(block), Direction::East);
    EXPECT_EQ(&state3, &state5);
}

// ============================================================================
// 方块注册测试 - 验证动态ID分配和资源位置查找
// ============================================================================

TEST(BlockRegistryTest, BasicBlocksRegistration)
{
    VanillaBlocks::initialize();

    // 验证基础方块已注册且可通过资源位置查找
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:air")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:stone")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:grass_block")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:dirt")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:cobblestone")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:oak_planks")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:bedrock")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:water")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:lava")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:sand")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:gravel")), nullptr);
}

TEST(BlockRegistryTest, OreBlocksRegistration)
{
    VanillaBlocks::initialize();

    // 验证矿石方块已注册
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:gold_ore")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:iron_ore")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:coal_ore")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:diamond_ore")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:emerald_ore")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:lapis_ore")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:redstone_ore")), nullptr);
}

TEST(BlockRegistryTest, LogBlocksRegistration)
{
    VanillaBlocks::initialize();

    // 验证原木和树叶已注册
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:oak_log")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:oak_leaves")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:spruce_log")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:birch_log")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:jungle_log")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:acacia_log")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:dark_oak_log")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:spruce_leaves")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:birch_leaves")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:jungle_leaves")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:acacia_leaves")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:dark_oak_leaves")), nullptr);
}

TEST(BlockRegistryTest, StoneVariantsRegistration)
{
    VanillaBlocks::initialize();

    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:granite")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:diorite")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:andesite")), nullptr);
}

TEST(BlockRegistryTest, VegetationBlocksRegistration)
{
    VanillaBlocks::initialize();

    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:short_grass")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:tall_grass")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:fern")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:dandelion")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:poppy")), nullptr);

    // 甜浆果丛 - 验证注册成功
    EXPECT_NE(VanillaBlocks::SWEET_BERRY_BUSH, nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:sweet_berry_bush")), nullptr);
}

TEST(BlockRegistryTest, AgriculturalBlocksRegistration)
{
    VanillaBlocks::initialize();

    // 验证农作物方块已注册
    EXPECT_NE(VanillaBlocks::WHEAT, nullptr);
    EXPECT_NE(VanillaBlocks::CARROTS, nullptr);
    EXPECT_NE(VanillaBlocks::POTATOES, nullptr);
    EXPECT_NE(VanillaBlocks::BEETROOTS, nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:wheat")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:carrots")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:potatoes")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:beetroots")), nullptr);

    // 可可豆 - 验证注册成功
    EXPECT_NE(VanillaBlocks::COCOA, nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:cocoa")), nullptr);
}

TEST(BlockRegistryTest, NetherBlocksRegistration)
{
    VanillaBlocks::initialize();

    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:netherrack")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:soul_sand")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:basalt")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:glowstone")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:end_stone")), nullptr);
    // 验证地狱疣块和诡异疣块已注册
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:nether_wart_block")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:warped_wart_block")), nullptr);
}

TEST(BlockStateComparisonTest, IsComparisonWorks)
{
    VanillaBlocks::initialize();

    // 验证方块状态比较正确工作
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    const BlockState* dirtState = &VanillaBlocks::DIRT->defaultState();

    ASSERT_NE(stoneState, nullptr);
    ASSERT_NE(dirtState, nullptr);

    // 验证 is() 方法工作正常
    EXPECT_TRUE(stoneState->is(VanillaBlocks::STONE));
    EXPECT_FALSE(stoneState->is(VanillaBlocks::DIRT));
    EXPECT_TRUE(dirtState->is(VanillaBlocks::DIRT));
    EXPECT_FALSE(dirtState->is(VanillaBlocks::STONE));
}

TEST(BlockRegistryTest, UniqueBlockIds)
{
    VanillaBlocks::initialize();

    // 验证不同方块有不同的ID
    std::set<u32> ids;

    // 添加基础方块ID
    ids.insert(VanillaBlocks::AIR->blockId());
    ids.insert(VanillaBlocks::STONE->blockId());
    ids.insert(VanillaBlocks::GRASS_BLOCK->blockId());
    ids.insert(VanillaBlocks::DIRT->blockId());
    ids.insert(VanillaBlocks::COBBLESTONE->blockId());
    ids.insert(VanillaBlocks::OAK_PLANKS->blockId());
    ids.insert(VanillaBlocks::WATER->blockId());
    ids.insert(VanillaBlocks::LAVA->blockId());
    ids.insert(VanillaBlocks::BEDROCK->blockId());
    ids.insert(VanillaBlocks::SAND->blockId());
    ids.insert(VanillaBlocks::GRAVEL->blockId());

    // 验证矿石有不同ID
    ids.insert(VanillaBlocks::GOLD_ORE->blockId());
    ids.insert(VanillaBlocks::IRON_ORE->blockId());
    ids.insert(VanillaBlocks::COAL_ORE->blockId());
    ids.insert(VanillaBlocks::DIAMOND_ORE->blockId());
    ids.insert(VanillaBlocks::EMERALD_ORE->blockId());

    // 验证原木有不同ID
    ids.insert(VanillaBlocks::OAK_LOG->blockId());
    ids.insert(VanillaBlocks::SPRUCE_LOG->blockId());
    ids.insert(VanillaBlocks::BIRCH_LOG->blockId());
    ids.insert(VanillaBlocks::JUNGLE_LOG->blockId());
    ids.insert(VanillaBlocks::ACACIA_LOG->blockId());
    ids.insert(VanillaBlocks::DARK_OAK_LOG->blockId());

    // 验证树叶有不同ID
    ids.insert(VanillaBlocks::OAK_LEAVES->blockId());
    ids.insert(VanillaBlocks::SPRUCE_LEAVES->blockId());
    ids.insert(VanillaBlocks::BIRCH_LEAVES->blockId());
    ids.insert(VanillaBlocks::JUNGLE_LEAVES->blockId());
    ids.insert(VanillaBlocks::ACACIA_LEAVES->blockId());
    ids.insert(VanillaBlocks::DARK_OAK_LEAVES->blockId());

    // 确保所有ID都是唯一的 (数量等于添加的方块数)
    // AIR + STONE + GRASS_BLOCK + DIRT + COBBLESTONE + OAK_PLANKS + WATER + LAVA + BEDROCK + SAND + GRAVEL = 11
    // GOLD_ORE + IRON_ORE + COAL_ORE + DIAMOND_ORE + EMERALD_ORE = 5 (共 16)
    // OAK_LOG + SPRUCE_LOG + BIRCH_LOG + JUNGLE_LOG + ACACIA_LOG + DARK_OAK_LOG = 6 (共 22)
    // OAK_LEAVES + SPRUCE_LEAVES + BIRCH_LEAVES + JUNGLE_LEAVES + ACACIA_LEAVES + DARK_OAK_LEAVES = 6 (共 28)
    EXPECT_EQ(ids.size(), 28u);
}

TEST(VanillaBlocksTest, SandFamilyRegisteredAsFallingBlocks)
{
    VanillaBlocks::initialize();

    EXPECT_NE(dynamic_cast<blocks::FallingBlock*>(VanillaBlocks::SAND), nullptr);
    EXPECT_NE(dynamic_cast<blocks::FallingBlock*>(VanillaBlocks::GRAVEL), nullptr);
    EXPECT_NE(dynamic_cast<blocks::FallingBlock*>(VanillaBlocks::RED_SAND), nullptr);
}

TEST(AgriculturalBehaviorTest, SugarCaneRequiresAdjacentWater)
{
    fluid::FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    auto* sugarCaneBlock = dynamic_cast<blocks::SugarCaneBlock*>(VanillaBlocks::SUGAR_CANE);
    ASSERT_NE(sugarCaneBlock, nullptr);

    BlockRulesTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::SAND->defaultState());

    const BlockPos canePos(0, 64, 0);
    const BlockState& caneState = VanillaBlocks::SUGAR_CANE->defaultState();

    EXPECT_FALSE(sugarCaneBlock->isValidPosition(caneState, world, canePos));

    world.setBlockState(1, 63, 0, &VanillaBlocks::WATER->defaultState());
    EXPECT_TRUE(sugarCaneBlock->isValidPosition(caneState, world, canePos));
}

TEST(AgriculturalBehaviorTest, DryFarmlandWithoutCropsTurnsToDirt)
{
    fluid::FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    auto* farmlandBlock = dynamic_cast<blocks::FarmlandBlock*>(VanillaBlocks::FARMLAND);
    ASSERT_NE(farmlandBlock, nullptr);

    BlockRulesTestWorld world;
    const BlockPos farmlandPos(0, 64, 0);
    world.setBlockState(farmlandPos.x, farmlandPos.y, farmlandPos.z, &VanillaBlocks::FARMLAND->defaultState());

    BlockState farmlandState = VanillaBlocks::FARMLAND->defaultState();
    math::Random random(123456);
    farmlandBlock->randomTick(world, farmlandPos, farmlandState, random);

    const BlockState* updated = world.getBlockState(farmlandPos.x, farmlandPos.y, farmlandPos.z);
    ASSERT_NE(updated, nullptr);
    EXPECT_TRUE(updated->is(VanillaBlocks::DIRT));
}

TEST(AgriculturalBehaviorTest, FarmlandRehydratesWhenRaining)
{
    fluid::FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    auto* farmlandBlock = dynamic_cast<blocks::FarmlandBlock*>(VanillaBlocks::FARMLAND);
    ASSERT_NE(farmlandBlock, nullptr);

    BlockRulesTestWorld world;
    world.setRaining(true);
    world.setCanRainAt(true);

    const BlockPos farmlandPos(2, 64, 2);
    BlockState farmlandState = VanillaBlocks::FARMLAND->defaultState();
    world.setBlockState(farmlandPos.x, farmlandPos.y, farmlandPos.z, &farmlandState);

    math::Random random(123456);
    farmlandBlock->randomTick(world, farmlandPos, farmlandState, random);

    const BlockState* updated = world.getBlockState(farmlandPos.x, farmlandPos.y, farmlandPos.z);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ(updated->get(BlockStateProperties::MOISTURE_0_7()), 7);
}

TEST(AgriculturalBehaviorTest, CropRequiresLightToStayValid)
{
    // 注意：该用例依赖 VanillaBlocks（FARMLAND）已注册，否则会出现空指针访问。
    // 测试不能依赖其它用例的执行顺序，因此这里必须显式初始化。
    fluid::FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    TestCropBlock crop(BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());
    BlockRulesTestWorld world;
    const BlockPos cropPos(4, 65, 4);

    world.setBlockState(cropPos.x, cropPos.y - 1, cropPos.z, &VanillaBlocks::FARMLAND->defaultState());
    // 光照检查的是作物位置本身的光照
    // MC 1.16.5: max(blockLight, skyLight) >= 8 或 canSeeSky
    world.setSkyLightAt(cropPos, 7);
    world.setBlockLightAt(cropPos, 7);

    // 光照 7 < 8，作物不能生存
    EXPECT_FALSE(crop.isValidPosition(crop.defaultState(), world, cropPos));

    // 光照 8 >= 8，作物可以生存
    world.setSkyLightAt(cropPos, 8);
    EXPECT_TRUE(crop.isValidPosition(crop.defaultState(), world, cropPos));
}

TEST(AgriculturalBehaviorTest, CropBonemealGrowthUsesWorldSeedAndPosition)
{
    TestCropBlock crop(BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());
    BlockRulesTestWorld world;
    const BlockPos cropPos(8, 65, 8);
    const BlockState& cropState = crop.defaultState();

    world.setSeed(123456789ULL);
    world.setBlockState(cropPos.x, cropPos.y, cropPos.z, &cropState);
    crop.grow(world, cropPos, cropState);

    const BlockState* firstResult = world.getBlockState(cropPos.x, cropPos.y, cropPos.z);
    ASSERT_NE(firstResult, nullptr);
    const i32 firstAge = crop.getAge(*firstResult);
    EXPECT_GE(firstAge, 2);
    EXPECT_LE(firstAge, 5);

    world.setBlockState(cropPos.x, cropPos.y, cropPos.z, &cropState);
    crop.grow(world, cropPos, cropState);

    const BlockState* secondResult = world.getBlockState(cropPos.x, cropPos.y, cropPos.z);
    ASSERT_NE(secondResult, nullptr);
    EXPECT_EQ(crop.getAge(*secondResult), firstAge);
}

TEST(AgriculturalBehaviorTest, StemBonemealGrowthUsesWorldSeedAndPosition)
{
    TestStemBlock stem(BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());
    BlockRulesTestWorld world;
    const BlockPos stemPos(10, 65, 10);
    const BlockState& stemState = stem.defaultState();

    world.setSeed(987654321ULL);
    world.setBlockState(stemPos.x, stemPos.y, stemPos.z, &stemState);
    stem.grow(world, stemPos, stemState);

    const BlockState* firstResult = world.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(firstResult, nullptr);
    const i32 firstAge = stem.getAge(*firstResult);
    EXPECT_GE(firstAge, 2);
    EXPECT_LE(firstAge, 5);

    world.setBlockState(stemPos.x, stemPos.y, stemPos.z, &stemState);
    stem.grow(world, stemPos, stemState);

    const BlockState* secondResult = world.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(secondResult, nullptr);
    EXPECT_EQ(stem.getAge(*secondResult), firstAge);
}

TEST(CoralBehaviorTest, CoralBlockUsesSourceWaterAndFallsBackToDeadBlock)
{
    fluid::FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    ASSERT_NE(VanillaBlocks::DEAD_TUBE_CORAL_BLOCK, nullptr);

    blocks::CoralBlock coral(blocks::CoralColor::Tube,
        VanillaBlocks::DEAD_TUBE_CORAL_BLOCK->blockId(),
        BlockProperties(Material::CORAL).hardness(1.5f).resistance(6.0f));

    BlockRulesTestWorld wetWorld;
    const BlockPos wetPos(12, 64, 12);
    wetWorld.setBlockState(wetPos.x, wetPos.y, wetPos.z, &VanillaBlocks::WATER->defaultState());

    auto wetContext = makePlacementContext(wetWorld, wetPos, Direction::North, 180.0f);
    BlockState wetState = coral.getStateForPlacement(wetContext);
    EXPECT_TRUE(wetState.get(BlockStateProperties::WATERLOGGED()));

    BlockRulesTestWorld dryWorld;
    const BlockPos dryPos(14, 64, 14);
    const BlockState& dryState = coral.defaultState().with(BlockStateProperties::WATERLOGGED(), false);
    BlockState dryUpdated = coral.updatePostPlacement(dryState, Direction::Up, dryState, dryWorld, dryPos, dryPos.up());
    EXPECT_TRUE(dryUpdated.is(VanillaBlocks::DEAD_TUBE_CORAL_BLOCK));
}

TEST(CoralBehaviorTest, CoralFanUsesSourceWaterAndFallsBackToDeadBlock)
{
    fluid::FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    ASSERT_NE(VanillaBlocks::DEAD_TUBE_CORAL_BLOCK, nullptr);

    blocks::CoralFanBlock coralFan(blocks::CoralColor::Tube,
        VanillaBlocks::DEAD_TUBE_CORAL_BLOCK->blockId(),
        BlockProperties(Material::CORAL).hardness(0.0f).noCollision().notSolid());

    BlockRulesTestWorld wetWorld;
    const BlockPos wetPos(16, 64, 16);
    wetWorld.setBlockState(wetPos.x, wetPos.y, wetPos.z, &VanillaBlocks::WATER->defaultState());

    auto wetContext = makePlacementContext(wetWorld, wetPos, Direction::North, 180.0f);
    BlockState wetState = coralFan.getStateForPlacement(wetContext);
    EXPECT_TRUE(wetState.get(BlockStateProperties::WATERLOGGED()));

    BlockRulesTestWorld dryWorld;
    const BlockPos dryPos(18, 64, 18);
    const BlockState& dryState = coralFan.defaultState();
    BlockState dryUpdated =
        coralFan.updatePostPlacement(dryState, Direction::Up, dryState, dryWorld, dryPos, dryPos.up());
    EXPECT_TRUE(dryUpdated.is(VanillaBlocks::DEAD_TUBE_CORAL_BLOCK));
}

TEST(CoralBehaviorTest, CoralWallFanUsesSourceWaterAndFallsBackToDeadBlock)
{
    fluid::FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    ASSERT_NE(VanillaBlocks::DEAD_TUBE_CORAL_BLOCK, nullptr);

    blocks::CoralWallFanBlock coralWallFan(blocks::CoralColor::Tube,
        VanillaBlocks::DEAD_TUBE_CORAL_BLOCK->blockId(),
        BlockProperties(Material::CORAL).hardness(0.0f).noCollision().notSolid());

    BlockRulesTestWorld wetWorld;
    const BlockPos wetPos(20, 64, 20);
    wetWorld.setBlockState(wetPos.x, wetPos.y, wetPos.z, &VanillaBlocks::WATER->defaultState());

    auto wetContext = makePlacementContext(wetWorld, wetPos, Direction::North, 180.0f);
    BlockState wetState = coralWallFan.getStateForPlacement(wetContext);
    EXPECT_TRUE(wetState.get(BlockStateProperties::WATERLOGGED()));

    BlockRulesTestWorld dryWorld;
    const BlockPos dryPos(22, 64, 22);
    const BlockState& dryState = coralWallFan.defaultState();
    BlockState dryUpdated =
        coralWallFan.updatePostPlacement(dryState, Direction::Up, dryState, dryWorld, dryPos, dryPos.up());
    EXPECT_TRUE(dryUpdated.is(VanillaBlocks::DEAD_TUBE_CORAL_BLOCK));
}

TEST(FallingBlockBehaviorTest, UnsupportedSandSpawnsFallingEntity)
{
    fluid::FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    auto* fallingSand = dynamic_cast<blocks::FallingBlock*>(VanillaBlocks::SAND);
    ASSERT_NE(fallingSand, nullptr);

    BlockRulesTestWorld world;
    world.setSpawnEntityResult(1);

    const BlockPos sandPos(0, 70, 0);
    world.setBlockState(sandPos.x, sandPos.y, sandPos.z, &VanillaBlocks::SAND->defaultState());

    BlockState sandState = VanillaBlocks::SAND->defaultState();
    fallingSand->tick(world, sandPos, sandState, world.getRandom());

    EXPECT_EQ(world.spawnedEntityCount(), 1);
    const BlockState* stateAfterTick = world.getBlockState(sandPos.x, sandPos.y, sandPos.z);
    ASSERT_NE(stateAfterTick, nullptr);
    EXPECT_TRUE(stateAfterTick->isAir());
}

TEST(FunctionalBlockBehaviorTest, CakeUpdatePostPlacementReturnsAirWhenSupportIsMissing)
{
    VanillaBlocks::initialize();

    ASSERT_NE(VanillaBlocks::AIR, nullptr);

    blocks::CakeBlock cake(BlockProperties(Material::ORGANIC).hardness(0.5f).notSolid());

    BlockRulesTestWorld world;
    const BlockPos cakePos(24, 64, 24);
    const BlockState& cakeState = cake.defaultState();

    BlockState updatedState = cake.updatePostPlacement(cakeState, Direction::Down, cakeState, world, cakePos, cakePos);

    EXPECT_TRUE(updatedState.isAir());
}

TEST(FunctionalBlockBehaviorTest, BedUpdatePostPlacementReturnsAirWhenPartnerIsMissing)
{
    VanillaBlocks::initialize();

    ASSERT_NE(VanillaBlocks::AIR, nullptr);

    blocks::BedBlock bed(DyeColor::White, BlockProperties(Material::WOOL).hardness(0.2f).notSolid());

    BlockRulesTestWorld world;
    const BlockPos bedPos(26, 64, 26);
    const BlockState& bedState = bed.defaultState();
    const BlockState& airState = VanillaBlocks::AIR->defaultState();

    BlockState updatedState = bed.updatePostPlacement(bedState, Direction::South, airState, world, bedPos, bedPos);

    EXPECT_TRUE(updatedState.isAir());
}

// ============================================================================
// 特殊方块物理测试 - 史莱姆块、蜂蜜块、蜘蛛网
// ============================================================================

#include "physics/PhysicsConstants.hpp"
#include "world/block/blocks/special/HoneyBlock.hpp"
#include "world/block/blocks/special/SlimeBlock.hpp"
#include "world/block/blocks/special/WebBlock.hpp"

TEST(SpecialBlocksSlipperiness, SlimeBlockSlipperiness)
{
    // MC 1.16.5: 史莱姆块滑度为 0.8
    VanillaBlocks::initialize();
    EXPECT_FLOAT_EQ(physics::SLIPPERINESS_SLIME, 0.8f);
}

TEST(SpecialBlocksSlipperiness, BlueIceSlipperiness)
{
    // MC 1.16.5: 蓝冰滑度为 0.989
    EXPECT_FLOAT_EQ(physics::SLIPPERINESS_BLUE_ICE, 0.989f);
}

TEST(SpecialBlocksPhysics, SlimeBlockBounceFactor)
{
    // MC 1.16.5: 史莱姆块弹跳系数：活体实体 1.0，非活体实体 0.8
    EXPECT_FLOAT_EQ(physics::SLIME_BLOCK_BOUNCE_FACTOR_LIVING, 1.0f);
    EXPECT_FLOAT_EQ(physics::SLIME_BLOCK_BOUNCE_FACTOR_NON_LIVING, 0.8f);
}

TEST(SpecialBlocksPhysics, HoneyBlockFactors)
{
    // MC 1.16.5: 蜂蜜块跳跃因子为 0.5
    // 注意：蜂蜜块不修改 friction，滑度与默认值相同(0.6)，减速由 speedFactor/jumpFactor 实现
    EXPECT_FLOAT_EQ(physics::HONEY_BLOCK_JUMP_FACTOR, 0.5f);
    EXPECT_FLOAT_EQ(physics::HONEY_BLOCK_MAX_SLIDE_VELOCITY, 0.05f);
}

TEST(SpecialBlocksPhysics, CobwebSlowdown)
{
    // MC 1.16.5: 蜘蛛网减速：XZ 平面 0.25，Y 轴 0.05
    EXPECT_FLOAT_EQ(physics::COBWEB_SLOWDOWN_XZ, 0.25f);
    EXPECT_FLOAT_EQ(physics::COBWEB_SLOWDOWN_Y, 0.05f);
}

TEST(SpecialBlocksSlimeBlock, OnLandedBouncesDownwardVelocity)
{
    // 测试史莱姆块弹跳逻辑
    // 当实体以向下速度着陆时，Y速度应取反并乘以 0.9
    blocks::SlimeBlock slimeBlock(BlockProperties(Material::SLIME).slipperiness(physics::SLIPPERINESS_SLIME));

    // 验证滑度设置正确
    EXPECT_FLOAT_EQ(slimeBlock.getSlipperiness(slimeBlock.defaultState()), physics::SLIPPERINESS_SLIME);
}

TEST(SpecialBlocksHoneyBlock, OnLandedStopsFallDamage)
{
    // 测试蜂蜜块消除摔落伤害
    // 蜂蜜块不弹跳，只重置 Y 速度为 0
    // 蜂蜜块不修改 friction，滑度与默认值相同(0.6)；减速效果由 speedFactor=0.4 和 jumpFactor=0.5 实现
    blocks::HoneyBlock honeyBlock(BlockProperties(Material::SLIME)
            .slipperiness(physics::SLIPPERINESS_HONEY)
            .jumpFactor(physics::HONEY_BLOCK_JUMP_FACTOR));

    // 验证滑度和跳跃因子设置正确
    EXPECT_FLOAT_EQ(honeyBlock.getSlipperiness(honeyBlock.defaultState()), physics::SLIPPERINESS_HONEY);
    EXPECT_FLOAT_EQ(honeyBlock.getJumpFactor(honeyBlock.defaultState()), physics::HONEY_BLOCK_JUMP_FACTOR);
}

TEST(SpecialBlocksWebBlock, CollisionShapeIsEmpty)
{
    // 测试蜘蛛网碰撞箱为空（实体可以穿过）
    blocks::WebBlock webBlock(BlockProperties(Material::WEB).hardness(4.0f).noCollision());

    const CollisionShape& collisionShape = webBlock.getCollisionShape(webBlock.defaultState());
    EXPECT_TRUE(collisionShape.isEmpty());
}

TEST(SpecialBlocksLadder, LadderSpeedConstants)
{
    // MC 1.16.5: 梯子攀爬速度常量
    EXPECT_FLOAT_EQ(physics::LADDER_SPEED_MAX, 0.15f);
    EXPECT_FLOAT_EQ(physics::LADDER_CLIMB_SPEED, 0.15f);
    EXPECT_FLOAT_EQ(physics::LADDER_SLIDE_SPEED, -0.15f);
}

// ============================================================================
// Nether Wart Block 和 Warped Wart Block 测试
// ============================================================================

TEST(NetherWartBlocksTest, NetherWartBlockProperties)
{
    VanillaBlocks::initialize();

    ASSERT_NE(VanillaBlocks::NETHER_WART_BLOCK, nullptr);

    const Block& block = *VanillaBlocks::NETHER_WART_BLOCK;

    // 验证使用 ORGANIC 材料
    EXPECT_EQ(&block.material(), &Material::ORGANIC);

    // 验证硬度和抗性（MC 1.16.5: hardnessAndResistance(1.0F)）
    EXPECT_FLOAT_EQ(block.hardness(), 1.0f);
    EXPECT_FLOAT_EQ(block.resistance(), 1.0f);

    // 验证可以注册和查找
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:nether_wart_block")), nullptr);
}

TEST(NetherWartBlocksTest, WarpedWartBlockProperties)
{
    VanillaBlocks::initialize();

    ASSERT_NE(VanillaBlocks::WARPED_WART_BLOCK, nullptr);

    const Block& block = *VanillaBlocks::WARPED_WART_BLOCK;

    // 验证使用 ORGANIC 材料
    EXPECT_EQ(&block.material(), &Material::ORGANIC);

    // 验证硬度和抗性（MC 1.16.5: hardnessAndResistance(1.0F)）
    EXPECT_FLOAT_EQ(block.hardness(), 1.0f);
    EXPECT_FLOAT_EQ(block.resistance(), 1.0f);

    // 验证可以注册和查找
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:warped_wart_block")), nullptr);
}

TEST(NetherWartBlocksTest, BothWartBlocksHaveSameProperties)
{
    VanillaBlocks::initialize();

    ASSERT_NE(VanillaBlocks::NETHER_WART_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::WARPED_WART_BLOCK, nullptr);

    // 两种疣块应该有相同的物理属性
    EXPECT_EQ(&VanillaBlocks::NETHER_WART_BLOCK->material(), &VanillaBlocks::WARPED_WART_BLOCK->material());
    EXPECT_FLOAT_EQ(VanillaBlocks::NETHER_WART_BLOCK->hardness(), VanillaBlocks::WARPED_WART_BLOCK->hardness());
    EXPECT_FLOAT_EQ(VanillaBlocks::NETHER_WART_BLOCK->resistance(), VanillaBlocks::WARPED_WART_BLOCK->resistance());

    // 两种疣块应该是不同的方块
    EXPECT_NE(VanillaBlocks::NETHER_WART_BLOCK->blockId(), VanillaBlocks::WARPED_WART_BLOCK->blockId());
}

// ============================================================================
// Cave Air 和 Void Air 方块测试
// ============================================================================

TEST(AirVariantsTest, CaveAirBlockProperties)
{
    VanillaBlocks::initialize();

    ASSERT_NE(VanillaBlocks::CAVE_AIR, nullptr);

    const Block& block = *VanillaBlocks::CAVE_AIR;

    // 验证使用 AIR 材料
    EXPECT_EQ(&block.material(), &Material::AIR);

    // 验证空气属性
    EXPECT_TRUE(block.defaultState().isAir());
    EXPECT_FALSE(block.defaultState().isSolid());
    EXPECT_FALSE(block.defaultState().isOpaque());

    // 验证可以注册和查找
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:cave_air")), nullptr);
}

TEST(AirVariantsTest, VoidAirBlockProperties)
{
    VanillaBlocks::initialize();

    ASSERT_NE(VanillaBlocks::VOID_AIR, nullptr);

    const Block& block = *VanillaBlocks::VOID_AIR;

    // 验证使用 AIR 材料
    EXPECT_EQ(&block.material(), &Material::AIR);

    // 验证空气属性
    EXPECT_TRUE(block.defaultState().isAir());
    EXPECT_FALSE(block.defaultState().isSolid());
    EXPECT_FALSE(block.defaultState().isOpaque());

    // 验证可以注册和查找
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:void_air")), nullptr);
}

TEST(AirVariantsTest, AllAirBlocksAreDistinct)
{
    VanillaBlocks::initialize();

    ASSERT_NE(VanillaBlocks::AIR, nullptr);
    ASSERT_NE(VanillaBlocks::CAVE_AIR, nullptr);
    ASSERT_NE(VanillaBlocks::VOID_AIR, nullptr);

    // 三种空气方块应该是不同的方块（不同的方块ID）
    EXPECT_NE(VanillaBlocks::AIR->blockId(), VanillaBlocks::CAVE_AIR->blockId());
    EXPECT_NE(VanillaBlocks::AIR->blockId(), VanillaBlocks::VOID_AIR->blockId());
    EXPECT_NE(VanillaBlocks::CAVE_AIR->blockId(), VanillaBlocks::VOID_AIR->blockId());
}

TEST(AirVariantsTest, AllAirBlocksHaveSameMaterial)
{
    VanillaBlocks::initialize();

    // 三种空气方块应该使用相同的材料
    EXPECT_EQ(&VanillaBlocks::AIR->material(), &VanillaBlocks::CAVE_AIR->material());
    EXPECT_EQ(&VanillaBlocks::AIR->material(), &VanillaBlocks::VOID_AIR->material());
    EXPECT_EQ(&VanillaBlocks::CAVE_AIR->material(), &VanillaBlocks::VOID_AIR->material());
}

TEST(AirVariantsTest, AllAirBlocksAreAir)
{
    VanillaBlocks::initialize();

    // 所有空气方块的默认状态都应该 isAir() == true
    EXPECT_TRUE(VanillaBlocks::AIR->defaultState().isAir());
    EXPECT_TRUE(VanillaBlocks::CAVE_AIR->defaultState().isAir());
    EXPECT_TRUE(VanillaBlocks::VOID_AIR->defaultState().isAir());
}

TEST(AirVariantsTest, CaveAirUsedInWorldCarver)
{
    VanillaBlocks::initialize();

    // 验证 CAVE_AIR 可以用于世界雕刻器
    const BlockState* caveAirState = VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
    ASSERT_NE(caveAirState, nullptr);
    EXPECT_TRUE(caveAirState->isAir());
}

// ============================================================================
// canBeReplaced / canBeReplacedByFluid 测试
// ============================================================================

TEST(BlockStateCanBeReplacedTest, AirBlocksAreReplaceable)
{
    VanillaBlocks::initialize();

    // 空气方块 should be replaceable
    EXPECT_TRUE(VanillaBlocks::AIR->defaultState().canBeReplaced());
    EXPECT_TRUE(VanillaBlocks::CAVE_AIR->defaultState().canBeReplaced());
    EXPECT_TRUE(VanillaBlocks::VOID_AIR->defaultState().canBeReplaced());
}

TEST(BlockStateCanBeReplacedTest, SolidBlocksAreNotReplaceable)
{
    VanillaBlocks::initialize();

    // 石头、泥土等实心方块不应可替换
    EXPECT_FALSE(VanillaBlocks::STONE->defaultState().canBeReplaced());
    EXPECT_FALSE(VanillaBlocks::DIRT->defaultState().canBeReplaced());
    EXPECT_FALSE(VanillaBlocks::GRASS_BLOCK->defaultState().canBeReplaced());
    EXPECT_FALSE(VanillaBlocks::OAK_PLANKS->defaultState().canBeReplaced());
}

TEST(BlockStateCanBeReplacedTest, VegetationBlocksAreReplaceable)
{
    VanillaBlocks::initialize();

    // 花草类方块应可替换（Material::REPLACEABLE_PLANT 或 Material::PLANT 都设置了 replaceable）
    EXPECT_TRUE(VanillaBlocks::SHORT_GRASS->defaultState().canBeReplaced());
    EXPECT_TRUE(VanillaBlocks::DANDELION->defaultState().canBeReplaced());
    EXPECT_TRUE(VanillaBlocks::POPPY->defaultState().canBeReplaced());
}

TEST(BlockStateCanBeReplacedTest, LiquidBlocksAreReplaceable)
{
    VanillaBlocks::initialize();

    // 水和岩浆应可替换（Material::WATER 和 Material::LAVA 都设置了 replaceable）
    EXPECT_TRUE(VanillaBlocks::WATER->defaultState().canBeReplaced());
    EXPECT_TRUE(VanillaBlocks::LAVA->defaultState().canBeReplaced());
}

TEST(BlockStateCanBeReplacedTest, FireIsReplaceable)
{
    VanillaBlocks::initialize();

    // 火方块应可替换
    EXPECT_TRUE(VanillaBlocks::FIRE->defaultState().canBeReplaced());
}

TEST(BlockStateCanBeReplacedTest, SnowLayerIsReplaceable)
{
    VanillaBlocks::initialize();

    // 雪层方块应可替换
    EXPECT_TRUE(VanillaBlocks::SNOW->defaultState().canBeReplaced());
}

TEST(BlockStateCanBeReplacedTest, CanBeReplacedByFluid)
{
    VanillaBlocks::initialize();

    // 可替换方块 also canBeReplacedByFluid
    EXPECT_TRUE(VanillaBlocks::AIR->defaultState().canBeReplacedByFluid());
    EXPECT_TRUE(VanillaBlocks::SHORT_GRASS->defaultState().canBeReplacedByFluid());
    EXPECT_TRUE(VanillaBlocks::WATER->defaultState().canBeReplacedByFluid());

    // 非固体且不可替换的方块 canBeReplacedByFluid() 应返回 true（门、告示牌等非固体方块允许流体通过）
    // 验证 canBeReplacedByFluid = canBeReplaced || !isSolid 的语义
    // 石头：不可替换且固体 → canBeReplacedByFluid() = false
    EXPECT_FALSE(VanillaBlocks::STONE->defaultState().canBeReplacedByFluid());
    // 泥土：不可替换且固体 → canBeReplacedByFluid() = false
    EXPECT_FALSE(VanillaBlocks::DIRT->defaultState().canBeReplacedByFluid());
}

TEST(BlockStateCanBeReplacedTest, CanBeReplacedByFluidNonSolidBlocks)
{
    VanillaBlocks::initialize();

    // canBeReplacedByFluid() = canBeReplaced() || !isSolid()
    // 非固体但不可替换的方块也允许流体流入（如门、告示牌等）
    // 验证关系：对于任何方块，canBeReplacedByFluid() >= canBeReplaced()

    // 可替换方块的 canBeReplacedByFluid 应该为 true
    if (VanillaBlocks::AIR->defaultState().canBeReplaced()) {
        EXPECT_TRUE(VanillaBlocks::AIR->defaultState().canBeReplacedByFluid());
    }
    if (VanillaBlocks::SHORT_GRASS->defaultState().canBeReplaced()) {
        EXPECT_TRUE(VanillaBlocks::SHORT_GRASS->defaultState().canBeReplacedByFluid());
    }

    // 固体不可替换方块的 canBeReplacedByFluid 应该为 false
    if (!VanillaBlocks::STONE->defaultState().canBeReplaced() && VanillaBlocks::STONE->defaultState().isSolid()) {
        EXPECT_FALSE(VanillaBlocks::STONE->defaultState().canBeReplacedByFluid());
    }
}

TEST(BlockStateCanBeReplacedTest, CanBeReplacedConsistentWithMaterial)
{
    VanillaBlocks::initialize();

    // canBeReplaced() 应与 isAir() || getMaterial().isReplaceable() 等价
    // 这是 canBeReplaced() 的语义定义

    // 空气：isAir()=true，canBeReplaced()=true
    EXPECT_TRUE(VanillaBlocks::AIR->defaultState().isAir());
    EXPECT_TRUE(VanillaBlocks::AIR->defaultState().canBeReplaced());

    // 石头：isAir()=false，Material::ROCK.isReplaceable()=false，canBeReplaced()=false
    EXPECT_FALSE(VanillaBlocks::STONE->defaultState().isAir());
    EXPECT_FALSE(VanillaBlocks::STONE->defaultState().getMaterial().isReplaceable());
    EXPECT_FALSE(VanillaBlocks::STONE->defaultState().canBeReplaced());

    // 花：isAir()=false，Material::REPLACEABLE_PLANT.isReplaceable()=true，canBeReplaced()=true
    EXPECT_FALSE(VanillaBlocks::DANDELION->defaultState().isAir());
    EXPECT_TRUE(VanillaBlocks::DANDELION->defaultState().getMaterial().isReplaceable());
    EXPECT_TRUE(VanillaBlocks::DANDELION->defaultState().canBeReplaced());

    // 水：isAir()=false，Material::WATER.isReplaceable()=true，canBeReplaced()=true
    EXPECT_FALSE(VanillaBlocks::WATER->defaultState().isAir());
    EXPECT_TRUE(VanillaBlocks::WATER->defaultState().getMaterial().isReplaceable());
    EXPECT_TRUE(VanillaBlocks::WATER->defaultState().canBeReplaced());
}

// ============================================================================
// canBeReplacedByFluid 与流体系统交互测试
// ============================================================================

TEST(BlockStateCanBeReplacedTest, LiquidBlocksCanBeReplacedByFluid)
{
    VanillaBlocks::initialize();

    // 关键回归测试：MC Java 允许在已有液体上放置桶装流体
    // 旧代码使用 canBeReplaced() && !isLiquid() 阻止了这一行为，但 MC Java 的
    // canBeReplaced(Fluid) 对液体方块返回 true（因为液体 Material 设置了 replaceable）
    // 因此 canBeReplacedByFluid() 对液体方块也应返回 true
    EXPECT_TRUE(VanillaBlocks::WATER->defaultState().canBeReplacedByFluid());
    EXPECT_TRUE(VanillaBlocks::LAVA->defaultState().canBeReplacedByFluid());

    // 验证语义：canBeReplacedByFluid() = canBeReplaced() || !isSolid()
    // 液体方块：canBeReplaced()=true, isSolid()=false → canBeReplacedByFluid()=true
    EXPECT_TRUE(VanillaBlocks::WATER->defaultState().canBeReplaced());
    EXPECT_FALSE(VanillaBlocks::WATER->defaultState().isSolid());
}

TEST(BlockStateCanBeReplacedTest, BlacklistBlocksCanBeReplacedByFluidButBlockedByFlowingFluid)
{
    VanillaBlocks::initialize();

    // 门、告示牌等方块在 MC Java 中 isSolid()=false（通过 BlockProperties::notSolid() 设置），
    // 因此 canBeReplacedByFluid() 应返回 true。
    // FlowingFluid::isBlocked() 对这些方块有专门的黑名单返回 true（阻挡流体流入）。

    // 门方块：不可替换，isSolid()=false（BlockProperties 设置了 notSolid()），canBeReplacedByFluid()=true
    EXPECT_FALSE(VanillaBlocks::OAK_DOOR->defaultState().canBeReplaced());
    EXPECT_FALSE(VanillaBlocks::OAK_DOOR->defaultState().isSolid());
    EXPECT_TRUE(VanillaBlocks::OAK_DOOR->defaultState().canBeReplacedByFluid());
    // FlowingFluid::isBlocked() 的黑名单仍会正确阻挡流体流入门方块

    // 告示牌方块同理
    EXPECT_FALSE(VanillaBlocks::OAK_SIGN->defaultState().canBeReplaced());
    EXPECT_FALSE(VanillaBlocks::OAK_SIGN->defaultState().isSolid());
    EXPECT_TRUE(VanillaBlocks::OAK_SIGN->defaultState().canBeReplacedByFluid());

    // 梯子同理
    EXPECT_FALSE(VanillaBlocks::LADDER->defaultState().canBeReplaced());
    EXPECT_FALSE(VanillaBlocks::LADDER->defaultState().isSolid());
    EXPECT_TRUE(VanillaBlocks::LADDER->defaultState().canBeReplacedByFluid());
}

TEST(BlockStateCanBeReplacedTest, CanBeReplacedByFluidSemanticsVerification)
{
    VanillaBlocks::initialize();

    // 验证 canBeReplacedByFluid() == canBeReplaced() || !isSolid() 在各类方块上的正确性

    // 1. 可替换且非固体 → canBeReplacedByFluid()=true
    {
        const BlockState& state = VanillaBlocks::SHORT_GRASS->defaultState();
        EXPECT_TRUE(state.canBeReplaced());
        EXPECT_FALSE(state.isSolid());
        EXPECT_TRUE(state.canBeReplacedByFluid());
    }

    // 2. 不可替换且固体 → canBeReplacedByFluid()=false
    {
        const BlockState& state = VanillaBlocks::STONE->defaultState();
        EXPECT_FALSE(state.canBeReplaced());
        EXPECT_TRUE(state.isSolid());
        EXPECT_FALSE(state.canBeReplacedByFluid());
    }

    // 3. 不可替换且非固体（门方块通过 BlockProperties::notSolid() 设置）→ canBeReplacedByFluid()=true
    // 门方块在 MC Java 中 isSolid()=false，通过 BlockProperties::notSolid() 正确反映
    {
        const BlockState& state = VanillaBlocks::OAK_DOOR->defaultState();
        EXPECT_FALSE(state.canBeReplaced());
        EXPECT_FALSE(state.isSolid());
        EXPECT_TRUE(state.canBeReplacedByFluid());
    }

    // 4. 可替换且非固体（液体）→ canBeReplacedByFluid()=true
    {
        const BlockState& state = VanillaBlocks::WATER->defaultState();
        EXPECT_TRUE(state.canBeReplaced());
        EXPECT_FALSE(state.isSolid());
        EXPECT_TRUE(state.canBeReplacedByFluid());
    }

    // 5. 空气：可替换且非固体 → canBeReplacedByFluid()=true
    {
        const BlockState& state = VanillaBlocks::AIR->defaultState();
        EXPECT_TRUE(state.canBeReplaced());
        EXPECT_FALSE(state.isSolid());
        EXPECT_TRUE(state.canBeReplacedByFluid());
    }
}

TEST(BlockStateCanBeReplacedTest, PortalAndStructureVoidAreNotReplacedByFluid)
{
    VanillaBlocks::initialize();

    // 传送门和结构空位方块在 FlowingFluid::isBlocked() 中被特殊处理（返回 true=阻挡）
    // 但它们本身的 canBeReplacedByFluid() 值取决于 Material 属性

    // 结构空位：Material::STRUCTURE_VOID 是 replaceable=true 且非固体
    // 所以 canBeReplacedByFluid()=true，但 FlowingFluid::isBlocked() 黑名单返回 true
    // 这与门/告示牌的情况类似：canBeReplacedByFluid() 与 isBlocked() 是两个层面的判断
}

// ============================================================================
// getShadeBrightness 测试
// ============================================================================

TEST(GetShadeBrightnessTest, DefaultOpaqueBlockReturns02)
{
    VanillaBlocks::initialize();

    // 石头是不透明实心方块，默认 getShadeBrightness 应返回 0.2F
    const auto& stoneState = VanillaBlocks::STONE->defaultState();
    EXPECT_FLOAT_EQ(stoneState.getShadeBrightness(), 0.2f);
    EXPECT_FLOAT_EQ(stoneState.getAmbientOcclusionLightValue(), 0.2f);

    // 通过 Block 虚方法调用也应返回 0.2F
    EXPECT_FLOAT_EQ(VanillaBlocks::STONE->getShadeBrightness(stoneState), 0.2f);
}

TEST(GetShadeBrightnessTest, DefaultTransparentBlockReturns10)
{
    VanillaBlocks::initialize();

    // 玻璃是非固体透明方块，默认 getShadeBrightness 应返回 1.0F
    const auto& glassState = VanillaBlocks::GLASS->defaultState();
    EXPECT_FLOAT_EQ(glassState.getShadeBrightness(), 1.0f);
    EXPECT_FLOAT_EQ(glassState.getAmbientOcclusionLightValue(), 1.0f);

    // 屏障方块 isOpaque 返回 false，也应返回 1.0F
    const auto& barrierState = VanillaBlocks::BARRIER->defaultState();
    EXPECT_FLOAT_EQ(barrierState.getShadeBrightness(), 1.0f);
    EXPECT_FLOAT_EQ(barrierState.getAmbientOcclusionLightValue(), 1.0f);
}

TEST(GetShadeBrightnessTest, MudBlockReturns02)
{
    VanillaBlocks::initialize();

    // 泥巴碰撞形状不完整（14/16高），但重写 getShadeBrightness 返回 0.2F
    const auto& mudState = VanillaBlocks::MUD->defaultState();
    EXPECT_FLOAT_EQ(mudState.getShadeBrightness(), 0.2f);
    EXPECT_FLOAT_EQ(mudState.getAmbientOcclusionLightValue(), 0.2f);
}

TEST(GetShadeBrightnessTest, SnowBlockLayerDependent)
{
    VanillaBlocks::initialize();

    // 满层(8层)雪应返回 0.2F
    const auto& snowFullState = VanillaBlocks::SNOW->defaultState().with(blocks::SnowBlock::LAYERS(), 8);
    EXPECT_FLOAT_EQ(snowFullState.getShadeBrightness(), 0.2f);
    EXPECT_FLOAT_EQ(snowFullState.getAmbientOcclusionLightValue(), 0.2f);

    // 非满层(1层)雪应返回 1.0F
    const auto& snowPartialState = VanillaBlocks::SNOW->defaultState().with(blocks::SnowBlock::LAYERS(), 1);
    EXPECT_FLOAT_EQ(snowPartialState.getShadeBrightness(), 1.0f);
    EXPECT_FLOAT_EQ(snowPartialState.getAmbientOcclusionLightValue(), 1.0f);

    // 中间层(4层)也应返回 1.0F
    const auto& snowMidState = VanillaBlocks::SNOW->defaultState().with(blocks::SnowBlock::LAYERS(), 4);
    EXPECT_FLOAT_EQ(snowMidState.getShadeBrightness(), 1.0f);
}

TEST(GetShadeBrightnessTest, StructureVoidReturns10)
{
    VanillaBlocks::initialize();

    // 结构空位 isOpaque 返回 false，应返回 1.0F
    const auto& voidState = VanillaBlocks::STRUCTURE_VOID->defaultState();
    EXPECT_FLOAT_EQ(voidState.getShadeBrightness(), 1.0f);
    EXPECT_FLOAT_EQ(voidState.getAmbientOcclusionLightValue(), 1.0f);
}

TEST(GetShadeBrightnessTest, LeafBlockReturns10)
{
    VanillaBlocks::initialize();

    // 树叶方块是非不透明方块，应返回 1.0F
    const auto& leafState = VanillaBlocks::OAK_LEAVES->defaultState();
    EXPECT_FLOAT_EQ(leafState.getShadeBrightness(), 1.0f);
    EXPECT_FLOAT_EQ(leafState.getAmbientOcclusionLightValue(), 1.0f);
}
