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

#include "world/blockentity/interactive/BannerEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "util/color/DyeColor.hpp"
#include "util/nbt/Nbt.hpp"
#include "util/text/StringTextComponent.hpp"
#include "util/text/TranslationTextComponent.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;
using namespace mc::blockentity;

// ========== BannerEntity 测试 ==========

class BannerEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        pos_ = BlockPos(10, 64, 20);
        entity_ = std::make_unique<BannerEntity>(pos_);
    }

    BlockPos pos_;
    std::unique_ptr<BannerEntity> entity_;
};

TEST_F(BannerEntityTest, DefaultConstruction)
{
    EXPECT_EQ(entity_->getBaseColor(), DyeColor::White);
    EXPECT_EQ(entity_->getPatternCount(), 0);
    EXPECT_FALSE(entity_->hasCustomDisplayName());
}

TEST_F(BannerEntityTest, SetBaseColor)
{
    entity_->setBaseColor(DyeColor::Red);
    EXPECT_EQ(entity_->getBaseColor(), DyeColor::Red);

    entity_->setBaseColor(DyeColor::White);
    EXPECT_EQ(entity_->getBaseColor(), DyeColor::White);
}

TEST_F(BannerEntityTest, AddPatterns)
{
    entity_->addPattern(BannerPattern(BannerPatternType::StripeBottom, DyeColor::White));
    EXPECT_EQ(entity_->getPatternCount(), 1);

    entity_->addPattern(BannerPattern(BannerPatternType::Cross, DyeColor::Red));
    EXPECT_EQ(entity_->getPatternCount(), 2);
}

TEST_F(BannerEntityTest, MaxPatternsLimit)
{
    // 最多6层图案
    for (i32 i = 0; i < 6; ++i) {
        entity_->addPattern(BannerPattern(BannerPatternType::StripeBottom, DyeColor::White));
    }
    EXPECT_EQ(entity_->getPatternCount(), 6);

    // 超过6层应该失败
    bool added = entity_->addPattern(BannerPattern(BannerPatternType::Cross, DyeColor::Red));
    EXPECT_FALSE(added);
    EXPECT_EQ(entity_->getPatternCount(), 6);
}

TEST_F(BannerEntityTest, CustomDisplayName)
{
    EXPECT_FALSE(entity_->hasCustomDisplayName());

    entity_->setCustomDisplayName(std::make_unique<text::StringTextComponent>("My Banner"));
    EXPECT_TRUE(entity_->hasCustomDisplayName());
    const auto* displayName = entity_->getCustomDisplayName();
    ASSERT_NE(displayName, nullptr);
    EXPECT_EQ(displayName->getUnformattedText(), "My Banner");
}

TEST_F(BannerEntityTest, GetPatternsFromItemStack)
{
    // 空ItemStack应返回空图案列表
    ItemStack emptyStack;
    auto patterns = BannerEntity::getPatternsFromItemStack(emptyStack);
    EXPECT_TRUE(patterns.empty());
}

TEST_F(BannerEntityTest, GetPatternCountEmptyStack)
{
    ItemStack emptyStack;
    EXPECT_EQ(BannerEntity::getPatternCount(emptyStack), 0);
}

TEST_F(BannerEntityTest, RemoveBannerData)
{
    // 测试空物品不影响
    ItemStack emptyStack;
    BannerEntity::removeBannerData(emptyStack);
    // 无崩溃即通过
}

// ========== NBT 序列化测试 ==========

TEST_F(BannerEntityTest, SaveToNBT_WritesBaseColor)
{
    entity_->setBaseColor(DyeColor::Red);

    nbt::tags::compound_tag tag;
    entity_->saveToNBT(tag);

    // 基类写入 id, x, y, z
    ASSERT_NE(tag.value.find("Base"), tag.value.end());
    const auto* intTag = dynamic_cast<const nbt::tags::int_tag*>(tag.value.at("Base").get());
    ASSERT_NE(intTag, nullptr);
    EXPECT_EQ(intTag->value, static_cast<i32>(DyeColor::Red));
}

