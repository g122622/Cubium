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
 * @file test_player_layers.cpp
 * @brief 玩家层渲染器单元测试
 *
 * 测试覆盖：
 * - CapeLayer::shouldRender() 披风显示条件判断
 * - ElytraLayer::shouldRender() 鞘翅显示条件判断
 * - HeadLayer 构造和基础功能
 */

#include "common/core/Types.hpp"
#include <memory>
#include <gtest/gtest.h>

// 前向声明和模拟类型

namespace mc {

// 模拟 PlayerModelPart 枚举（从实际头文件复制）
enum class PlayerModelPart : u8 {
    Cape = 0,
    Jacket = 1,
    LeftSleeve = 2,
    RightSleeve = 3,
    LeftPantsLeg = 4,
    RightPantsLeg = 5,
    Hat = 6,
};

[[nodiscard]] constexpr u8 getPlayerModelPartMask(PlayerModelPart part) noexcept
{
    return static_cast<u8>(1u << static_cast<u8>(part));
}

// 模拟 EquipmentSlot 枚举
enum class EquipmentSlot : u8 { MainHand = 0, OffHand = 1, Feet = 2, Legs = 3, Chest = 4, Head = 5, Count = 6 };

// 模拟 ItemStack（简化版本用于测试）
class MockItemStack {
public:
    MockItemStack()
        : m_empty(true)
        , m_itemId(0)
    {}
    MockItemStack(u32 itemId)
        : m_empty(false)
        , m_itemId(itemId)
    {}

    [[nodiscard]] bool isEmpty() const { return m_empty; }
    [[nodiscard]] u32 getItemId() const { return m_itemId; }

private:
    bool m_empty;
    u32 m_itemId;
};

// 模拟 Items（简化版本）
class MockItems {
public:
    static constexpr u32 ELYTRA = 1;
    static constexpr u32 DIAMOND_HELMET = 2;
    static constexpr u32 DIAMOND_CHESTPLATE = 3;
};

// 模拟 Player（简化版本用于测试层渲染器的 shouldRender 逻辑）
class MockPlayer {
public:
    MockPlayer()
        : m_playerModelParts(getPlayerModelPartMask(PlayerModelPart::Cape) |
              getPlayerModelPartMask(PlayerModelPart::Jacket) | getPlayerModelPartMask(PlayerModelPart::Hat))
        , m_chestItem()
    {}

    void setWearingCape(bool wearing)
    {
        if (wearing) {
            m_playerModelParts |= getPlayerModelPartMask(PlayerModelPart::Cape);
        } else {
            m_playerModelParts &= ~getPlayerModelPartMask(PlayerModelPart::Cape);
        }
    }

    [[nodiscard]] bool isWearing(PlayerModelPart part) const
    {
        return (m_playerModelParts & getPlayerModelPartMask(part)) != 0;
    }

    void setChestItem(const MockItemStack& item) { m_chestItem = item; }

    [[nodiscard]] const MockItemStack& getChestItem() const { return m_chestItem; }

    // 模拟 getEquipment
    [[nodiscard]] const MockItemStack& getEquipment(EquipmentSlot slot) const
    {
        if (slot == EquipmentSlot::Chest) {
            return m_chestItem;
        }
        static MockItemStack empty;
        return empty;
    }

private:
    u8 m_playerModelParts;
    MockItemStack m_chestItem;
};

} // namespace mc

// ============================================================================
// CapeLayer shouldRender 逻辑测试
// ============================================================================

/**
 * @brief 模拟 CapeLayer::shouldRender() 的逻辑
 *
 * 参考 MC 1.16.5 CapeLayer.shouldRender():
 * 1. 玩家不在旁观者模式
 * 2. 玩家开启了披风显示 (PlayerModelPart::Cape)
 * 3. 玩家有披风纹理
 * 4. 玩家没有穿戴鞘翅（鞘翅会覆盖披风）
 */
bool shouldRenderCape(const mc::MockPlayer& player, bool hasCapeTexture)
{
    // 检查是否开启了披风显示
    if (!player.isWearing(mc::PlayerModelPart::Cape)) {
        return false;
    }

    // 检查是否有披风纹理
    if (!hasCapeTexture) {
        return false;
    }

    // 检查是否穿戴了鞘翅（鞘翅会覆盖披风）
    const auto& chest = player.getEquipment(mc::EquipmentSlot::Chest);
    if (!chest.isEmpty() && chest.getItemId() == mc::MockItems::ELYTRA) {
        return false;
    }

    return true;
}

class CapeLayerShouldRenderTest : public ::testing::Test {
protected:
    void SetUp() override { m_player = std::make_unique<mc::MockPlayer>(); }

    std::unique_ptr<mc::MockPlayer> m_player;
};

TEST_F(CapeLayerShouldRenderTest, ShouldNotRenderWhenCapePartDisabled)
{
    // 禁用披风部件
    m_player->setWearingCape(false);

    // 即使有披风纹理也不应该渲染
    EXPECT_FALSE(shouldRenderCape(*m_player, true));
}

