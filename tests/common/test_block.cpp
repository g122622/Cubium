#include <gtest/gtest.h>
#include "../src/common/util/property/StateHolder.hpp"
#include "../src/common/util/property/StateContainer.hpp"
#include "../src/common/world/block/Material.hpp"
#include "../src/common/world/block/Block.hpp"
#include "../src/common/world/block/BlockRegistry.hpp"
#include "../src/common/world/block/VanillaBlocks.hpp"
#include "../src/common/world/IWorld.hpp"
#include "../src/common/world/block/blocks/FallingBlock.hpp"
#include "../src/common/world/block/blocks/agricultural/FarmlandBlock.hpp"
#include "../src/common/world/block/blocks/vegetation/SugarCaneBlock.hpp"
#include "../src/common/world/fluid/Fluid.hpp"
#include "../src/common/world/fluid/FluidRegistry.hpp"
#include "../src/common/entity/core/Entity.hpp"
#include "../src/common/util/math/random/Random.hpp"
#include "../src/common/util/property/Properties.hpp"
#include <atomic>
#include <unordered_map>

using namespace mc;

// ============================================================================
// 测试用的简单方块类
// ============================================================================

class TestBlock : public Block {
public:
    explicit TestBlock(BlockProperties properties)
        : Block(properties) {
        // 创建空状态容器
        auto container = StateContainer<Block, BlockState>::Builder(*this)
            .create([](const Block& block, auto values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
        createBlockState(std::move(container));
    }
};

class TestBlockWithAxis : public Block {
public:
    explicit TestBlockWithAxis(BlockProperties properties)
        : Block(properties) {
        auto container = StateContainer<Block, BlockState>::Builder(*this)
            .addAxis("axis")
            .create([](const Block& block, auto values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
        createBlockState(std::move(container));
    }

    // 从 StateContainer 获取属性
    static const EnumProperty<Axis>& AXIS(const Block& block) {
        return *static_cast<const EnumProperty<Axis>*>(
            block.stateContainer().getProperty("axis"));
    }
};

class TestBlockWithFacing : public Block {
public:
    explicit TestBlockWithFacing(BlockProperties properties)
        : Block(properties) {
        auto container = StateContainer<Block, BlockState>::Builder(*this)
            .addHorizontalDirection("facing")
            .create([](const Block& block, auto values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
        createBlockState(std::move(container));
    }

    static const DirectionProperty& FACING(const Block& block) {
        return *static_cast<const DirectionProperty*>(
            block.stateContainer().getProperty("facing"));
    }
};

class TestBlockWithMultiple : public Block {
public:
    explicit TestBlockWithMultiple(BlockProperties properties)
        : Block(properties) {
        auto container = StateContainer<Block, BlockState>::Builder(*this)
            .addHorizontalDirection("facing")
            .addBoolean("lit")
            .create([](const Block& block, auto values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
        createBlockState(std::move(container));
    }

    static const DirectionProperty& FACING(const Block& block) {
        return *static_cast<const DirectionProperty*>(
            block.stateContainer().getProperty("facing"));
    }

    static const BooleanProperty& LIT(const Block& block) {
        return *static_cast<const BooleanProperty*>(
            block.stateContainer().getProperty("lit"));
    }
};

class TestBlockWithoutExplicitStateContainer : public Block {
public:
    explicit TestBlockWithoutExplicitStateContainer(BlockProperties properties)
        : Block(properties) {
    }
};

namespace {

ResourceLocation makeUniqueTestBlockId() {
    static std::atomic<u32> counter{0};
    const u32 suffix = ++counter;
    return ResourceLocation("test:auto_state_block_" + std::to_string(suffix));
}

class BlockRulesTestWorld final : public IBlockReader {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override {
        m_blocks[packPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr) {
                return fluidState;
            }
        }
        return fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= 0 && y < 256; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override {
        return {};
    }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] i32 difficulty() const override { return 0; }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override {
        (void)entity;
        ++m_spawnedEntityCount;
        return m_spawnEntityResult;
    }

    void setSpawnEntityResult(EntityId result) {
        m_spawnEntityResult = result;
    }

    [[nodiscard]] i32 spawnedEntityCount() const {
        return m_spawnedEntityCount;
    }

private:
    static i64 packPos(i32 x, i32 y, i32 z) {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    EntityId m_spawnEntityResult = 1;
    i32 m_spawnedEntityCount = 0;
};

} // namespace

// ============================================================================
// Material 测试
// ============================================================================

TEST(MaterialTest, PredefinedMaterials) {
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
}

TEST(MaterialTest, MaterialBuilder) {
    Material customMaterial = MaterialBuilder()
        .solid()
        .flammable()
        .opaque()
        .build();

    EXPECT_TRUE(customMaterial.isSolid());
    EXPECT_TRUE(customMaterial.isFlammable());
    EXPECT_TRUE(customMaterial.isOpaque());
    EXPECT_FALSE(customMaterial.isLiquid());
}

// ============================================================================
// BlockProperties 测试
// ============================================================================

TEST(BlockPropertiesTest, BasicProperties) {
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

TEST(BlockPropertiesTest, ChainProperties) {
    BlockProperties props = BlockProperties{Material::WOOD}
        .hardness(2.0f)
        .resistance(3.0f)
        .lightLevel(15)
        .flammable();

    EXPECT_EQ(props.hardness(), 2.0f);
    EXPECT_EQ(props.resistance(), 3.0f);
    EXPECT_EQ(props.lightLevel(), 15);
    EXPECT_TRUE(props.isFlammable());
}

TEST(BlockPropertiesTest, SpecialFlags) {
    BlockProperties noCollision = BlockProperties{Material::ROCK}.noCollision();
    EXPECT_FALSE(noCollision.hasCollision());

    BlockProperties notSolid = BlockProperties{Material::GLASS}.notSolid();
    EXPECT_FALSE(notSolid.isSolid());

    BlockProperties replaceable = BlockProperties{Material::PLANT}.replaceable();
    EXPECT_TRUE(replaceable.isReplaceable());
}

TEST(BlockPropertiesTest, Strength) {
    BlockProperties props = BlockProperties{Material::ROCK}.strength(2.5f);

    EXPECT_EQ(props.hardness(), 2.5f);
    EXPECT_EQ(props.resistance(), 2.5f);
}

TEST(BlockPropertiesTest, TransparentDefaultOpacityMatchesVanillaRule) {
    // 非不透明方块在未显式设置 opacity 时，默认应为 1（非全黑遮挡）。
    TestBlock glassLike{BlockProperties{Material::GLASS}.notSolid()};
    EXPECT_EQ(glassLike.defaultState().getOpacity(), 1);
}

TEST(BlockSoundTypeTest, DirtUsesVanillaGroundSoundEvents) {
    const auto& soundType = BlockSoundTypes::DIRT;

    EXPECT_EQ(soundType.getBreakSound().toString(), "minecraft:block.gravel.break");
    EXPECT_EQ(soundType.getStepSound().toString(), "minecraft:block.gravel.step");
    EXPECT_EQ(soundType.getPlaceSound().toString(), "minecraft:block.gravel.place");
    EXPECT_EQ(soundType.getHitSound().toString(), "minecraft:block.gravel.hit");
    EXPECT_EQ(soundType.getFallSound().toString(), "minecraft:block.gravel.fall");
}

TEST(BlockSoundTypeTest, GrassBlockUsesGrassSoundType) {
    VanillaBlocks::initialize();

    const auto& soundType = VanillaBlocks::GRASS_BLOCK->defaultState().getSoundType();
    EXPECT_EQ(soundType.getBreakSound().toString(), BlockSoundTypes::GRASS.getBreakSound().toString());
    EXPECT_EQ(soundType.getPlaceSound().toString(), BlockSoundTypes::GRASS.getPlaceSound().toString());
}

// ============================================================================
// StateContainer 测试
// ============================================================================

TEST(StateContainerTest, EmptyContainer) {
    TestBlock block{BlockProperties{Material::ROCK}.hardness(1.0f)};

    const auto& container = block.stateContainer();

    // 空容器应该有1个状态（基础状态）
    EXPECT_EQ(container.stateCount(), 1u);

    // 基础状态应该没有属性
    const auto& baseState = container.baseState();
    EXPECT_EQ(baseState.values().size(), 0u);
}

TEST(StateContainerTest, SingleProperty) {
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

TEST(StateContainerTest, MultipleProperties) {
    TestBlockWithMultiple block{BlockProperties{Material::ROCK}};

    const auto& container = block.stateContainer();

    // facing: 4 values * lit: 2 values = 8 states
    EXPECT_EQ(container.stateCount(), 8u);
}

TEST(StateContainerTest, GetProperty) {
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

TEST(BlockStateTest, GetProperty) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    Axis axis = state.get(TestBlockWithAxis::AXIS(block));
    // 默认值应该是第一个值 (X)
    EXPECT_EQ(axis, Axis::X);
}

TEST(BlockStateTest, SetProperty) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    // 设置新值
    const auto& newState = state.with(TestBlockWithAxis::AXIS(block), Axis::Y);
    EXPECT_EQ(newState.get(TestBlockWithAxis::AXIS(block)), Axis::Y);

    // 设置另一个值
    const auto& state3 = state.with(TestBlockWithAxis::AXIS(block), Axis::Z);
    EXPECT_EQ(state3.get(TestBlockWithAxis::AXIS(block)), Axis::Z);
}

TEST(BlockStateTest, SetPropertySameValue) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    // 设置相同的值应该返回同一个状态
    const auto& newState = state.with(TestBlockWithAxis::AXIS(block), Axis::X);
    EXPECT_EQ(&state, &newState);
}

TEST(BlockStateTest, CycleProperty) {
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

TEST(BlockStateTest, HasProperty) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();
    // 这个测试不需要 litProp

    EXPECT_TRUE(state.hasProperty(TestBlockWithAxis::AXIS(block)));
    // 该方块没有 lit 属性，跳过此测试  // 这个方块没有 lit 属性
}

TEST(BlockStateTest, StateId) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& states = block.stateContainer().validStates();

    // 每个状态应该有不同的ID
    std::set<u32> ids;
    for (const auto& s : states) {
        ids.insert(s->stateId());
    }
    EXPECT_EQ(ids.size(), states.size());
}

TEST(BlockStateTest, ToString) {
    TestBlockWithAxis block{BlockProperties{Material::WOOD}};
    const auto& state = block.defaultState();

    String str = state.toString();
    // 应该包含属性名和值
    EXPECT_TRUE(str.find("axis") != String::npos);
}

TEST(BlockStateTest, ToModelKeyUsesStableSortedOrder) {
    TestBlockWithMultiple block{BlockProperties{Material::ROCK}};
    const auto& state = block.defaultState()
        .with(TestBlockWithMultiple::FACING(block), Direction::West)
        .with(TestBlockWithMultiple::LIT(block), true);

    const String modelKey = state.toModelKey();
    EXPECT_EQ(modelKey, "facing=west,lit=true");
}

TEST(BlockStateTest, MultiplePropertiesInteraction) {
    TestBlockWithMultiple block{BlockProperties{Material::ROCK}};
    const auto& state = block.defaultState();

    // 获取并设置多个属性
    Direction facing = state.get(TestBlockWithMultiple::FACING(block));
    bool lit = state.get(TestBlockWithMultiple::LIT(block));

    const auto& state1 = state.with(TestBlockWithMultiple::FACING(block), Direction::East);
    EXPECT_EQ(state1.get(TestBlockWithMultiple::FACING(block)), Direction::East);
    EXPECT_EQ(state1.get(TestBlockWithMultiple::LIT(block)), lit);  // lit 应该不变

    const auto& state2 = state1.with(TestBlockWithMultiple::LIT(block), true);
    EXPECT_EQ(state2.get(TestBlockWithMultiple::FACING(block)), Direction::East);  // facing 应该不变
    EXPECT_EQ(state2.get(TestBlockWithMultiple::LIT(block)), true);
}

// ============================================================================
// Block 测试
// ============================================================================

TEST(BlockTest, BasicProperties) {
    TestBlock block{BlockProperties{Material::ROCK}.hardness(1.5f).resistance(6.0f)};

    EXPECT_EQ(block.hardness(), 1.5f);
    EXPECT_EQ(block.resistance(), 6.0f);
    // Material 是副本，比较属性而非地址
    EXPECT_EQ(block.material().isSolid(), Material::ROCK.isSolid());
}

TEST(BlockTest, DefaultState) {
    TestBlock block{BlockProperties{Material::ROCK}};

    const auto& state = block.defaultState();
    EXPECT_EQ(&state.owner(), &block);
}

TEST(BlockTest, IsAir) {
    // 初始化 VanillaBlocks 确保 AIR 存在
    VanillaBlocks::initialize();

    // 空气方块应该返回 true
    EXPECT_TRUE(VanillaBlocks::AIR->isAir(VanillaBlocks::AIR->defaultState()));

    // 普通方块不是空气
    TestBlock normalBlock{BlockProperties{Material::ROCK}};
    EXPECT_FALSE(normalBlock.isAir(normalBlock.defaultState()));
}

TEST(BlockTest, StateCount) {
    TestBlockWithMultiple block{BlockProperties{Material::ROCK}};

    // 4 directions * 2 lit values = 8 states
    EXPECT_EQ(block.stateContainer().stateCount(), 8u);
}

TEST(BlockTest, BlockWithoutExplicitStateContainer_HasValidDefaultState) {
    TestBlockWithoutExplicitStateContainer block{BlockProperties{Material::ROCK}};

    EXPECT_EQ(block.stateContainer().stateCount(), 1u);

    const auto& defaultState = block.defaultState();
    EXPECT_EQ(&defaultState.owner(), &block);
}

TEST(VoxelShapesTest, CubeAcceptsPixelCoordinates) {
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

TEST(VoxelShapesTest, CubeAcceptsNormalizedCoordinates) {
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

TEST(BlockRegistryTest, RegisterBlock) {
    // 注意：由于 VanillaBlocks::initialize() 可能已运行，blockId 可能不为 1
    auto& block = BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:test_block_reg1"),
        BlockProperties{Material::ROCK}
    );

    EXPECT_NE(&block, nullptr);
    EXPECT_EQ(block.blockLocation(), ResourceLocation("test:test_block_reg1"));
    EXPECT_GT(block.blockId(), 0u);  // ID should be > 0
}

TEST(BlockRegistryTest, RegisterBlockWithoutExplicitStateContainer) {
    auto& block = BlockRegistry::instance().registerBlock<TestBlockWithoutExplicitStateContainer>(
        makeUniqueTestBlockId(),
        BlockProperties{Material::ROCK}
    );

    EXPECT_EQ(block.stateContainer().stateCount(), 1u);

    const BlockState* state = BlockRegistry::instance().getBlockState(block.defaultState().stateId());
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->blockId(), block.blockId());
}

TEST(BlockRegistryTest, GetBlockById) {
    auto& registered = BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:test_block_reg2"),
        BlockProperties{Material::ROCK}
    );

    // 通过已注册方块的 ID 查找，确保返回相同的方块
    Block* retrieved = BlockRegistry::instance().getBlock(registered.blockId());
    EXPECT_EQ(retrieved, &registered);
}

TEST(BlockRegistryTest, GetBlockByLocation) {
    auto& registered = BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:test_block_reg3"),
        BlockProperties{Material::ROCK}
    );

    Block* retrieved = BlockRegistry::instance().getBlock(ResourceLocation("test:test_block_reg3"));
    EXPECT_EQ(retrieved, &registered);
}

TEST(BlockRegistryTest, DuplicateRegistrationReturnsExistingBlock) {
    const ResourceLocation id("test:duplicate_block_reg");
    const size_t countBefore = BlockRegistry::instance().blockCount();

    auto& first = BlockRegistry::instance().registerBlock<TestBlock>(
        id,
        BlockProperties{Material::ROCK}
    );
    auto& second = BlockRegistry::instance().registerBlock<TestBlock>(
        id,
        BlockProperties{Material::WOOD}
    );

    EXPECT_EQ(&first, &second);
    EXPECT_EQ(BlockRegistry::instance().blockCount(), countBefore + 1);
}

TEST(BlockRegistryTest, GetBlockState) {
    auto& block = BlockRegistry::instance().registerBlock<TestBlockWithAxis>(
        ResourceLocation("test:block_with_axis_reg"),
        BlockProperties{Material::WOOD}
    );

    // 获取默认状态
    const auto& defaultState = block.defaultState();
    BlockState* retrieved = BlockRegistry::instance().getBlockState(defaultState.stateId());
    EXPECT_EQ(retrieved, &defaultState);
}

TEST(BlockRegistryTest, ForEachBlock) {
    BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:blockA_reg"),
        BlockProperties{Material::ROCK}
    );
    BlockRegistry::instance().registerBlock<TestBlock>(
        ResourceLocation("test:blockB_reg"),
        BlockProperties{Material::WOOD}
    );

    int count = 0;
    BlockRegistry::instance().forEachBlock([&count](Block&) {
        count++;
    });

    EXPECT_GT(count, 0);
}

TEST(BlockRegistryTest, ForEachBlockState) {
    BlockRegistry::instance().registerBlock<TestBlockWithAxis>(
        ResourceLocation("test:block_with_axis_reg2"),
        BlockProperties{Material::WOOD}
    );

    int count = 0;
    BlockRegistry::instance().forEachBlockState([&count](const BlockState&) {
        count++;
    });

    // axis has 3 values, so at least 3 states
    EXPECT_GE(count, 3);
}

// ============================================================================
// VanillaBlocks 测试
// ============================================================================

TEST(VanillaBlocksTest, Initialization) {
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

TEST(BlockStateTest, Caching) {
    auto& block = BlockRegistry::instance().registerBlock<TestBlockWithMultiple>(
        ResourceLocation("test:block_caching"),
        BlockProperties{Material::ROCK}
    );

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

TEST(BlockRegistryTest, BasicBlocksRegistration) {
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

TEST(BlockRegistryTest, OreBlocksRegistration) {
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

TEST(BlockRegistryTest, LogBlocksRegistration) {
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

TEST(BlockRegistryTest, StoneVariantsRegistration) {
    VanillaBlocks::initialize();

    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:granite")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:diorite")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:andesite")), nullptr);
}

TEST(BlockRegistryTest, VegetationBlocksRegistration) {
    VanillaBlocks::initialize();

    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:short_grass")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:tall_grass")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:fern")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:dandelion")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:poppy")), nullptr);
}

TEST(BlockRegistryTest, NetherBlocksRegistration) {
    VanillaBlocks::initialize();

    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:netherrack")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:soul_sand")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:basalt")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:glowstone")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:end_stone")), nullptr);
}

TEST(BlockStateComparisonTest, IsComparisonWorks) {
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

TEST(BlockRegistryTest, UniqueBlockIds) {
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

TEST(VanillaBlocksTest, SandFamilyRegisteredAsFallingBlocks) {
    VanillaBlocks::initialize();

    EXPECT_NE(dynamic_cast<blocks::FallingBlock*>(VanillaBlocks::SAND), nullptr);
    EXPECT_NE(dynamic_cast<blocks::FallingBlock*>(VanillaBlocks::GRAVEL), nullptr);
    EXPECT_NE(dynamic_cast<blocks::FallingBlock*>(VanillaBlocks::RED_SAND), nullptr);
}

TEST(AgriculturalBehaviorTest, SugarCaneRequiresAdjacentWater) {
    fluid::FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    auto* sugarCaneBlock = dynamic_cast<blocks::SugarCaneBlock*>(VanillaBlocks::SUGAR_CANE);
    ASSERT_NE(sugarCaneBlock, nullptr);

    BlockRulesTestWorld world;
    world.setBlock(0, 63, 0, &VanillaBlocks::SAND->defaultState());

    const BlockPos canePos(0, 64, 0);
    const BlockState& caneState = VanillaBlocks::SUGAR_CANE->defaultState();

    EXPECT_FALSE(sugarCaneBlock->isValidPosition(caneState, world, canePos));

    world.setBlock(1, 63, 0, &VanillaBlocks::WATER->defaultState());
    EXPECT_TRUE(sugarCaneBlock->isValidPosition(caneState, world, canePos));
}

TEST(AgriculturalBehaviorTest, DryFarmlandWithoutCropsTurnsToDirt) {
    fluid::FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    auto* farmlandBlock = dynamic_cast<blocks::FarmlandBlock*>(VanillaBlocks::FARMLAND);
    ASSERT_NE(farmlandBlock, nullptr);

    BlockRulesTestWorld world;
    const BlockPos farmlandPos(0, 64, 0);
    world.setBlock(farmlandPos.x, farmlandPos.y, farmlandPos.z, &VanillaBlocks::FARMLAND->defaultState());

    BlockState farmlandState = VanillaBlocks::FARMLAND->defaultState();
    math::Random random(123456);
    farmlandBlock->randomTick(world, farmlandPos, farmlandState, random);

    const BlockState* updated = world.getBlockState(farmlandPos.x, farmlandPos.y, farmlandPos.z);
    ASSERT_NE(updated, nullptr);
    EXPECT_TRUE(updated->is(VanillaBlocks::DIRT));
}

TEST(FallingBlockBehaviorTest, UnsupportedSandSpawnsFallingEntity) {
    fluid::FluidRegistry::instance().initialize();
    VanillaBlocks::initialize();

    auto* fallingSand = dynamic_cast<blocks::FallingBlock*>(VanillaBlocks::SAND);
    ASSERT_NE(fallingSand, nullptr);

    BlockRulesTestWorld world;
    world.setSpawnEntityResult(1);

    const BlockPos sandPos(0, 70, 0);
    world.setBlock(sandPos.x, sandPos.y, sandPos.z, &VanillaBlocks::SAND->defaultState());

    BlockState sandState = VanillaBlocks::SAND->defaultState();
    fallingSand->tick(world, sandPos, sandState);

    EXPECT_EQ(world.spawnedEntityCount(), 1);
    const BlockState* stateAfterTick = world.getBlockState(sandPos.x, sandPos.y, sandPos.z);
    ASSERT_NE(stateAfterTick, nullptr);
    EXPECT_TRUE(stateAfterTick->isAir());
}