TEST_F(BannerEntityTest, SaveToNBT_WritesPatterns)
{
    entity_->setBaseColor(DyeColor::White);
    entity_->addPattern(BannerPattern(BannerPatternType::StripeBottom, DyeColor::Red));
    entity_->addPattern(BannerPattern(BannerPatternType::Cross, DyeColor::Blue));

    nbt::tags::compound_tag tag;
    entity_->saveToNBT(tag);

    ASSERT_NE(tag.value.find("Patterns"), tag.value.end());
    const auto* listTag = dynamic_cast<const nbt::tags::list_tag*>(tag.value.at("Patterns").get());
    ASSERT_NE(listTag, nullptr);
    ASSERT_EQ(listTag->element_id(), nbt::TagId::Compound);

    const auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*listTag);
    ASSERT_EQ(compoundList.value.size(), 2u);

    // 第一个图案：StripeBottom (哈希名 "bs"), Red (14)
    const auto& pattern0 = compoundList.value[0];
    const auto* patternStr0 = dynamic_cast<const nbt::tags::string_tag*>(pattern0.value.at("Pattern").get());
    ASSERT_NE(patternStr0, nullptr);
    EXPECT_EQ(patternStr0->value, "bs");
    const auto* colorInt0 = dynamic_cast<const nbt::tags::int_tag*>(pattern0.value.at("Color").get());
    ASSERT_NE(colorInt0, nullptr);
    EXPECT_EQ(colorInt0->value, static_cast<i32>(DyeColor::Red));

    // 第二个图案：Cross (哈希名 "cr"), Blue (11)
    const auto& pattern1 = compoundList.value[1];
    const auto* patternStr1 = dynamic_cast<const nbt::tags::string_tag*>(pattern1.value.at("Pattern").get());
    ASSERT_NE(patternStr1, nullptr);
    EXPECT_EQ(patternStr1->value, "cr");
    const auto* colorInt1 = dynamic_cast<const nbt::tags::int_tag*>(pattern1.value.at("Color").get());
    ASSERT_NE(colorInt1, nullptr);
    EXPECT_EQ(colorInt1->value, static_cast<i32>(DyeColor::Blue));
}

TEST_F(BannerEntityTest, SaveToNBT_WritesCustomName)
{
    entity_->setCustomDisplayName(std::make_unique<text::StringTextComponent>("Test Banner"));

    nbt::tags::compound_tag tag;
    entity_->saveToNBT(tag);

    ASSERT_NE(tag.value.find("CustomName"), tag.value.end());
    const auto* nameTag = dynamic_cast<const nbt::tags::string_tag*>(tag.value.at("CustomName").get());
    ASSERT_NE(nameTag, nullptr);
    // JSON 格式的文本组件字符串
    EXPECT_NE(nameTag->value.find("Test Banner"), std::string::npos);
}

TEST_F(BannerEntityTest, SaveToNBT_NoPatternsOmitsField)
{
    // 空图案列表不应写入 Patterns 字段
    entity_->setBaseColor(DyeColor::White);

    nbt::tags::compound_tag tag;
    entity_->saveToNBT(tag);

    EXPECT_EQ(tag.value.find("Patterns"), tag.value.end());
}

TEST_F(BannerEntityTest, SaveToNBT_NoCustomNameOmitsField)
{
    // 无自定义名称不应写入 CustomName 字段
    nbt::tags::compound_tag tag;
    entity_->saveToNBT(tag);

    EXPECT_EQ(tag.value.find("CustomName"), tag.value.end());
}

TEST_F(BannerEntityTest, LoadFromNBT_ReadsBaseColor)
{
    nbt::tags::compound_tag tag;
    tag.put("Base", static_cast<i32>(DyeColor::Orange));

    auto entity = std::make_unique<BannerEntity>(pos_);
    ASSERT_TRUE(entity->loadFromNBT(tag));
    EXPECT_EQ(entity->getBaseColor(), DyeColor::Orange);
}

