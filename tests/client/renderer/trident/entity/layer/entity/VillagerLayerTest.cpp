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
 * @file VillagerLayerTest.cpp
 * @brief VillagerLayer 核心逻辑单元测试
 *
 * 测试村民多层纹理渲染层的核心逻辑，包括：
 * - 纹理路径计算（类型层、职业层、等级徽章层）
 * - 渲染条件判断
 * - 纹理名称映射
 *
 * 注意：由于 VillagerLayer 是模板类且依赖 Vulkan 渲染管线，
 * 本测试专注于核心逻辑，不测试完整渲染流程。
 */

#include <type_traits>
#include <gtest/gtest.h>

#include "client/renderer/trident/entity/layer/entity/VillagerLayer.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/resource/ResourceLocation.hpp"

using namespace mc;
using namespace mc::client::renderer::entity::layer::entity;
using namespace mc::entity;

namespace mc::renderer::layer::test {

/**
 * @brief VillagerLayer 纹理路径测试
 *
 * 测试 VillagerLayer 提供的纹理路径计算功能
 */
class VillagerLayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化测试
    }

    void TearDown() override
    {
        // 清理测试
    }
};

// ============================================================================
// 类型名称映射测试
// ============================================================================

/**
 * @brief 测试 VillagerType 到纹理名称的映射
 */
TEST_F(VillagerLayerTest, TypeNameMapping)
{
    // 验证类型索引与名称数组对应
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_TYPE_NAMES[0], "desert");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_TYPE_NAMES[1], "jungle");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_TYPE_NAMES[2], "plains");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_TYPE_NAMES[3], "savanna");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_TYPE_NAMES[4], "snow");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_TYPE_NAMES[5], "swamp");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_TYPE_NAMES[6], "taiga");

    // 验证类型数量
    EXPECT_EQ(VillagerLayerDetail::VILLAGER_TYPE_COUNT, 7);
}

/**
 * @brief 测试 VillagerProfession 到纹理名称的映射
 */
TEST_F(VillagerLayerTest, ProfessionNameMapping)
{
    // 验证职业索引与名称数组对应
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[0], "none");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[1], "armorer");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[2], "butcher");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[3], "cartographer");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[4], "cleric");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[5], "farmer");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[6], "fisherman");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[7], "fletcher");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[8], "leatherworker");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[9], "librarian");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[10], "mason");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[11], "nitwit");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[12], "shepherd");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[13], "toolsmith");
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[14], "weaponsmith");

    // 验证职业数量
    EXPECT_EQ(VillagerLayerDetail::VILLAGER_PROFESSION_COUNT, 15);
}

/**
 * @brief 测试等级徽章名称映射
 */
TEST_F(VillagerLayerTest, LevelNameMapping)
{
    // 验证等级索引与名称数组对应
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_LEVEL_NAMES[0], "stone");   // 等级1 - 新手
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_LEVEL_NAMES[1], "iron");    // 等级2 - 学徒
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_LEVEL_NAMES[2], "gold");    // 等级3 - 老手
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_LEVEL_NAMES[3], "emerald"); // 等级4 - 专家
    EXPECT_STREQ(VillagerLayerDetail::VILLAGER_LEVEL_NAMES[4], "diamond"); // 等级5 - 大师

    // 验证等级数量
    EXPECT_EQ(VillagerLayerDetail::VILLAGER_LEVEL_COUNT, 5);
    EXPECT_EQ(VillagerLayerDetail::VILLAGER_MIN_LEVEL, 1);
    EXPECT_EQ(VillagerLayerDetail::VILLAGER_MAX_LEVEL, 5);
}

// ============================================================================
// 静态方法测试
// ============================================================================

/**
 * @brief 测试 getProfessionName 静态方法
 */
TEST_F(VillagerLayerTest, GetProfessionName)
{
    using TestLayer = VillagerLayer<VillagerEntity, mc::client::renderer::entity::model::animal::VillagerModel>;

    EXPECT_STREQ(TestLayer::getProfessionName(VillagerProfession::None), "none");
    EXPECT_STREQ(TestLayer::getProfessionName(VillagerProfession::Farmer), "farmer");
    EXPECT_STREQ(TestLayer::getProfessionName(VillagerProfession::Librarian), "librarian");
    EXPECT_STREQ(TestLayer::getProfessionName(VillagerProfession::Nitwit), "nitwit");
    EXPECT_STREQ(TestLayer::getProfessionName(VillagerProfession::Weaponsmith), "weaponsmith");
}

/**
 * @brief 测试 getTypeName 静态方法
 */
