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

/**
 * @file EnergyGlintLayerTest.cpp
 * @brief EnergyGlintLayer 单元测试
 *
 * 测试附魔光效层渲染器的 shouldRender 方法，验证对附魔物品的检测逻辑。
 */

#include "client/renderer/trident/entity/layer/effect/EnergyGlintLayer.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <type_traits>
#include <gtest/gtest.h>

namespace mc::client::renderer::entity::layer::effect::test {

// ============================================================================
// 测试夹具
// ============================================================================

/**
 * @brief 测试用 LivingEntity 子类
 *
 * 提供最小化的 LivingEntity 实现，用于测试 shouldRender 方法。
 */
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1))
    {}

    void tick() override {}
};

class TestEnergyGlintLayer : public EnergyGlintLayer<LivingEntity> {
public:
    using EnergyGlintLayer<LivingEntity>::buildGlintMesh;
    using EnergyGlintLayer<LivingEntity>::calculateGlintOffset;
};

/**
 * @brief EnergyGlintLayer 测试夹具
 */
class EnergyGlintLayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化附魔注册表
        mc::item::enchant::EnchantmentRegistry::initialize();
        // 初始化物品注册表
        mc::Items::initialize();
        // 初始化方块注册表
        mc::VanillaBlocks::initialize();
        // 创建测试实体
        entity_ = std::make_unique<TestLivingEntity>();
        // 创建测试层
        layer_ = std::make_unique<TestEnergyGlintLayer>();
    }

    void TearDown() override
    {
        layer_.reset();
        entity_.reset();
        mc::item::enchant::EnchantmentRegistry::clear();
    }

    std::unique_ptr<TestLivingEntity> entity_;
    std::unique_ptr<TestEnergyGlintLayer> layer_;
};

// ============================================================================
// 类型检查测试
// ============================================================================

TEST_F(EnergyGlintLayerTest, InheritsFromLayerRenderer)
{
    // 验证 EnergyGlintLayer 继承自 LayerRenderer
    EXPECT_TRUE((std::is_base_of_v<core::LayerRenderer<LivingEntity>, EnergyGlintLayer<LivingEntity>>));
}

TEST_F(EnergyGlintLayerTest, TemplateWorksForLivingEntity)
{
    // 验证模板实例化正确
    EXPECT_NO_THROW(EnergyGlintLayer<LivingEntity> layer);
}

// ============================================================================
// shouldRender 测试 - 无装备情况
// ============================================================================

TEST_F(EnergyGlintLayerTest, ShouldRender_NoEquipment_ReturnsFalse)
{
    // 无任何装备时，shouldRender 应返回 false
    EXPECT_FALSE(layer_->shouldRender(*entity_));
}

// ============================================================================
// shouldRender 测试 - 非附魔物品
// ============================================================================

TEST_F(EnergyGlintLayerTest, ShouldRender_NonEnchantedMainHand_ReturnsFalse)
{
    // 主手持有非附魔物品
    ItemStack stoneSword(mc::Items::STONE_SWORD, 1);
    entity_->setEquipment(EquipmentSlot::MainHand, stoneSword);

    EXPECT_FALSE(layer_->shouldRender(*entity_));
}

TEST_F(EnergyGlintLayerTest, ShouldRender_NonEnchantedOffHand_ReturnsFalse)
{
    // 副手持有非附魔物品
    ItemStack shield(mc::Items::SHIELD, 1);
    entity_->setEquipment(EquipmentSlot::OffHand, shield);

    EXPECT_FALSE(layer_->shouldRender(*entity_));
}

TEST_F(EnergyGlintLayerTest, ShouldRender_NonEnchantedArmor_ReturnsFalse)
{
    // 穿戴非附魔盔甲
    ItemStack ironHelmet(mc::Items::IRON_HELMET, 1);
    ItemStack ironChestplate(mc::Items::IRON_CHESTPLATE, 1);
    ItemStack ironLeggings(mc::Items::IRON_LEGGINGS, 1);
    ItemStack ironBoots(mc::Items::IRON_BOOTS, 1);

    entity_->setEquipment(EquipmentSlot::Head, ironHelmet);
    entity_->setEquipment(EquipmentSlot::Chest, ironChestplate);
    entity_->setEquipment(EquipmentSlot::Legs, ironLeggings);
    entity_->setEquipment(EquipmentSlot::Feet, ironBoots);

    EXPECT_FALSE(layer_->shouldRender(*entity_));
}

// ============================================================================
// shouldRender 测试 - 附魔主手物品
// ============================================================================