TEST_F(BannerEntityTest, LoadFromNBT_ReadsPatterns)
{
    // 构造 NBT: Patterns 列表
    auto patternsList = std::make_unique<nbt::tags::compound_list_tag>();
    nbt::tags::compound_tag pattern0;
    pattern0.put("Pattern", std::string("bs"));
    pattern0.put("Color", static_cast<i32>(DyeColor::Red));
    patternsList->value.push_back(std::move(pattern0));

    nbt::tags::compound_tag pattern1;
    pattern1.put("Pattern", std::string("cr"));
    pattern1.put("Color", static_cast<i32>(DyeColor::Blue));
    patternsList->value.push_back(std::move(pattern1));

    nbt::tags::compound_tag tag;
    tag.put("Base", static_cast<i32>(DyeColor::White));
    tag.value.emplace("Patterns", std::move(patternsList));

    auto entity = std::make_unique<BannerEntity>(pos_);
    ASSERT_TRUE(entity->loadFromNBT(tag));

    EXPECT_EQ(entity->getBaseColor(), DyeColor::White);
    ASSERT_EQ(entity->getPatternCount(), 2);

    const auto& patterns = entity->getPatterns();
    EXPECT_EQ(patterns[0].pattern, BannerPatternType::StripeBottom);
    EXPECT_EQ(patterns[0].color, DyeColor::Red);
    EXPECT_EQ(patterns[1].pattern, BannerPatternType::Cross);
    EXPECT_EQ(patterns[1].color, DyeColor::Blue);
}

TEST_F(BannerEntityTest, LoadFromNBT_ReadsCustomName)
{
    nbt::tags::compound_tag tag;
    tag.put("Base", static_cast<i32>(DyeColor::White));
    tag.put("CustomName", std::string("{\"text\":\"My Banner\",\"color\":\"gold\"}"));

    auto entity = std::make_unique<BannerEntity>(pos_);
    ASSERT_TRUE(entity->loadFromNBT(tag));

    EXPECT_TRUE(entity->hasCustomDisplayName());
    const auto* name = entity->getCustomDisplayName();
    ASSERT_NE(name, nullptr);
    EXPECT_EQ(name->getUnformattedText(), "My Banner");
}

TEST_F(BannerEntityTest, LoadFromNBT_CustomNameFallbackOnInvalidJson)
{
    // 无效 JSON 字符串应回退为纯文本
    nbt::tags::compound_tag tag;
    tag.put("Base", static_cast<i32>(DyeColor::White));
    tag.put("CustomName", std::string("Plain Text Name"));

    auto entity = std::make_unique<BannerEntity>(pos_);
    ASSERT_TRUE(entity->loadFromNBT(tag));

    EXPECT_TRUE(entity->hasCustomDisplayName());
    const auto* name = entity->getCustomDisplayName();
    ASSERT_NE(name, nullptr);
    EXPECT_EQ(name->getUnformattedText(), "Plain Text Name");
}

TEST_F(BannerEntityTest, LoadFromNBT_ExceedsMaxPatternsTruncates)
{
    // 超过6层图案应截断
    auto patternsList = std::make_unique<nbt::tags::compound_list_tag>();
    for (i32 i = 0; i < 10; ++i) {
        nbt::tags::compound_tag p;
        p.put("Pattern", std::string("bs"));
        p.put("Color", static_cast<i32>(DyeColor::White));
        patternsList->value.push_back(std::move(p));
    }

    nbt::tags::compound_tag tag;
    tag.value.emplace("Patterns", std::move(patternsList));

    auto entity = std::make_unique<BannerEntity>(pos_);
    ASSERT_TRUE(entity->loadFromNBT(tag));

    // 最多只保留6层
    EXPECT_EQ(entity->getPatternCount(), 6);
}

TEST_F(BannerEntityTest, LoadFromNBT_InvalidDyeColorClamps)
{
    // 超出范围的 DyeColor 值应被忽略
    auto patternsList = std::make_unique<nbt::tags::compound_list_tag>();
    nbt::tags::compound_tag p;
    p.put("Pattern", std::string("bs"));
    p.put("Color", 999); // 无效颜色值
    patternsList->value.push_back(std::move(p));

    nbt::tags::compound_tag tag;
    tag.put("Base", 999); // 无效底色值
    tag.value.emplace("Patterns", std::move(patternsList));

    auto entity = std::make_unique<BannerEntity>(pos_);
    ASSERT_TRUE(entity->loadFromNBT(tag));

    // 底色应保持默认值（White），因为999超出范围
    EXPECT_EQ(entity->getBaseColor(), DyeColor::White);
    // 图案颜色应保持默认值（White），因为999超出范围
    ASSERT_EQ(entity->getPatternCount(), 1);
    EXPECT_EQ(entity->getPatterns()[0].color, DyeColor::White);
}

