/**
 * @file EntityDropGameRuleTest.cpp
 * @brief 测试 doEntityDrops 游戏规则对实体掉落行为的影响
 *
 * 测试覆盖：
 * 1. GameRules::DO_ENTITY_DROPS 默认值为 true
 * 2. GameRules::DO_ENTITY_DROPS 设置为 false 时规则值正确
 * 3. AbstractMinecartEntity::dropItem 受 DO_ENTITY_DROPS 控制
 * 4. ChestMinecartEntity::dropItem 容器内容物受 DO_ENTITY_DROPS 控制
 * 5. FurnaceMinecartEntity::dropItem 额外掉落受 DO_ENTITY_DROPS 控制
 * 6. TNTMinecartEntity::dropItem 额外掉落受 DO_ENTITY_DROPS 控制
 * 7. BoatEntity::dropItem 受 DO_ENTITY_DROPS 控制
 * 8. PaintingEntity::dropItem 受 DO_ENTITY_DROPS 控制
 * 9. ItemFrameEntity::dropItem 受 DO_ENTITY_DROPS 控制
 * 10. LeashKnotEntity::dropItem 受 DO_ENTITY_DROPS 控制
 * 11. FallingBlockEntity 已有的 DO_ENTITY_DROPS 检查
 *
 * 参考 VehicleEntity.destroy()、ContainerEntity.chestVehicleDestroyed()、
 * Painting.dropItem()、ItemFrame.dropItem()、Leashable.tickLeash() 中的 ENTITY_DROPS 检查
 */

#include "common/world/gamerule/GameRule.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::gamerule;

// ============================================================================
// GameRules DO_ENTITY_DROPS 基础测试
// ============================================================================

class EntityDropGameRuleTest : public ::testing::Test {
protected:
    GameRules rules;
};

/**
 * @brief 测试 DO_ENTITY_DROPS 默认值为 true
 *
 * MC 中 doEntityDrops 默认为 true，实体被破坏时应该掉落物品
 */
TEST_F(EntityDropGameRuleTest, DoEntityDrops_DefaultIsTrue)
{
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::DO_ENTITY_DROPS)) << "DO_ENTITY_DROPS should default to true";
}

/**
 * @brief 测试 DO_ENTITY_DROPS 可以设置为 false
 *
 * 设置为 false 后，实体被破坏时不应该掉落物品
 */
TEST_F(EntityDropGameRuleTest, DoEntityDrops_CanBeSetToFalse)
{
    rules.setBoolean(GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::DO_ENTITY_DROPS))
        << "DO_ENTITY_DROPS should be false after setBoolean(false)";
}

/**
 * @brief 测试 DO_ENTITY_DROPS 可以恢复为 true
 */
TEST_F(EntityDropGameRuleTest, DoEntityDrops_CanBeRestoredToTrue)
{
    rules.setBoolean(GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::DO_ENTITY_DROPS));

    rules.setBoolean(GameRuleKeys::DO_ENTITY_DROPS, true, nullptr);
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::DO_ENTITY_DROPS))
        << "DO_ENTITY_DROPS should be true after setBoolean(true)";
}

/**
 * @brief 测试 DO_ENTITY_DROPS 的规则名称
 */
TEST_F(EntityDropGameRuleTest, DoEntityDrops_HasCorrectName)
{
    EXPECT_EQ(GameRuleKeys::DO_ENTITY_DROPS.getName(), "doEntityDrops")
        << "DO_ENTITY_DROPS key name should be 'doEntityDrops'";
}

/**
 * @brief 测试 DO_ENTITY_DROPS 属于 Drops 分类
 */
TEST_F(EntityDropGameRuleTest, DoEntityDrops_BelongsToDropsCategory)
{
    EXPECT_EQ(GameRuleKeys::DO_ENTITY_DROPS.getCategory(), GameRuleCategory::Drops)
        << "DO_ENTITY_DROPS should belong to Drops category";
}

// ============================================================================
// 相关游戏规则测试
// ============================================================================

/**
 * @brief 测试 DO_MOB_LOOT 默认值为 true
 */
TEST_F(EntityDropGameRuleTest, DoMobLoot_DefaultIsTrue)
{
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::DO_MOB_LOOT)) << "DO_MOB_LOOT should default to true";
}

/**
 * @brief 测试 DO_TILE_DROPS 默认值为 true
 */
TEST_F(EntityDropGameRuleTest, DoTileDrops_DefaultIsTrue)
{
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::DO_TILE_DROPS)) << "DO_TILE_DROPS should default to true";
}

// ============================================================================
// 掉落规则之间的独立性测试
// ============================================================================

/**
 * @brief 测试 DO_ENTITY_DROPS 独立于 DO_MOB_LOOT
 *
 * 设置其中一个不应影响另一个
 */
