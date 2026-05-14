#include <gtest/gtest.h>

#include "common/core/Constants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/explosion/ExplosionContext.hpp"
#include "common/world/explosion/ExplosionMode.hpp"

#include <memory>

using namespace mc;
using namespace mc::world::explosion;

namespace {

/**
 * @brief 测试用方块 - 高爆炸抗性
 */
class BlastResistantBlock final : public Block {
public:
    BlastResistantBlock()
        : Block(makeProperties())
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
        createBlockState(std::move(container));
    }

    [[nodiscard]] static BlockProperties makeProperties()
    {
        return BlockProperties(Material::ROCK).resistance(1200.0f);
    }
};

/**
 * @brief 测试用方块 - 低爆炸抗性
 */
class FragileBlock final : public Block {
public:
    FragileBlock()
        : Block(makeProperties())
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
        createBlockState(std::move(container));
    }

    [[nodiscard]] static BlockProperties makeProperties() { return BlockProperties(Material::EARTH).resistance(0.5f); }
};

// ============================================================================
// ExplosionMode 测试
// ============================================================================

TEST(ExplosionModeTest, EnumValues)
{
    EXPECT_EQ(static_cast<int>(ExplosionMode::None), 0);
    EXPECT_EQ(static_cast<int>(ExplosionMode::Break), 1);
    EXPECT_EQ(static_cast<int>(ExplosionMode::Destroy), 2);
}

// ============================================================================
// ExplosionContext 测试
// ============================================================================

TEST(ExplosionContextTest, DefaultResistance)
{
    ExplosionContext context;

    // 创建测试方块状态
    FragileBlock fragileBlock;
    const BlockState& fragileState = fragileBlock.defaultState();

    // 测试默认爆炸抗性获取
    auto resistance = context.getExplosionResistance(fragileState, nullptr);
    ASSERT_TRUE(resistance.has_value());
    EXPECT_FLOAT_EQ(resistance.value(), 0.5f);
}

TEST(ExplosionContextTest, CanDestroyBlock)
{
    ExplosionContext context;

    FragileBlock fragileBlock;
    const BlockState& fragileState = fragileBlock.defaultState();

    // 默认情况下，非空气方块都可以被破坏
    EXPECT_TRUE(context.canDestroyBlock(fragileState, 1.0f));
}

TEST(ExplosionContextTest, BlastResistantBlock)
{
    ExplosionContext context;

    BlastResistantBlock resistantBlock;
    const BlockState& resistantState = resistantBlock.defaultState();

    // 高抗性方块
    auto resistance = context.getExplosionResistance(resistantState, nullptr);
    ASSERT_TRUE(resistance.has_value());
    EXPECT_FLOAT_EQ(resistance.value(), 1200.0f);
}

// ============================================================================
// EntityExplosionContext 测试
// ============================================================================

TEST(EntityExplosionContextTest, DefaultBehavior)
{
    EntityExplosionContext context(nullptr);

    FragileBlock fragileBlock;
    const BlockState& fragileState = fragileBlock.defaultState();

    // 默认行为应该与基类相同
    auto resistance = context.getExplosionResistance(fragileState, nullptr);
    ASSERT_TRUE(resistance.has_value());
    EXPECT_FLOAT_EQ(resistance.value(), 0.5f);

    EXPECT_TRUE(context.canDestroyBlock(fragileState, 1.0f));
}

// ============================================================================
// 常量测试
// ============================================================================

TEST(ExplosionConstantsTest, RayGridSize)
{
    using namespace mc::game::explosion;
    EXPECT_EQ(RAY_GRID_SIZE, 16);
}

TEST(ExplosionConstantsTest, RayStepSize)
{
    using namespace mc::game::explosion;
    EXPECT_FLOAT_EQ(RAY_STEP_SIZE, 0.3f);
}

TEST(ExplosionConstantsTest, ResistanceCoefficients)
{
    using namespace mc::game::explosion;
    EXPECT_FLOAT_EQ(RESISTANCE_COEFFICIENT, 0.3f);
    EXPECT_FLOAT_EQ(INITIAL_STRENGTH_MIN, 0.7f);
    EXPECT_FLOAT_EQ(INITIAL_STRENGTH_RANGE, 0.6f);
}

TEST(ExplosionConstantsTest, TNTConstants)
{
    using namespace mc::game::explosion;
    EXPECT_FLOAT_EQ(TNT_RADIUS, 4.0f);
    EXPECT_FLOAT_EQ(CREEPER_RADIUS, 3.0f);
    EXPECT_FLOAT_EQ(CHARGED_CREEPER_RADIUS_MULTIPLIER, 2.0f);
}

TEST(ExplosionConstantsTest, DamageConstants)
{
    using namespace mc::game::explosion;
    EXPECT_FLOAT_EQ(DAMAGE_MULTIPLIER, 7.0f);
    EXPECT_FLOAT_EQ(ENTITY_RANGE_MULTIPLIER, 2.0f);
}

TEST(ExplosionConstantsTest, OtherConstants)
{
    using namespace mc::game::explosion;
    EXPECT_FLOAT_EQ(EXPLOSION_VOLUME, 4.0f);
    EXPECT_FLOAT_EQ(EXPLOSION_PITCH_BASE, 0.7f);
    EXPECT_FLOAT_EQ(EXPLOSION_PITCH_RANGE, 0.2f);
    EXPECT_FLOAT_EQ(FIRE_SPAWN_CHANCE, 0.333f);
}

} // namespace