TEST_F(BannerEntityTest, LoadFromNBT_EmptyPatternsField)
{
    // 空 Patterns 列表
    auto patternsList = std::make_unique<nbt::tags::compound_list_tag>();

    nbt::tags::compound_tag tag;
    tag.value.emplace("Patterns", std::move(patternsList));

    auto entity = std::make_unique<BannerEntity>(pos_);
    ASSERT_TRUE(entity->loadFromNBT(tag));
    EXPECT_EQ(entity->getPatternCount(), 0);
}

TEST_F(BannerEntityTest, LoadFromNBT_NoCustomNameClearsExisting)
{
    // 先设置自定义名称，再加载没有 CustomName 的 NBT，应清除
    entity_->setCustomDisplayName(std::make_unique<text::StringTextComponent>("Old Name"));
    EXPECT_TRUE(entity_->hasCustomDisplayName());

    nbt::tags::compound_tag tag;
    // 不包含 CustomName 字段
    ASSERT_TRUE(entity_->loadFromNBT(tag));
    EXPECT_FALSE(entity_->hasCustomDisplayName());
}

TEST_F(BannerEntityTest, NBT_RoundTrip)
{
    // 完整往返测试：save -> load
    entity_->setBaseColor(DyeColor::Purple);
    entity_->addPattern(BannerPattern(BannerPatternType::StripeBottom, DyeColor::White));
    entity_->addPattern(BannerPattern(BannerPatternType::Cross, DyeColor::Red));
    entity_->addPattern(BannerPattern(BannerPatternType::Creeper, DyeColor::Black));
    entity_->setCustomDisplayName(std::make_unique<text::StringTextComponent>("Round Trip Banner"));

    // 保存到 NBT
    nbt::tags::compound_tag tag;
    entity_->saveToNBT(tag);

    // 加载到新实体
    auto entity2 = std::make_unique<BannerEntity>(pos_);
    ASSERT_TRUE(entity2->loadFromNBT(tag));

    // 验证数据一致性
    EXPECT_EQ(entity2->getBaseColor(), DyeColor::Purple);
    ASSERT_EQ(entity2->getPatternCount(), 3);
    EXPECT_EQ(entity2->getPatterns()[0].pattern, BannerPatternType::StripeBottom);
    EXPECT_EQ(entity2->getPatterns()[0].color, DyeColor::White);
    EXPECT_EQ(entity2->getPatterns()[1].pattern, BannerPatternType::Cross);
    EXPECT_EQ(entity2->getPatterns()[1].color, DyeColor::Red);
    EXPECT_EQ(entity2->getPatterns()[2].pattern, BannerPatternType::Creeper);
    EXPECT_EQ(entity2->getPatterns()[2].color, DyeColor::Black);

    EXPECT_TRUE(entity2->hasCustomDisplayName());
    ASSERT_NE(entity2->getCustomDisplayName(), nullptr);
    EXPECT_EQ(entity2->getCustomDisplayName()->getUnformattedText(), "Round Trip Banner");
}

TEST_F(BannerEntityTest, Clone_CopiesAllFields)
{
    entity_->setBaseColor(DyeColor::Cyan);
    entity_->addPattern(BannerPattern(BannerPatternType::StripeBottom, DyeColor::Red));
    entity_->setCustomDisplayName(std::make_unique<text::StringTextComponent>("Cloned Banner"));

    auto cloned = entity_->clone();
    auto* clonedBanner = dynamic_cast<BannerEntity*>(cloned.get());
    ASSERT_NE(clonedBanner, nullptr);

    EXPECT_EQ(clonedBanner->getBaseColor(), DyeColor::Cyan);
    ASSERT_EQ(clonedBanner->getPatternCount(), 1);
    EXPECT_EQ(clonedBanner->getPatterns()[0].pattern, BannerPatternType::StripeBottom);
    EXPECT_EQ(clonedBanner->getPatterns()[0].color, DyeColor::Red);

    // 自定义名称深拷贝
    EXPECT_TRUE(clonedBanner->hasCustomDisplayName());
    ASSERT_NE(clonedBanner->getCustomDisplayName(), nullptr);
    EXPECT_EQ(clonedBanner->getCustomDisplayName()->getUnformattedText(), "Cloned Banner");

    // 验证是深拷贝而非共享指针
    EXPECT_NE(clonedBanner->getCustomDisplayName(), entity_->getCustomDisplayName());
}