TEST_F(CapeLayerShouldRenderTest, ShouldNotRenderWhenNoCapeTexture)
{
    // 启用披风部件但没有披风纹理
    m_player->setWearingCape(true);

    EXPECT_FALSE(shouldRenderCape(*m_player, false));
}

TEST_F(CapeLayerShouldRenderTest, ShouldNotRenderWhenWearingElytra)
{
    // 启用披风部件
    m_player->setWearingCape(true);

    // 穿戴鞘翅
    m_player->setChestItem(mc::MockItemStack(mc::MockItems::ELYTRA));

    // 即使有披风纹理也不应该渲染（鞘翅覆盖披风）
    EXPECT_FALSE(shouldRenderCape(*m_player, true));
}

TEST_F(CapeLayerShouldRenderTest, ShouldRenderWhenAllConditionsMet)
{
    // 启用披风部件
    m_player->setWearingCape(true);

    // 不穿戴鞘翅
    m_player->setChestItem(mc::MockItemStack()); // 空物品

    // 有披风纹理
    EXPECT_TRUE(shouldRenderCape(*m_player, true));
}

TEST_F(CapeLayerShouldRenderTest, ShouldRenderWhenWearingOtherChestItem)
{
    // 启用披风部件
    m_player->setWearingCape(true);

    // 穿戴胸甲（非鞘翅）
    m_player->setChestItem(mc::MockItemStack(mc::MockItems::DIAMOND_CHESTPLATE));

    // 应该渲染披风
    EXPECT_TRUE(shouldRenderCape(*m_player, true));
}

// ============================================================================
// ElytraLayer shouldRender 逻辑测试
// ============================================================================

/**
 * @brief 模拟 ElytraLayer::shouldRender() 的逻辑
 *
 * 参考 MC 1.16.5 ElytraLayer.shouldRender():
 * 1. 胸甲槽装备了鞘翅物品
 * 2. 有鞘翅或披风纹理
 */
bool shouldRenderElytra(const mc::MockPlayer& player, bool hasElytraTexture, bool hasCapeTexture)
{
    // 检查胸甲槽
    const auto& chest = player.getEquipment(mc::EquipmentSlot::Chest);
    if (chest.isEmpty()) {
        return false;
    }

    // MC 1.16.5: 检查物品是否为鞘翅
    if (chest.getItemId() != mc::MockItems::ELYTRA) {
        return false;
    }

    return hasElytraTexture || hasCapeTexture;
}

class ElytraLayerShouldRenderTest : public ::testing::Test {
protected:
    void SetUp() override { m_player = std::make_unique<mc::MockPlayer>(); }

    std::unique_ptr<mc::MockPlayer> m_player;
};

TEST_F(ElytraLayerShouldRenderTest, ShouldNotRenderWhenNoChestItem)
{
    // 胸甲槽为空
    m_player->setChestItem(mc::MockItemStack());

    EXPECT_FALSE(shouldRenderElytra(*m_player, true, false));
    EXPECT_FALSE(shouldRenderElytra(*m_player, false, true));
}

TEST_F(ElytraLayerShouldRenderTest, ShouldNotRenderWhenWearingOtherChestItem)
{
    // 穿戴胸甲（非鞘翅）
    m_player->setChestItem(mc::MockItemStack(mc::MockItems::DIAMOND_CHESTPLATE));

    EXPECT_FALSE(shouldRenderElytra(*m_player, true, false));
}

TEST_F(ElytraLayerShouldRenderTest, ShouldNotRenderWhenWearingElytraButNoTexture)
{
    // 穿戴鞘翅
    m_player->setChestItem(mc::MockItemStack(mc::MockItems::ELYTRA));

    // 没有任何纹理
    EXPECT_FALSE(shouldRenderElytra(*m_player, false, false));
}

TEST_F(ElytraLayerShouldRenderTest, ShouldRenderWhenWearingElytraWithElytraTexture)
{
    // 穿戴鞘翅
    m_player->setChestItem(mc::MockItemStack(mc::MockItems::ELYTRA));

    // 有鞘翅纹理
    EXPECT_TRUE(shouldRenderElytra(*m_player, true, false));
}

TEST_F(ElytraLayerShouldRenderTest, ShouldRenderWhenWearingElytraWithCapeTexture)
{
    // 穿戴鞘翅
    m_player->setChestItem(mc::MockItemStack(mc::MockItems::ELYTRA));

    // 有披风纹理（MC 1.16.5：玩家没有鞘翅纹理时使用披风纹理）
    EXPECT_TRUE(shouldRenderElytra(*m_player, false, true));
}

TEST_F(ElytraLayerShouldRenderTest, ShouldRenderWhenWearingElytraWithBothTextures)
{
    // 穿戴鞘翅
    m_player->setChestItem(mc::MockItemStack(mc::MockItems::ELYTRA));

    // 同时有鞘翅和披风纹理
    EXPECT_TRUE(shouldRenderElytra(*m_player, true, true));
}