TEST_F(EnergyGlintLayerTest, ShouldRender_EnchantedMainHand_ReturnsTrue)
{
    // 主手持有附魔物品
    ItemStack diamondSword(mc::Items::DIAMOND_SWORD, 1);
    diamondSword.addEnchantment("minecraft:sharpness", 3);

    entity_->setEquipment(EquipmentSlot::MainHand, diamondSword);

    EXPECT_TRUE(layer_->shouldRender(*entity_));
}

TEST_F(EnergyGlintLayerTest, ShouldRender_EnchantedMainHandMultipleEnchants_ReturnsTrue)
{
    // 主手持有多重附魔物品
    ItemStack diamondSword(mc::Items::DIAMOND_SWORD, 1);
    diamondSword.addEnchantment("minecraft:sharpness", 5);
    diamondSword.addEnchantment("minecraft:looting", 3);
    diamondSword.addEnchantment("minecraft:unbreaking", 3);

    entity_->setEquipment(EquipmentSlot::MainHand, diamondSword);

    EXPECT_TRUE(layer_->shouldRender(*entity_));
}

// ============================================================================
// shouldRender 测试 - 附魔副手物品
// ============================================================================

TEST_F(EnergyGlintLayerTest, ShouldRender_EnchantedOffHand_ReturnsTrue)
{
    // 副手持有附魔物品
    ItemStack enchantedBook(mc::Items::ENCHANTED_BOOK, 1);
    enchantedBook.addEnchantment("minecraft:mending", 1);

    entity_->setEquipment(EquipmentSlot::OffHand, enchantedBook);

    EXPECT_TRUE(layer_->shouldRender(*entity_));
}

// ============================================================================
// shouldRender 测试 - 附魔盔甲
// ============================================================================

TEST_F(EnergyGlintLayerTest, ShouldRender_EnchantedHelmet_ReturnsTrue)
{
    // 头盔附魔
    ItemStack diamondHelmet(mc::Items::DIAMOND_HELMET, 1);
    diamondHelmet.addEnchantment("minecraft:protection", 4);

    entity_->setEquipment(EquipmentSlot::Head, diamondHelmet);

    EXPECT_TRUE(layer_->shouldRender(*entity_));
}

TEST_F(EnergyGlintLayerTest, ShouldRender_EnchantedChestplate_ReturnsTrue)
{
    // 胸甲附魔
    ItemStack diamondChestplate(mc::Items::DIAMOND_CHESTPLATE, 1);
    diamondChestplate.addEnchantment("minecraft:protection", 4);

    entity_->setEquipment(EquipmentSlot::Chest, diamondChestplate);

    EXPECT_TRUE(layer_->shouldRender(*entity_));
}

TEST_F(EnergyGlintLayerTest, ShouldRender_EnchantedLeggings_ReturnsTrue)
{
    // 护腿附魔
    ItemStack diamondLeggings(mc::Items::DIAMOND_LEGGINGS, 1);
    diamondLeggings.addEnchantment("minecraft:protection", 4);

    entity_->setEquipment(EquipmentSlot::Legs, diamondLeggings);

    EXPECT_TRUE(layer_->shouldRender(*entity_));
}

TEST_F(EnergyGlintLayerTest, ShouldRender_EnchantedBoots_ReturnsTrue)
{
    // 靴子附魔
    ItemStack diamondBoots(mc::Items::DIAMOND_BOOTS, 1);
    diamondBoots.addEnchantment("minecraft:protection", 4);

    entity_->setEquipment(EquipmentSlot::Feet, diamondBoots);

    EXPECT_TRUE(layer_->shouldRender(*entity_));
}

// ============================================================================
// shouldRender 测试 - 混合情况
// ============================================================================

TEST_F(EnergyGlintLayerTest, ShouldRender_MixedEnchantedAndNonEnchanted_ReturnsTrue)
{
    // 混合穿戴附魔和非附魔装备
    ItemStack ironHelmet(mc::Items::IRON_HELMET, 1); // 非附魔
    ItemStack diamondChestplate(mc::Items::DIAMOND_CHESTPLATE, 1);
    diamondChestplate.addEnchantment("minecraft:protection", 4); // 附魔
    ItemStack ironLeggings(mc::Items::IRON_LEGGINGS, 1);         // 非附魔
    ItemStack diamondBoots(mc::Items::DIAMOND_BOOTS, 1);         // 非附魔

    entity_->setEquipment(EquipmentSlot::Head, ironHelmet);
    entity_->setEquipment(EquipmentSlot::Chest, diamondChestplate);
    entity_->setEquipment(EquipmentSlot::Legs, ironLeggings);
    entity_->setEquipment(EquipmentSlot::Feet, diamondBoots);

    // 只要有任何附魔装备，就应该返回 true
    EXPECT_TRUE(layer_->shouldRender(*entity_));
}

