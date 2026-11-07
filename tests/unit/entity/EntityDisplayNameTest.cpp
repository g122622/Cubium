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

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/TranslationTextComponent.hpp"

using namespace mc;
using namespace mc::entity;

namespace {

/// EntityType::Builder 要求提供分类；这里只关心名字与翻译键，分类取 Misc。
constexpr EntityClassification TEST_CLASSIFICATION = EntityClassification::Misc;

/**
 * @brief 构造一个"像工厂产出那样"带 typeId 的猪
 *
 * PigEntity 的直接构造函数不设置 typeId（类型由 EntityType::create 工厂在真实路径上补齐），
 * 而默认显示名需要经 entityType() 取翻译键，故这里显式补上，与生产路径一致。
 */
std::unique_ptr<PigEntity> makeTypedPig(EntityInstanceId id)
{
    auto pig = std::make_unique<PigEntity>(id, mc::test::testEcsRegistry());
    pig->setTypeId(EntityTypeKeys::PIG);
    return pig;
}

} // namespace

/**
 * @brief 实体默认显示名与类型翻译键的测试
 *
 * 无自定义名的实体，其显示名必须是实体类型的可翻译名（`entity.minecraft.<path>`），
 * 而不是占位字符串——否则统计信息、死亡消息、命令回执等所有依赖显示名的地方都会出现
 * "entity" 这样的废文本。
 */
class EntityDisplayNameTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaEntities::registerAll();
    }
};

// ============================================================================
// EntityType::getDescriptionId
// ============================================================================

TEST_F(EntityDisplayNameTest, DescriptionIdUsesDottedNamespace)
{
    // 经注册表注册一个自定义命名空间的类型，再取其翻译键（bindIdentity 是私有的，
    // 外部只能经 registerType 建立带名字的类型）。
    auto& registry = EntityRegistry::instance();
    const auto registered = registry.registerType("mymod:custom_mob",
        EntityType::Builder(&PigEntity::create, TEST_CLASSIFICATION).size(0.6f, 1.8f).build());
    ASSERT_TRUE(registered.success());

    const EntityType* type = registry.getType("mymod:custom_mob");
    ASSERT_NE(type, nullptr);
    // 冒号必须换成点：`entity.mymod:custom_mob` 不是合法翻译键。
    EXPECT_EQ(type->getDescriptionId(), "entity.mymod.custom_mob");

    registry.unregisterType("mymod:custom_mob", type);
}

TEST_F(EntityDisplayNameTest, DescriptionIdForVanillaRegisteredTypes)
{
    const EntityType* pig = EntityRegistry::instance().getType(EntityTypeKeys::PIG);
    ASSERT_NE(pig, nullptr);
    EXPECT_EQ(pig->getDescriptionId(), "entity.minecraft.pig");

    const EntityType* zombie = EntityRegistry::instance().getType(EntityTypeKeys::ZOMBIE);
    ASSERT_NE(zombie, nullptr);
    EXPECT_EQ(zombie->getDescriptionId(), "entity.minecraft.zombie");
}

// ============================================================================
// Entity::getDisplayName 默认名
// ============================================================================

TEST_F(EntityDisplayNameTest, DefaultDisplayNameIsTranslatableTypeName)
{
    const auto pig = makeTypedPig(EntityInstanceId(1));

    const auto displayName = pig->getDisplayName();
    ASSERT_NE(displayName, nullptr);

    // 必须是翻译组件而非纯文本：语言在渲染/序列化时才决定。
    const auto* translation = dynamic_cast<const text::TranslationTextComponent*>(displayName.get());
    ASSERT_NE(translation, nullptr) << "default display name must be a translation component";
    EXPECT_EQ(translation->getKey(), "entity.minecraft.pig");
}

TEST_F(EntityDisplayNameTest, DefaultDisplayNameIsNotPlaceholder)
{
    const auto pig = makeTypedPig(EntityInstanceId(1));
    // 回归：曾经返回占位串 "entity"。
    EXPECT_NE(pig->getDisplayName()->getUnformattedText(), "entity");
}

TEST_F(EntityDisplayNameTest, CustomNameOverridesTypeName)
{
    const auto pig = makeTypedPig(EntityInstanceId(1));
    pig->setCustomName("Bacon");

    const auto displayName = pig->getDisplayName();
    ASSERT_NE(displayName, nullptr);
    EXPECT_EQ(displayName->getUnformattedText(), "Bacon");
    // 自定义名优先，不应再包一层翻译组件。
    EXPECT_EQ(dynamic_cast<const text::TranslationTextComponent*>(displayName.get()), nullptr);
}

// ============================================================================
// VanillaEntities::getLocalizedNameKey 与 EntityType::getDescriptionId 同源
// ============================================================================

TEST_F(EntityDisplayNameTest, LocalizedNameKeyMatchesDescriptionId)
{
    // 两条路径必须给出同一个键：此前 getLocalizedNameKey 手工拼接会产出带冒号的非法键。
    EXPECT_EQ(VanillaEntities::getLocalizedNameKey(EntityTypeKeys::PIG), "entity.minecraft.pig");
    EXPECT_EQ(VanillaEntities::getLocalizedNameKey(EntityTypeKeys::PIG),
        EntityRegistry::instance().getType(EntityTypeKeys::PIG)->getDescriptionId());
}

TEST_F(EntityDisplayNameTest, LocalizedNameKeyFallsBackForUnknownType)
{
    EXPECT_EQ(VanillaEntities::getLocalizedNameKey("minecraft:no_such_type"), "entity.minecraft.unknown");
}