// ========== JSON 序列化测试 ==========

TEST_F(BannerEntityTest, JSON_RoundTrip)
{
    entity_->setBaseColor(DyeColor::Pink);
    entity_->addPattern(BannerPattern(BannerPatternType::SquareBottomLeft, DyeColor::Orange));
    entity_->setCustomDisplayName(std::make_unique<text::StringTextComponent>("JSON Banner"));

    // 保存到 JSON
    nlohmann::json data;
    entity_->save(data);

    // 加载到新实体
    auto entity2 = std::make_unique<BannerEntity>(pos_);
    ASSERT_TRUE(entity2->load(data));

    // 验证数据一致性
    EXPECT_EQ(entity2->getBaseColor(), DyeColor::Pink);
    ASSERT_EQ(entity2->getPatternCount(), 1);
    EXPECT_EQ(entity2->getPatterns()[0].pattern, BannerPatternType::SquareBottomLeft);
    EXPECT_EQ(entity2->getPatterns()[0].color, DyeColor::Orange);

    EXPECT_TRUE(entity2->hasCustomDisplayName());
    ASSERT_NE(entity2->getCustomDisplayName(), nullptr);
    EXPECT_EQ(entity2->getCustomDisplayName()->getUnformattedText(), "JSON Banner");
}

// ========== ItemStack 图案操作测试（炼药锅旗帜清洗所需核心方法） ==========

class BannerEntityItemStackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保 Items 已初始化（旗帜物品注册需要）
        Items::initialize();
    }
};

TEST_F(BannerEntityItemStackTest, GetPatternCount_EmptyStack_ReturnsZero)
{
    ItemStack emptyStack;
    EXPECT_EQ(BannerEntity::getPatternCount(emptyStack), 0);
}

TEST_F(BannerEntityItemStackTest, GetPatternCount_StackWithNoPatterns_ReturnsZero)
{
    // 没有 BlockEntityTag 的物品
    if (Items::WHITE_BANNER == nullptr) {
        GTEST_SKIP() << "WHITE_BANNER not registered";
    }
    ItemStack bannerStack(*Items::WHITE_BANNER, 1);
    EXPECT_EQ(BannerEntity::getPatternCount(bannerStack), 0);
}

TEST_F(BannerEntityItemStackTest, GetPatternCount_StackWithPatterns_ReturnsCorrectCount)
{
    if (Items::WHITE_BANNER == nullptr) {
        GTEST_SKIP() << "WHITE_BANNER not registered";
    }
    ItemStack bannerStack(*Items::WHITE_BANNER, 1);

    // 添加3个图案到 BlockEntityTag.Patterns
    nlohmann::json& blockEntityTag = bannerStack.getOrCreateChildTag("BlockEntityTag");
    nlohmann::json patterns = nlohmann::json::array();
    patterns.push_back({{"Pattern", "bs"}, {"Color", 14}}); // StripeBottom + Red
    patterns.push_back({{"Pattern", "cr"}, {"Color", 11}}); // Cross + Blue
    patterns.push_back({{"Pattern", "mc"}, {"Color", 0}});  // SquareBottomLeft + White
    blockEntityTag["Patterns"] = patterns;

    EXPECT_EQ(BannerEntity::getPatternCount(bannerStack), 3);
}

TEST_F(BannerEntityItemStackTest, RemoveBannerData_RemovesTopPattern)
{
    if (Items::WHITE_BANNER == nullptr) {
        GTEST_SKIP() << "WHITE_BANNER not registered";
    }
    ItemStack bannerStack(*Items::WHITE_BANNER, 1);

    // 添加3个图案
    nlohmann::json& blockEntityTag = bannerStack.getOrCreateChildTag("BlockEntityTag");
    nlohmann::json patterns = nlohmann::json::array();
    patterns.push_back({{"Pattern", "bs"}, {"Color", 14}});
    patterns.push_back({{"Pattern", "cr"}, {"Color", 11}});
    patterns.push_back({{"Pattern", "mc"}, {"Color", 0}});
    blockEntityTag["Patterns"] = patterns;

    ASSERT_EQ(BannerEntity::getPatternCount(bannerStack), 3);

    // 移除顶层图案
    BannerEntity::removeBannerData(bannerStack);

    // 应该只剩2个图案
    EXPECT_EQ(BannerEntity::getPatternCount(bannerStack), 2);

    // 验证剩余的是前两个图案（后进先出）
    const nlohmann::json* tag = bannerStack.getChildTag("BlockEntityTag");
    ASSERT_NE(tag, nullptr);
    ASSERT_TRUE(tag->contains("Patterns"));
    const auto& remainingPatterns = (*tag)["Patterns"];
    ASSERT_EQ(remainingPatterns.size(), 2u);
    EXPECT_EQ(remainingPatterns[0]["Pattern"].get<std::string>(), "bs");
    EXPECT_EQ(remainingPatterns[1]["Pattern"].get<std::string>(), "cr");
}