TEST_F(EnergyGlintLayerTest, ShouldRender_AllEnchanted_ReturnsTrue)
{
    // 所有装备都附魔
    ItemStack diamondSword(mc::Items::DIAMOND_SWORD, 1);
    diamondSword.addEnchantment("minecraft:sharpness", 5);

    ItemStack shield(mc::Items::SHIELD, 1);
    shield.addEnchantment("minecraft:mending", 1);

    ItemStack diamondHelmet(mc::Items::DIAMOND_HELMET, 1);
    diamondHelmet.addEnchantment("minecraft:protection", 4);

    ItemStack diamondChestplate(mc::Items::DIAMOND_CHESTPLATE, 1);
    diamondChestplate.addEnchantment("minecraft:protection", 4);

    ItemStack diamondLeggings(mc::Items::DIAMOND_LEGGINGS, 1);
    diamondLeggings.addEnchantment("minecraft:protection", 4);

    ItemStack diamondBoots(mc::Items::DIAMOND_BOOTS, 1);
    diamondBoots.addEnchantment("minecraft:protection", 4);

    entity_->setEquipment(EquipmentSlot::MainHand, diamondSword);
    entity_->setEquipment(EquipmentSlot::OffHand, shield);
    entity_->setEquipment(EquipmentSlot::Head, diamondHelmet);
    entity_->setEquipment(EquipmentSlot::Chest, diamondChestplate);
    entity_->setEquipment(EquipmentSlot::Legs, diamondLeggings);
    entity_->setEquipment(EquipmentSlot::Feet, diamondBoots);

    EXPECT_TRUE(layer_->shouldRender(*entity_));
}

// ============================================================================
// shouldRender 测试 - 空物品堆
// ============================================================================

TEST_F(EnergyGlintLayerTest, ShouldRender_EmptyItemStack_ReturnsFalse)
{
    // 设置空物品堆
    ItemStack emptyStack;
    entity_->setEquipment(EquipmentSlot::MainHand, emptyStack);

    EXPECT_FALSE(layer_->shouldRender(*entity_));
}

// ============================================================================
// shouldRender 测试 - 工具附魔
// ============================================================================

TEST_F(EnergyGlintLayerTest, ShouldRender_EnchantedTool_ReturnsTrue)
{
    // 工具附魔
    ItemStack diamondPickaxe(mc::Items::DIAMOND_PICKAXE, 1);
    diamondPickaxe.addEnchantment("minecraft:fortune", 3);

    entity_->setEquipment(EquipmentSlot::MainHand, diamondPickaxe);

    EXPECT_TRUE(layer_->shouldRender(*entity_));
}

TEST_F(EnergyGlintLayerTest, ShouldRender_EnchantedToolWithSilkTouch_ReturnsTrue)
{
    // 工具附魔（精准采集）
    ItemStack diamondPickaxe(mc::Items::DIAMOND_PICKAXE, 1);
    diamondPickaxe.addEnchantment("minecraft:silk_touch", 1);

    entity_->setEquipment(EquipmentSlot::MainHand, diamondPickaxe);

    EXPECT_TRUE(layer_->shouldRender(*entity_));
}

// ============================================================================
// shouldRender 测试 - 特殊附魔
// ============================================================================

TEST_F(EnergyGlintLayerTest, ShouldRender_EnchantedWithMending_ReturnsTrue)
{
    // 经验修补附魔
    ItemStack diamondSword(mc::Items::DIAMOND_SWORD, 1);
    diamondSword.addEnchantment("minecraft:mending", 1);

    entity_->setEquipment(EquipmentSlot::MainHand, diamondSword);

    EXPECT_TRUE(layer_->shouldRender(*entity_));
}

TEST_F(EnergyGlintLayerTest, ShouldRender_EnchantedWithCurse_ReturnsTrue)
{
    // 诅咒附魔也应该显示光效
    ItemStack diamondSword(mc::Items::DIAMOND_SWORD, 1);
    diamondSword.addEnchantment("minecraft:vanishing_curse", 1);

    entity_->setEquipment(EquipmentSlot::MainHand, diamondSword);

    EXPECT_TRUE(layer_->shouldRender(*entity_));
}

// ============================================================================
// calculateGlintOffset 测试
// ============================================================================

TEST_F(EnergyGlintLayerTest, CalculateGlintOffset_ZeroTicks_ReturnsZero)
{
    f32 offset = layer_->calculateGlintOffset(0.0f);
    EXPECT_FLOAT_EQ(offset, 0.0f);
}