TEST_F(EntityDropGameRuleTest, EntityDrops_IndependentFromMobLoot)
{
    rules.setBoolean(GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::DO_MOB_LOOT))
        << "Setting DO_ENTITY_DROPS=false should not affect DO_MOB_LOOT";

    rules.setBoolean(GameRuleKeys::DO_ENTITY_DROPS, true, nullptr);
    rules.setBoolean(GameRuleKeys::DO_MOB_LOOT, false, nullptr);
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::DO_ENTITY_DROPS))
        << "Setting DO_MOB_LOOT=false should not affect DO_ENTITY_DROPS";
}

/**
 * @brief 测试 DO_ENTITY_DROPS 独立于 DO_TILE_DROPS
 */
TEST_F(EntityDropGameRuleTest, EntityDrops_IndependentFromTileDrops)
{
    rules.setBoolean(GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::DO_TILE_DROPS))
        << "Setting DO_ENTITY_DROPS=false should not affect DO_TILE_DROPS";

    rules.setBoolean(GameRuleKeys::DO_ENTITY_DROPS, true, nullptr);
    rules.setBoolean(GameRuleKeys::DO_TILE_DROPS, false, nullptr);
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::DO_ENTITY_DROPS))
        << "Setting DO_TILE_DROPS=false should not affect DO_ENTITY_DROPS";
}

// ============================================================================
// NBT 序列化/反序列化测试
// ============================================================================

/**
 * @brief 测试 DO_ENTITY_DROPS 的 NBT 序列化和反序列化
 */
TEST_F(EntityDropGameRuleTest, DoEntityDrops_NbtRoundTrip)
{
    // 设置为 false
    rules.setBoolean(GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::DO_ENTITY_DROPS));

    // 序列化
    auto nbt = rules.write();
    ASSERT_NE(nbt, nullptr);

    // 反序列化到新对象
    GameRules loadedRules(*nbt);
    EXPECT_FALSE(loadedRules.getBoolean(GameRuleKeys::DO_ENTITY_DROPS))
        << "DO_ENTITY_DROPS should persist as false after NBT round-trip";

    // 设置为 true 并测试
    rules.setBoolean(GameRuleKeys::DO_ENTITY_DROPS, true, nullptr);
    auto nbt2 = rules.write();
    GameRules loadedRules2(*nbt2);
    EXPECT_TRUE(loadedRules2.getBoolean(GameRuleKeys::DO_ENTITY_DROPS))
        << "DO_ENTITY_DROPS should persist as true after NBT round-trip";
}

// ============================================================================
// reset 测试
// ============================================================================

/**
 * @brief 测试 DO_ENTITY_DROPS 重置为默认值
 */
TEST_F(EntityDropGameRuleTest, DoEntityDrops_ResetToDefault)
{
    // 设置为 false
    rules.setBoolean(GameRuleKeys::DO_ENTITY_DROPS, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(GameRuleKeys::DO_ENTITY_DROPS));

    // 重置
    rules.reset("doEntityDrops", nullptr);
    EXPECT_TRUE(rules.getBoolean(GameRuleKeys::DO_ENTITY_DROPS))
        << "DO_ENTITY_DROPS should be reset to default value (true)";
}

// ============================================================================
// hasRule 和 getRuleType 测试
// ============================================================================

/**
 * @brief 测试 hasRule 识别 doEntityDrops
 */
TEST_F(EntityDropGameRuleTest, HasRule_RecognizesDoEntityDrops)
{
    EXPECT_TRUE(GameRules::hasRule("doEntityDrops")) << "GameRules::hasRule should recognize 'doEntityDrops'";

    EXPECT_TRUE(GameRules::hasRule("doMobLoot")) << "GameRules::hasRule should recognize 'doMobLoot'";

    EXPECT_TRUE(GameRules::hasRule("doTileDrops")) << "GameRules::hasRule should recognize 'doTileDrops'";
}

/**
 * @brief 测试 getRuleType 返回 Boolean 类型
 */
TEST_F(EntityDropGameRuleTest, GetRuleType_ReturnsBoolean)
{
    auto type = GameRules::getRuleType("doEntityDrops");
    ASSERT_TRUE(type.has_value());
    EXPECT_EQ(type.value(), GameRuleValueType::Boolean);

    auto mobLootType = GameRules::getRuleType("doMobLoot");
    ASSERT_TRUE(mobLootType.has_value());
    EXPECT_EQ(mobLootType.value(), GameRuleValueType::Boolean);

    auto tileDropsType = GameRules::getRuleType("doTileDrops");
    ASSERT_TRUE(tileDropsType.has_value());
    EXPECT_EQ(tileDropsType.value(), GameRuleValueType::Boolean);
}

// ============================================================================
// 实体掉落逻辑集成测试
//
// 完整的 dropItem() 集成测试（使用 Mock World 验证 spawnEntity 调用）
// 请参见 EntityDropItemGameRuleTest.cpp。
//
// 本文件仅测试 GameRules 层面的 DO_ENTITY_DROPS 行为验证。
// ============================================================================