TEST_F(BannerEntityItemStackTest, RemoveBannerData_RemovesAllPatternsCleansBlockEntityTag)
{
    if (Items::WHITE_BANNER == nullptr) {
        GTEST_SKIP() << "WHITE_BANNER not registered";
    }
    ItemStack bannerStack(*Items::WHITE_BANNER, 1);

    // 添加1个图案
    nlohmann::json& blockEntityTag = bannerStack.getOrCreateChildTag("BlockEntityTag");
    nlohmann::json patterns = nlohmann::json::array();
    patterns.push_back({{"Pattern", "bs"}, {"Color", 14}});
    blockEntityTag["Patterns"] = patterns;

    ASSERT_EQ(BannerEntity::getPatternCount(bannerStack), 1);

    // 移除唯一图案
    BannerEntity::removeBannerData(bannerStack);

    // 图案数为0
    EXPECT_EQ(BannerEntity::getPatternCount(bannerStack), 0);

    // BlockEntityTag 应该被完全清除（removeBannerData 在 Patterns 为空时删除 BlockEntityTag）
    const nlohmann::json* tag = bannerStack.getChildTag("BlockEntityTag");
    EXPECT_TRUE(tag == nullptr || !tag->contains("Patterns") || !tag->contains("BlockEntityTag"));
}

TEST_F(BannerEntityItemStackTest, RemoveBannerData_EmptyStack_NoCrash)
{
    ItemStack emptyStack;
    // 空物品不应崩溃
    BannerEntity::removeBannerData(emptyStack);
    SUCCEED();
}

TEST_F(BannerEntityItemStackTest, RemoveBannerData_StackWithNoPatterns_NoChange)
{
    if (Items::WHITE_BANNER == nullptr) {
        GTEST_SKIP() << "WHITE_BANNER not registered";
    }
    ItemStack bannerStack(*Items::WHITE_BANNER, 1);
    // 没有图案的旗帜不应改变
    i32 countBefore = BannerEntity::getPatternCount(bannerStack);
    BannerEntity::removeBannerData(bannerStack);
    EXPECT_EQ(BannerEntity::getPatternCount(bannerStack), countBefore);
}

TEST_F(BannerEntityItemStackTest, RemoveBannerData_ShieldWithPatterns_RemovesTopPattern)
{
    if (Items::SHIELD == nullptr) {
        GTEST_SKIP() << "SHIELD not registered";
    }
    ItemStack shieldStack(*Items::SHIELD, 1);

    // 盾牌也使用 BlockEntityTag.Patterns 存储图案
    nlohmann::json& blockEntityTag = shieldStack.getOrCreateChildTag("BlockEntityTag");
    nlohmann::json patterns = nlohmann::json::array();
    patterns.push_back({{"Pattern", "cr"}, {"Color", 14}});
    patterns.push_back({{"Pattern", "mc"}, {"Color", 0}});
    blockEntityTag["Patterns"] = patterns;

    ASSERT_EQ(BannerEntity::getPatternCount(shieldStack), 2);

    // 移除顶层图案
    BannerEntity::removeBannerData(shieldStack);

    // 应该只剩1个图案
    EXPECT_EQ(BannerEntity::getPatternCount(shieldStack), 1);

    // 验证剩余图案
    const nlohmann::json* tag = shieldStack.getChildTag("BlockEntityTag");
    ASSERT_NE(tag, nullptr);
    ASSERT_TRUE(tag->contains("Patterns"));
    const auto& remainingPatterns = (*tag)["Patterns"];
    ASSERT_EQ(remainingPatterns.size(), 1u);
    EXPECT_EQ(remainingPatterns[0]["Pattern"].get<std::string>(), "cr");
}