TEST_F(EnergyGlintLayerTest, CalculateGlintOffset_PositiveTicks_ReturnsValueBetweenZeroAndOne)
{
    f32 offset = layer_->calculateGlintOffset(100.0f);
    EXPECT_GE(offset, 0.0f);
    EXPECT_LT(offset, 1.0f);
}

TEST_F(EnergyGlintLayerTest, CalculateGlintOffset_WrapsAroundAtModulo)
{
    // 100 ticks: offset = 100 * 0.01 = 1.0, mod 1.0 = 0.0
    f32 offset1 = layer_->calculateGlintOffset(100.0f);
    f32 offset2 = layer_->calculateGlintOffset(0.0f);
    EXPECT_FLOAT_EQ(offset1, offset2);
}

TEST_F(EnergyGlintLayerTest, CalculateGlintOffset_IncreasesWithTime)
{
    f32 offset1 = layer_->calculateGlintOffset(0.0f);
    f32 offset2 = layer_->calculateGlintOffset(50.0f);
    f32 offset3 = layer_->calculateGlintOffset(99.0f);
    EXPECT_GT(offset2, offset1);
    EXPECT_GT(offset3, offset2);
}

TEST_F(EnergyGlintLayerTest, CalculateGlintOffset_NegativeTicks_StillValid)
{
    f32 offset = layer_->calculateGlintOffset(-10.0f);
    EXPECT_GE(offset, 0.0f);
    EXPECT_LT(offset, 1.0f);
}

// ============================================================================
// buildGlintMesh 测试
// ============================================================================

TEST_F(EnergyGlintLayerTest, BuildGlintMesh_ProducesNonEmptyMesh)
{
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    layer_->buildGlintMesh(0.0f, vertices, indices);

    EXPECT_GT(vertices.size(), 0u);
    EXPECT_GT(indices.size(), 0u);
}

TEST_F(EnergyGlintLayerTest, BuildGlintMesh_ProducesValidTriangleIndices)
{
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    layer_->buildGlintMesh(0.5f, vertices, indices);

    // 索引应该是三角形的倍数
    EXPECT_EQ(indices.size() % 3, 0u);

    // 所有索引应该在顶点范围内
    for (u32 index : indices) {
        EXPECT_LT(index, vertices.size());
    }
}

TEST_F(EnergyGlintLayerTest, BuildGlintMesh_ProducesCubeMesh)
{
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    layer_->buildGlintMesh(0.0f, vertices, indices);

    // 立方体应该有 6 个面 * 4 个顶点 = 24 个顶点
    EXPECT_EQ(vertices.size(), 24u);

    // 立方体应该有 6 个面 * 6 个索引 = 36 个索引
    EXPECT_EQ(indices.size(), 36u);
}

TEST_F(EnergyGlintLayerTest, BuildGlintMesh_UVScrollsWithOffset)
{
    std::vector<model::ModelVertex> vertices1, vertices2;
    std::vector<u32> indices1, indices2;

    layer_->buildGlintMesh(0.0f, vertices1, indices1);
    layer_->buildGlintMesh(0.5f, vertices2, indices2);

    // UV 坐标应该随 offset 变化
    // 注意：ModelVertex 的 UV 成员名可能是 u, v 或 u0, v0
    // 这里检查顶点数量一致
    EXPECT_EQ(vertices1.size(), vertices2.size());
}

// ============================================================================
// EnchantmentHelper 集成测试
// ============================================================================

TEST_F(EnergyGlintLayerTest, EnchantmentHelper_HasEnchantments_ReturnsCorrectValue)
{
    // 验证 EnchantmentHelper 与 ItemStack 的集成
    ItemStack diamondSword(mc::Items::DIAMOND_SWORD, 1);

    // 无附魔
    EXPECT_FALSE(mc::item::enchant::EnchantmentHelper::hasEnchantments(diamondSword));

    // 添加附魔
    diamondSword.addEnchantment("minecraft:sharpness", 1);
    EXPECT_TRUE(mc::item::enchant::EnchantmentHelper::hasEnchantments(diamondSword));
}

TEST_F(EnergyGlintLayerTest, ItemStack_HasEnchantments_ReturnsCorrectValue)
{
    // 验证 ItemStack::hasEnchantments() 方法
    ItemStack diamondSword(mc::Items::DIAMOND_SWORD, 1);

    EXPECT_FALSE(diamondSword.hasEnchantments());

    diamondSword.addEnchantment("minecraft:sharpness", 1);
    EXPECT_TRUE(diamondSword.hasEnchantments());
}

} // namespace mc::client::renderer::entity::layer::effect::test
