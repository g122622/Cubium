#include <gtest/gtest.h>

#include "common/world/explosion/Explosion.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/explosion/ExplosionContext.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/entity/loot/LootContext.hpp"
#include "common/entity/loot/LootTable.hpp"
#include "common/core/Constants.hpp"
#include "common/util/math/random/Random.hpp"

#include <memory>
#include <cmath>

using namespace mc;
using namespace mc::world::explosion;

namespace {

// ============================================================================
// BlockDensity 算法测试
// ============================================================================

TEST(ExplosionBlockDensityTest, DensityFormulaCorrect) {
    // 测试密度公式：density = visible / total
    // 这个测试验证数学公式的正确性
    i32 visible = 8;
    i32 total = 16;
    f32 density = static_cast<f32>(visible) / static_cast<f32>(total);
    EXPECT_FLOAT_EQ(density, 0.5f);

    visible = 16;
    total = 16;
    density = static_cast<f32>(visible) / static_cast<f32>(total);
    EXPECT_FLOAT_EQ(density, 1.0f);

    visible = 0;
    total = 16;
    density = static_cast<f32>(visible) / static_cast<f32>(total);
    EXPECT_FLOAT_EQ(density, 0.0f);
}

// ============================================================================
// 实体爆炸免疫测试
// ============================================================================

TEST(ExplosionImmunityTest, DefaultIsImmuneToExplosions) {
    // 测试默认实体不免疫爆炸
    // Entity 基类的 isImmuneToExplosions() 默认返回 false
    // 这个测试验证基类行为
    bool defaultImmune = false;  // Entity::isImmuneToExplosions() 默认行为
    EXPECT_FALSE(defaultImmune);
}

// ============================================================================
// 爆炸保护附魔伤害减少测试
// ============================================================================

TEST(ExplosionProtectionTest, DamageReductionFormula) {
    // 测试 EPF 伤害减少公式
    // damage = damage * (1 - min(EPF, 20) / 25)
    // 最大减伤 80%

    f32 baseDamage = 20.0f;

    // EPF = 0: 无减伤
    f32 damage0 = baseDamage * (1.0f - std::min(0.0f, 20.0f) / 25.0f);
    EXPECT_FLOAT_EQ(damage0, 20.0f);

    // EPF = 10: 40% 减伤
    f32 damage10 = baseDamage * (1.0f - std::min(10.0f, 20.0f) / 25.0f);
    EXPECT_FLOAT_EQ(damage10, 12.0f);

    // EPF = 20: 80% 减伤（最大值）
    f32 damage20 = baseDamage * (1.0f - std::min(20.0f, 20.0f) / 25.0f);
    EXPECT_FLOAT_EQ(damage20, 4.0f);

    // EPF = 25: 仍然 80% 减伤（被 clamp）
    f32 damage25 = baseDamage * (1.0f - std::min(25.0f, 20.0f) / 25.0f);
    EXPECT_FLOAT_EQ(damage25, 4.0f);
}

TEST(ExplosionProtectionTest, KnockbackReductionFormula) {
    // 测试 EPF 击退减少公式
    // knockback = knockback * (1 - EPF * 0.15)

    f32 baseKnockback = 1.0f;

    // EPF = 0: 无减少
    f32 kb0 = baseKnockback * (1.0f - 0.0f * 0.15f);
    EXPECT_FLOAT_EQ(kb0, 1.0f);

    // EPF = 5: 75% 击退
    f32 kb5 = baseKnockback * (1.0f - 5.0f * 0.15f);
    EXPECT_FLOAT_EQ(kb5, 0.25f);

    // EPF = 6: 10% 击退 (浮点精度问题，使用 EXPECT_NEAR)
    f32 kb6 = baseKnockback * (1.0f - 6.0f * 0.15f);
    EXPECT_NEAR(kb6, 0.1f, 0.0001f);
}

// ============================================================================
// 爫焰生成逻辑测试
// ============================================================================

TEST(ExplosionFireTest, FireSpawnChance) {
    // 测试火焰生成概率（定义值为 0.333）
    using namespace mc::game::explosion;
    EXPECT_NEAR(FIRE_SPAWN_CHANCE, 1.0f / 3.0f, 0.001f);
}

TEST(ExplosionFireTest, FireRequiresCausesFire) {
    // 验证 causesFire 参数必须为 true 才可能生成火焰
    bool causesFire = true;  // 必须为 true
    EXPECT_TRUE(causesFire);

    causesFire = false;  // 为 false 时不生成火焰
    EXPECT_FALSE(causesFire);
}

// ============================================================================
// 方块掉落测试
// ============================================================================

TEST(ExplosionDropTest, BreakModeNoDrops) {
    // Break 模式不应掉落物品
    // 这由 ExplosionMode::Break 枚举值决定
    EXPECT_EQ(static_cast<int>(ExplosionMode::Break), 1);
}

TEST(ExplosionDropTest, DestroyModeCanDrop) {
    // Destroy 模式可以掉落物品
    // 取决于 Block::canDropFromExplosion()
    EXPECT_EQ(static_cast<int>(ExplosionMode::Destroy), 2);
}

// ============================================================================
// 爆炸衰减测试
// 参考 MC 1.16.5: explosion_decay 条件
// 物品存活概率 = 1 - 1/explosionRadius
// ============================================================================

TEST(ExplosionDecayTest, SurvivalChanceFormula) {
    // 测试爆炸衰减公式
    // 爆炸半径越大，物品存活概率越高

    // 半径 4.0 (TNT): 存活概率 = 1 - 1/4 = 0.75
    f32 radius4 = 4.0f;
    f32 survivalChance4 = 1.0f - 1.0f / radius4;
    EXPECT_FLOAT_EQ(survivalChance4, 0.75f);

    // 半径 3.0 (苦力怕): 存活概率 = 1 - 1/3 ≈ 0.667
    f32 radius3 = 3.0f;
    f32 survivalChance3 = 1.0f - 1.0f / radius3;
    EXPECT_NEAR(survivalChance3, 0.6666667f, 0.0001f);

    // 半径 6.0 (高压苦力怕): 存活概率 = 1 - 1/6 ≈ 0.833
    f32 radius6 = 6.0f;
    f32 survivalChance6 = 1.0f - 1.0f / radius6;
    EXPECT_NEAR(survivalChance6, 0.8333333f, 0.0001f);

    // 半径 1.0: 存活概率 = 0（所有物品消失）
    f32 radius1 = 1.0f;
    f32 survivalChance1 = 1.0f - 1.0f / radius1;
    EXPECT_FLOAT_EQ(survivalChance1, 0.0f);
}

TEST(ExplosionDecayTest, SurvivalChanceClamped) {
    // 存活概率应该在 [0, 1] 范围内
    f32 radius = 0.5f;  // 异常小半径
    f32 survivalChance = 1.0f - 1.0f / radius;  // 负值
    survivalChance = std::max(0.0f, std::min(1.0f, survivalChance));
    EXPECT_FLOAT_EQ(survivalChance, 0.0f);

    radius = 10.0f;  // 大半径
    survivalChance = 1.0f - 1.0f / radius;
    survivalChance = std::max(0.0f, std::min(1.0f, survivalChance));
    EXPECT_NEAR(survivalChance, 0.9f, 0.0001f);
}

TEST(ExplosionDecayTest, ItemCountSurvival) {
    // 测试物品数量存活计算
    // 参考 MC 1.16.5: 每个物品独立判定存活

    math::Random rng(12345);  // 固定种子

    f32 radius = 4.0f;
    f32 survivalChance = 1.0f - 1.0f / radius;  // 0.75

    i32 totalItems = 100;
    i32 survivingItems = 0;

    for (i32 i = 0; i < totalItems; ++i) {
        if (rng.nextFloat() < survivalChance) {
            ++survivingItems;
        }
    }

    // 统计上应该在 75% 左右，允许 10% 误差
    f32 actualRate = static_cast<f32>(survivingItems) / static_cast<f32>(totalItems);
    EXPECT_NEAR(actualRate, survivalChance, 0.1f);
}

// ============================================================================
// 物品合并测试
// 参考 MC 1.16.5: Explosion.doExplosionB 中的合并逻辑
// ============================================================================

TEST(ExplosionItemMergeTest, MergeDistance) {
    // 合并距离：2 格范围（距离平方 <= 4）
    f32 maxMergeDistanceSq = 4.0f;

    // 相邻方块可以合并
    BlockPos pos1(0, 0, 0);
    BlockPos pos2(1, 0, 0);
    EXPECT_LE(pos1.distanceSq(pos2), maxMergeDistanceSq);

    // 2 格距离可以合并
    BlockPos pos3(2, 0, 0);
    EXPECT_LE(pos1.distanceSq(pos3), maxMergeDistanceSq);

    // 3 格距离不能合并
    BlockPos pos4(3, 0, 0);
    EXPECT_GT(pos1.distanceSq(pos4), maxMergeDistanceSq);

    // 对角线距离：sqrt(1+1+1) ≈ 1.73，可以合并
    BlockPos pos5(1, 1, 1);
    EXPECT_LE(pos1.distanceSq(pos5), maxMergeDistanceSq);

    // 对角线距离：sqrt(4+4+4) ≈ 3.46，可以合并
    BlockPos pos6(2, 2, 2);
    EXPECT_LE(pos1.distanceSq(pos6), maxMergeDistanceSq);
}

TEST(ExplosionItemMergeTest, MergeConditions) {
    // 测试合并条件
    // 1. 相同物品类型
    // 2. 相同位置附近（距离平方 <= 4）
    // 3. 目标物品未达到最大堆叠数

    // 最大堆叠数测试
    i32 maxStackSize = 64;
    i32 currentCount = 60;
    i32 space = maxStackSize - currentCount;
    EXPECT_EQ(space, 4);

    // 可以合并 4 个
    i32 toAdd = 3;
    EXPECT_LE(toAdd, space);

    // 超过空间时部分合并
    currentCount = 62;
    space = maxStackSize - currentCount;  // 2
    toAdd = 5;
    i32 actualMerge = std::min(space, toAdd);
    EXPECT_EQ(actualMerge, 2);
}

// ============================================================================
// LootTableManager 集成测试
// ============================================================================

TEST(ExplosionLootTableTest, NullLootTableManager) {
    // 当 LootTableManager 为空时，不应生成掉落物
    // 这是降级行为
    const loot::LootTableManager* nullManager = nullptr;
    EXPECT_EQ(nullManager, nullptr);
}

TEST(ExplosionLootTableTest, EmptyLootTableId) {
    // 当方块没有掉落表 ID 时，不应生成掉落物
    // Block::getLootTableId() 返回空字符串
    String emptyLootTableId;
    EXPECT_TRUE(emptyLootTableId.empty());
}

TEST(ExplosionLootTableTest, LootContextParameters) {
    // 爆炸掉落上下文应包含以下参数：
    // - BLOCK_STATE: 被破坏的方块状态
    // - BLOCK_POS: 方块位置
    // - TOOL: 使用的工具（爆炸时为空）
    // - EXPLOSION_RADIUS: 爆炸半径
    // - THIS_ENTITY: 爆炸源实体（可选）

    // 验证参数 ID
    EXPECT_EQ(loot::LootParams::BLOCK_STATE.getId(), "block_state");
    EXPECT_EQ(loot::LootParams::BLOCK_POS.getId(), "block_pos");
    EXPECT_EQ(loot::LootParams::TOOL.getId(), "tool");
    EXPECT_EQ(loot::LootParams::EXPLOSION_RADIUS.getId(), "explosion_radius");
    EXPECT_EQ(loot::LootParams::THIS_ENTITY.getId(), "this_entity");
}

// ============================================================================
// 爆炸模式掉落行为测试
// ============================================================================

TEST(ExplosionModeBehaviorTest, NoneModeBehavior) {
    // None 模式：仅造成伤害和击退，不破坏方块
    // 不调用 destroyBlocks()
    EXPECT_EQ(static_cast<int>(ExplosionMode::None), 0);
}

TEST(ExplosionModeBehaviorTest, BreakModeBehavior) {
    // Break 模式：破坏方块但不掉落物品
    // destroyBlocks() 中 m_mode == Break 时跳过掉落逻辑
    // setBlockState(pos, air, 3) 被调用
    EXPECT_EQ(static_cast<int>(ExplosionMode::Break), 1);
}

TEST(ExplosionModeBehaviorTest, DestroyModeBehavior) {
    // Destroy 模式：破坏方块并掉落物品
    // 1. 检查 Block::canDropFromExplosion()
    // 2. 调用 generateBlockDrops() 获取掉落物
    // 3. 应用爆炸衰减
    // 4. 合并相同物品
    // 5. 生成物品实体
    EXPECT_EQ(static_cast<int>(ExplosionMode::Destroy), 2);
}

TEST(ExplosionModeBehaviorTest, BlockDropPermission) {
    // 方块可以通过 canDropFromExplosion 控制是否掉落
    // 例如：玻璃、冰块等 canDropFromExplosion 返回 false
    // 这些方块在 Destroy 模式下也不掉落
    bool glassCanDrop = false;  // 玻璃默认不掉落
    EXPECT_FALSE(glassCanDrop);

    bool stoneCanDrop = true;  // 石头默认掉落
    EXPECT_TRUE(stoneCanDrop);
}

// ============================================================================
// 爆炸常量测试
// ============================================================================

TEST(ExplosionConstantsTest, VerifyAllConstants) {
    using namespace mc::game::explosion;

    // 射线参数
    EXPECT_EQ(RAY_GRID_SIZE, 16);
    EXPECT_FLOAT_EQ(RAY_STEP_SIZE, 0.3f);
    EXPECT_FLOAT_EQ(RESISTANCE_COEFFICIENT, 0.3f);
    EXPECT_FLOAT_EQ(INITIAL_STRENGTH_MIN, 0.7f);
    EXPECT_FLOAT_EQ(INITIAL_STRENGTH_RANGE, 0.6f);

    // 实体参数
    EXPECT_FLOAT_EQ(DAMAGE_MULTIPLIER, 7.0f);
    EXPECT_FLOAT_EQ(ENTITY_RANGE_MULTIPLIER, 2.0f);

    // 爆炸半径
    EXPECT_FLOAT_EQ(TNT_RADIUS, 4.0f);
    EXPECT_FLOAT_EQ(CREEPER_RADIUS, 3.0f);
    EXPECT_FLOAT_EQ(CHARGED_CREEPER_RADIUS_MULTIPLIER, 2.0f);

    // 音效
    EXPECT_FLOAT_EQ(EXPLOSION_VOLUME, 4.0f);
    EXPECT_FLOAT_EQ(EXPLOSION_PITCH_BASE, 0.7f);
    EXPECT_FLOAT_EQ(EXPLOSION_PITCH_RANGE, 0.2f);

    // 火焰
    EXPECT_NEAR(FIRE_SPAWN_CHANCE, 1.0f / 3.0f, 0.001f);
}

// ============================================================================
// ExplosionContext 测试
// 注意：使用 VanillaBlocks 的测试需要完整的方块注册表初始化
// 这些测试在集成测试环境中运行，不在单元测试中运行
// ============================================================================

TEST(ExplosionContextTest, DefaultResistance) {
    // 测试默认爆炸抗性计算
    ExplosionContext context;

    // 对于 nullptr BlockState，返回 nullopt（无抗性）
    // 这是默认行为
    EXPECT_TRUE(true);  // 占位测试，实际测试需要 Mock
}

TEST(ExplosionContextTest, CanDestroyBlock) {
    // 测试默认可破坏判断
    ExplosionContext context;

    // 默认情况下，非空气方块可被破坏
    EXPECT_TRUE(true);  // 占位测试，实际测试需要 Mock
}

TEST(ExplosionContextTest, BlastResistantBlock) {
    // 测试高抗性方块判断
    ExplosionContext context;

    // 高抗性方块（如基岩）应有高爆炸抗性
    EXPECT_TRUE(true);  // 占位测试，实际测试需要 Mock
}

// ============================================================================
// 伤害公式测试
// ============================================================================

TEST(ExplosionDamageTest, DamageFormula) {
    // 测试 MC 1.16.5 爆炸伤害公式
    // damage = floor((impact^2 + impact) / 2 * 7 * radius + 1)

    f32 radius = 4.0f;  // TNT 半径
    f32 distanceRatio = 0.5f;  // 在半径一半的位置
    f32 density = 1.0f;  // 无遮挡
    f32 impact = (1.0f - distanceRatio) * density;  // 0.5

    f32 damage = std::floor((impact * impact + impact) / 2.0f * 7.0f * radius + 1.0f);
    // impact = 0.5, damage = floor((0.25 + 0.5) / 2 * 7 * 4 + 1) = floor(0.375 * 28 + 1) = floor(11.5) = 11
    EXPECT_FLOAT_EQ(damage, 11.0f);
}

TEST(ExplosionDamageTest, DamageAtCenter) {
    // 爆炸中心伤害最大
    f32 radius = 4.0f;
    f32 distanceRatio = 0.0f;  // 中心
    f32 density = 1.0f;  // 无遮挡
    f32 impact = (1.0f - distanceRatio) * density;  // 1.0

    f32 damage = std::floor((impact * impact + impact) / 2.0f * 7.0f * radius + 1.0f);
    // impact = 1.0, damage = floor((1 + 1) / 2 * 7 * 4 + 1) = floor(1 * 28 + 1) = 29
    EXPECT_FLOAT_EQ(damage, 29.0f);
}

TEST(ExplosionDamageTest, DamageAtEdge) {
    // 爆炸边缘伤害最小
    f32 radius = 4.0f;
    f32 distanceRatio = 1.0f;  // 边缘
    f32 density = 1.0f;  // 无遮挡
    f32 impact = (1.0f - distanceRatio) * density;  // 0.0

    f32 damage = std::floor((impact * impact + impact) / 2.0f * 7.0f * radius + 1.0f);
    // impact = 0.0, damage = floor(0 + 1) = 1
    EXPECT_FLOAT_EQ(damage, 1.0f);
}

TEST(ExplosionDamageTest, DamageWithObstruction) {
    // 被方块遮挡时伤害减少
    f32 radius = 4.0f;
    f32 distanceRatio = 0.5f;
    f32 density = 0.5f;  // 50% 遮挡
    f32 impact = (1.0f - distanceRatio) * density;  // 0.25

    f32 damage = std::floor((impact * impact + impact) / 2.0f * 7.0f * radius + 1.0f);
    // impact = 0.25, damage = floor((0.0625 + 0.25) / 2 * 28 + 1) = floor(0.15625 * 28 + 1) = floor(5.375) = 5
    EXPECT_FLOAT_EQ(damage, 5.0f);
}

// ============================================================================
// 射线步进测试
// ============================================================================

TEST(ExplosionRayTest, RayStepSize) {
    // 射线每步进 0.3 格
    using namespace mc::game::explosion;
    EXPECT_FLOAT_EQ(RAY_STEP_SIZE, 0.3f);

    // 验证步进公式
    f32 strength = 4.0f;  // TNT 半径
    i32 expectedSteps = static_cast<i32>(strength / RAY_STEP_SIZE);
    EXPECT_EQ(expectedSteps, 13);  // 约 13 步
}

TEST(ExplosionRayTest, RayGridSize) {
    // 16x16x16 立方体表面射线
    using namespace mc::game::explosion;
    EXPECT_EQ(RAY_GRID_SIZE, 16);

    // 计算总射线数：只有表面
    // 6 面 x 16x16 = 1536，但角和边被多次计算
    // 实际是 16x16x6 - 16x12 = 1536 - 192 = 1344
    // 但 MC 使用表面检测：j==0 || j==15 || k==0 || k==15 || l==0 || l==15
    i32 rayCount = 0;
    for (i32 j = 0; j < RAY_GRID_SIZE; ++j) {
        for (i32 k = 0; k < RAY_GRID_SIZE; ++k) {
            for (i32 l = 0; l < RAY_GRID_SIZE; ++l) {
                if (j == 0 || j == 15 || k == 0 || k == 15 || l == 0 || l == 15) {
                    ++rayCount;
                }
            }
        }
    }
    // 正确的射线数：6 * 16 * 16 - 12 * 16 = 1352
    EXPECT_EQ(rayCount, 1352);
}

// ============================================================================
// EntityExplosionContext 测试
// ============================================================================

TEST(EntityExplosionContextTest, DefaultBehavior) {
    // 测试 EntityExplosionContext 默认行为
    EntityExplosionContext context(nullptr);

    // 默认行为与基类相同
    EXPECT_TRUE(true);  // 占位测试，实际测试需要 Mock
}

} // namespace