TEST_F(VillagerLayerTest, GetTypeName)
{
    using TestLayer = VillagerLayer<VillagerEntity, mc::client::renderer::entity::model::animal::VillagerModel>;

    EXPECT_STREQ(TestLayer::getTypeName(VillagerType::Desert), "desert");
    EXPECT_STREQ(TestLayer::getTypeName(VillagerType::Plains), "plains");
    EXPECT_STREQ(TestLayer::getTypeName(VillagerType::Snow), "snow");
    EXPECT_STREQ(TestLayer::getTypeName(VillagerType::Taiga), "taiga");
}

/**
 * @brief 测试 getLevelName 静态方法
 */
TEST_F(VillagerLayerTest, GetLevelName)
{
    using TestLayer = VillagerLayer<VillagerEntity, mc::client::renderer::entity::model::animal::VillagerModel>;

    EXPECT_STREQ(TestLayer::getLevelName(1), "stone");
    EXPECT_STREQ(TestLayer::getLevelName(2), "iron");
    EXPECT_STREQ(TestLayer::getLevelName(3), "gold");
    EXPECT_STREQ(TestLayer::getLevelName(4), "emerald");
    EXPECT_STREQ(TestLayer::getLevelName(5), "diamond");

    // 测试边界值处理
    EXPECT_STREQ(TestLayer::getLevelName(0), "stone");    // 低于最小值，clamp到1
    EXPECT_STREQ(TestLayer::getLevelName(10), "diamond"); // 高于最大值，clamp到5
}

// ============================================================================
// 类型检查测试
// ============================================================================

/**
 * @brief 测试 VillagerEntity 继承关系
 */
TEST_F(VillagerLayerTest, VillagerEntityInheritance)
{
    // VillagerEntity 继承自 AbstractVillagerEntity
    EXPECT_TRUE((std::is_base_of_v<AbstractVillagerEntity, VillagerEntity>));

    // AbstractVillagerEntity 继承自 AgeableEntity
    EXPECT_TRUE((std::is_base_of_v<AgeableEntity, AbstractVillagerEntity>));

    // AgeableEntity 继承自 CreatureEntity
    EXPECT_TRUE((std::is_base_of_v<CreatureEntity, AgeableEntity>));

    // CreatureEntity 继承自 MobEntity
    EXPECT_TRUE((std::is_base_of_v<MobEntity, CreatureEntity>));

    // MobEntity 继承自 LivingEntity
    EXPECT_TRUE((std::is_base_of_v<LivingEntity, MobEntity>));

    // LivingEntity 继承自 Entity
    EXPECT_TRUE((std::is_base_of_v<Entity, LivingEntity>));
}

/**
 * @brief 测试 VillagerData 默认值
 */
TEST_F(VillagerLayerTest, VillagerDataDefaults)
{
    VillagerData data;

    // 默认值
    EXPECT_EQ(data.type(), VillagerType::Plains);
    EXPECT_EQ(data.profession(), VillagerProfession::None);
    EXPECT_EQ(data.level(), 1);
    EXPECT_EQ(data.experience(), 0);
}

/**
 * @brief 测试 VillagerData 设置方法
 */
TEST_F(VillagerLayerTest, VillagerDataSetters)
{
    VillagerData data;

    data.setType(VillagerType::Desert);
    EXPECT_EQ(data.type(), VillagerType::Desert);

    data.setProfession(VillagerProfession::Farmer);
    EXPECT_EQ(data.profession(), VillagerProfession::Farmer);

    data.setLevel(3);
    EXPECT_EQ(data.level(), 3);

    data.addExperience(100);
    EXPECT_GE(data.experience(), 100);
}

// ============================================================================
// 纹理路径测试
// ============================================================================

/**
 * @brief 测试 ResourceLocation 生成
 */
TEST_F(VillagerLayerTest, ResourceLocationFormat)
{
    ResourceLocation typeTexture("minecraft", "textures/entity/villager/type/desert.png");
    EXPECT_EQ(typeTexture.namespace_(), "minecraft");
    EXPECT_EQ(typeTexture.path(), "textures/entity/villager/type/desert.png");
    EXPECT_EQ(typeTexture.toString(), "minecraft:textures/entity/villager/type/desert.png");

    ResourceLocation professionTexture("minecraft", "textures/entity/villager/profession/farmer.png");
    EXPECT_EQ(professionTexture.toString(), "minecraft:textures/entity/villager/profession/farmer.png");

    ResourceLocation levelTexture("minecraft", "textures/entity/villager/profession_level/diamond.png");
    EXPECT_EQ(levelTexture.toString(), "minecraft:textures/entity/villager/profession_level/diamond.png");
}

} // namespace mc::renderer::layer::test