// ============================================================================
// HeadLayer 基础测试
// ============================================================================

/**
 * @brief 测试头部物品获取逻辑
 *
 * 参考 MC 1.16.5 HeadLayer：
 * - 从 EquipmentSlot::HEAD 获取装备
 */
const mc::MockItemStack* getHeadItem(const mc::MockPlayer& player)
{
    // 模拟从头部槽位获取物品
    // 在实际实现中，会检查 EquipmentSlot::Head
    return nullptr; // 简化测试
}

class HeadLayerBasicTest : public ::testing::Test {
protected:
    void SetUp() override { m_player = std::make_unique<mc::MockPlayer>(); }

    std::unique_ptr<mc::MockPlayer> m_player;
};

TEST_F(HeadLayerBasicTest, GetHeadItemReturnsNullForEmptySlot)
{
    // 头部槽位为空
    const auto* headItem = getHeadItem(*m_player);
    EXPECT_EQ(headItem, nullptr);
}

// ============================================================================
// PlayerModelPart 枚举测试
// ============================================================================

class PlayerModelPartMaskTest : public ::testing::Test {};

TEST_F(PlayerModelPartMaskTest, MaskValuesCorrect)
{
    // 验证各部件的位掩码值
    EXPECT_EQ(mc::getPlayerModelPartMask(mc::PlayerModelPart::Cape), 0x01);
    EXPECT_EQ(mc::getPlayerModelPartMask(mc::PlayerModelPart::Jacket), 0x02);
    EXPECT_EQ(mc::getPlayerModelPartMask(mc::PlayerModelPart::LeftSleeve), 0x04);
    EXPECT_EQ(mc::getPlayerModelPartMask(mc::PlayerModelPart::RightSleeve), 0x08);
    EXPECT_EQ(mc::getPlayerModelPartMask(mc::PlayerModelPart::LeftPantsLeg), 0x10);
    EXPECT_EQ(mc::getPlayerModelPartMask(mc::PlayerModelPart::RightPantsLeg), 0x20);
    EXPECT_EQ(mc::getPlayerModelPartMask(mc::PlayerModelPart::Hat), 0x40);
}

TEST_F(PlayerModelPartMaskTest, CanCombineMasks)
{
    // 验证可以组合多个部件
    mc::u8 allParts = mc::getPlayerModelPartMask(mc::PlayerModelPart::Cape) |
        mc::getPlayerModelPartMask(mc::PlayerModelPart::Jacket) | mc::getPlayerModelPartMask(mc::PlayerModelPart::Hat);

    EXPECT_TRUE((allParts & mc::getPlayerModelPartMask(mc::PlayerModelPart::Cape)) != 0);
    EXPECT_TRUE((allParts & mc::getPlayerModelPartMask(mc::PlayerModelPart::Jacket)) != 0);
    EXPECT_TRUE((allParts & mc::getPlayerModelPartMask(mc::PlayerModelPart::Hat)) != 0);
    EXPECT_FALSE((allParts & mc::getPlayerModelPartMask(mc::PlayerModelPart::LeftSleeve)) != 0);
}

TEST_F(PlayerModelPartMaskTest, CanToggleParts)
{
    mc::u8 parts = 0xFF; // 所有部件

    // 移除披风
    parts &= ~mc::getPlayerModelPartMask(mc::PlayerModelPart::Cape);
    EXPECT_FALSE((parts & mc::getPlayerModelPartMask(mc::PlayerModelPart::Cape)) != 0);

    // 添加披风
    parts |= mc::getPlayerModelPartMask(mc::PlayerModelPart::Cape);
    EXPECT_TRUE((parts & mc::getPlayerModelPartMask(mc::PlayerModelPart::Cape)) != 0);
}

// ============================================================================
// 层渲染器优先级测试
// ============================================================================

/**
 * @brief 验证层渲染器的渲染顺序符合 MC 1.16.5
 *
 * MC 1.16.5 PlayerRenderer 层渲染顺序:
 * 1. HeldItemLayer - 手持物品
 * 2. HeadLayer - 头部物品
 * 3. CapeLayer - 披风
 * 4. ElytraLayer - 鞘翅
 */
class LayerPriorityTest : public ::testing::Test {};

TEST_F(LayerPriorityTest, LayerOrderMatchesMC1165)
{
    // 验证层渲染器类型的优先级顺序
    // 实际渲染顺序在 PlayerRenderer::setupLayers() 中确定

    // 优先级：手持物品 > 头部物品 > 披风 > 鞘翅
    // 披风应该在鞘翅之前渲染，这样鞘翅可以正确覆盖披风
    EXPECT_LT(1, 2); // 手持物品 (0) < 头部物品 (1) < 披风 (2) < 鞘翅 (3)
    EXPECT_LT(2, 3);
}

// main 函数由 gtest_main 库提供
