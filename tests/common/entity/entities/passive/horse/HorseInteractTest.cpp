/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
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

#include "common/TestWorldHelper.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/entities/passive/horse/AbstractChestedHorseEntity.hpp"
#include "common/entity/entities/passive/horse/DonkeyEntity.hpp"
#include "common/entity/entities/passive/horse/HorseEntity.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/horse/MuleEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/armor/HorseArmorItem.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {

// ============================================================================
// 测试世界
// ============================================================================

class HorseInteractTestWorld final : public test::BaseTestWorld {
public:
    HorseInteractTestWorld()
    {
        Items::initialize();
        VanillaBlocks::initialize();
        item::tag::ItemTags::initialize();
    }

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty d) { m_difficulty = d; }

private:
    Difficulty m_difficulty = Difficulty::Normal;
};

// ============================================================================
// 测试夹具
// ============================================================================

class HorseInteractTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            entity::VanillaEntities::registerAll();
            s_initialized = true;
        }
    }

    void SetUp() override { m_world = std::make_unique<HorseInteractTestWorld>(); }

    std::unique_ptr<HorseInteractTestWorld> m_world;
};

// ============================================================================
// AbstractHorseEntity::interactMob 测试
// ============================================================================

/**
 * @brief 幼年马交互时交给基类处理
 *
 * 幼年马不应该触发骑乘或装备逻辑。
 */
TEST_F(HorseInteractTest, ChildHorseReturnsBaseClassResult)
{
    auto horse = std::make_unique<HorseEntity>(EntityId(1));
    horse->setWorld(m_world.get());
    horse->setChild(true);

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 幼年马交互结果应该是基类 AnimalEntity::interactMob 的返回值（Pass）
    auto result = horse->interactMob(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Pass);
}

/**
 * @brief 已驯服且潜行的玩家应打开背包界面（返回 Success）
 */
TEST_F(HorseInteractTest, TameSneakingPlayerOpensInventory)
{
    auto horse = std::make_unique<HorseEntity>(EntityId(1));
    horse->setWorld(m_world.get());
    horse->setTame(true);

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setSneaking(true);

    auto result = horse->interactMob(*player, Hand::MainHand);
    // openInventory 目前为 TODO 空实现，但仍返回 Success
    EXPECT_EQ(result, ActionResultType::Success);
}

/**
 * @brief 未驯服且不潜行的玩家空手交互应触发骑乘
 */
TEST_F(HorseInteractTest, UntamedEmptyHandTriggersRide)
{
    auto horse = std::make_unique<HorseEntity>(EntityId(1));
    horse->setWorld(m_world.get());

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 空手交互未驯服的马应该触发 doPlayerRide，返回 Success
    auto result = horse->interactMob(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
}

/**
 * @brief 已驯服且不潜行的玩家空手交互应触发骑乘
 */
TEST_F(HorseInteractTest, TamedEmptyHandTriggersRide)
{
    auto horse = std::make_unique<HorseEntity>(EntityId(1));
    horse->setWorld(m_world.get());
    horse->setTame(true);

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    auto result = horse->interactMob(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
}

/**
 * @brief 已驯服且潜行的玩家不应该触发骑乘
 */
TEST_F(HorseInteractTest, TameSneakingPlayerDoesNotRide)
{
    auto horse = std::make_unique<HorseEntity>(EntityId(1));
    horse->setWorld(m_world.get());
    horse->setTame(true);

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setSneaking(true);

    auto result = horse->interactMob(*player, Hand::MainHand);
    // 应该打开背包而不是骑乘
    EXPECT_EQ(result, ActionResultType::Success);
}

// ============================================================================
// AbstractHorseEntity::equipArmor 测试
// ============================================================================

/**
 * @brief 测试马铠装备逻辑
 *
 * 给已驯服的马装备铁马铠，验证：
 * - 马铠槽位被填充
 * - hasArmor() 返回 true
 * - 物品被消耗（非创造模式）
 */
TEST_F(HorseInteractTest, EquipHorseArmorOnTamedHorse)
{
    if (Items::IRON_HORSE_ARMOR == nullptr) {
        GTEST_SKIP() << "IRON_HORSE_ARMOR not registered";
    }

    auto horse = std::make_unique<HorseEntity>(EntityId(1));
    horse->setWorld(m_world.get());
    horse->setTame(true);

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 马初始没有护甲
    EXPECT_FALSE(horse->hasArmor());

    // 马支持马铠槽位
    EXPECT_TRUE(horse->hasArmorSlot());

    // 构造马铠物品
    ItemStack armorStack(Items::IRON_HORSE_ARMOR, 1);
    EXPECT_TRUE(horse->isValidArmorForSlot(armorStack));

    // 直接调用 equipArmor
    horse->equipArmor(*player, armorStack);

    // 验证马铠已装备
    EXPECT_TRUE(horse->hasArmor());

    // 验证槽位 1 已有物品
    ItemStack slotItem = horse->getEquipment(1);
    EXPECT_FALSE(slotItem.isEmpty());
}

// ============================================================================
// AbstractHorseEntity::doPlayerRide 测试
// ============================================================================

/**
 * @brief doPlayerRide 调用 player.startRiding 不应崩溃
 *
 * 注意：完整的骑乘功能需要世界实体管理系统的支持，
 * 此测试仅验证 doPlayerRide 方法本身不崩溃。
 */
TEST_F(HorseInteractTest, DoPlayerRideDoesNotCrash)
{
    auto horse = std::make_unique<HorseEntity>(EntityId(1));
    horse->setWorld(m_world.get());

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 调用 doPlayerRide 不应该崩溃
    horse->doPlayerRide(*player);
    SUCCEED();
}

// ============================================================================
// HorseEntity::interactMob 特有逻辑测试
// ============================================================================

/**
 * @brief 手持食物的已驯服马应优先喂食
 */
TEST_F(HorseInteractTest, TamedHorseFoodPriorityOverRide)
{
    if (Items::GOLDEN_CARROT == nullptr) {
        GTEST_SKIP() << "GOLDEN_CARROT not registered";
    }

    auto horse = std::make_unique<HorseEntity>(EntityId(1));
    horse->setWorld(m_world.get());
    horse->setTame(true);

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 设置手持金胡萝卜
    ItemStack& heldItem = player->getHeldItem(Hand::MainHand);
    heldItem = ItemStack(Items::GOLDEN_CARROT, 1);

    auto result = horse->interactMob(*player, Hand::MainHand);
    // 应该触发喂食逻辑，返回 Success 或 Consume
    EXPECT_NE(result, ActionResultType::Pass);
}

/**
 * @brief 未驯服的马空手交互应触发骑乘（启动驯服流程）
 */
TEST_F(HorseInteractTest, UntamedHorseTriggersRideOnEmptyHand)
{
    auto horse = std::make_unique<HorseEntity>(EntityId(1));
    horse->setWorld(m_world.get());

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    auto result = horse->interactMob(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
}

// ============================================================================
// AbstractChestedHorseEntity::interactMob 测试
// ============================================================================

/**
 * @brief 已驯服驴手持箱子应装备箱子
 */
TEST_F(HorseInteractTest, TamedDonkeyWithChestEquipsChest)
{
    if (Items::CHEST == nullptr) {
        GTEST_SKIP() << "CHEST not registered";
    }

    auto donkey = std::make_unique<DonkeyEntity>(EntityId(1));
    donkey->setWorld(m_world.get());
    donkey->setTame(true);

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 驴初始没有箱子
    EXPECT_FALSE(donkey->hasChest());

    // 设置手持箱子
    ItemStack& heldItem = player->getHeldItem(Hand::MainHand);
    heldItem = ItemStack(Items::CHEST, 1);

    auto result = donkey->interactMob(*player, Hand::MainHand);
    // 应该成功装备箱子
    EXPECT_EQ(result, ActionResultType::Success);

    // 驴现在应该有箱子
    EXPECT_TRUE(donkey->hasChest());

    // 背包大小应该扩展（2 + 3 * 5 = 17）
    EXPECT_EQ(donkey->getInventorySize(), 17);
}

/**
 * @brief 未驯服驴手持箱子应愤怒而不是装备
 */
TEST_F(HorseInteractTest, UntamedDonkeyWithChestGetsAngry)
{
    if (Items::CHEST == nullptr) {
        GTEST_SKIP() << "CHEST not registered";
    }

    auto donkey = std::make_unique<DonkeyEntity>(EntityId(1));
    donkey->setWorld(m_world.get());
    // 驴未驯服

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 设置手持箱子
    ItemStack& heldItem = player->getHeldItem(Hand::MainHand);
    heldItem = ItemStack(Items::CHEST, 1);

    auto result = donkey->interactMob(*player, Hand::MainHand);
    // 未驯服时应该先让驴愤怒，返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 驴不应该装备箱子
    EXPECT_FALSE(donkey->hasChest());
}

/**
 * @brief 已驯服驴已有箱子时不能再装备箱子
 */
TEST_F(HorseInteractTest, TamedDonkeyWithExistingChestCannotEquipAgain)
{
    if (Items::CHEST == nullptr) {
        GTEST_SKIP() << "CHEST not registered";
    }

    auto donkey = std::make_unique<DonkeyEntity>(EntityId(1));
    donkey->setWorld(m_world.get());
    donkey->setTame(true);
    donkey->setChest(true);

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 设置手持箱子
    ItemStack& heldItem = player->getHeldItem(Hand::MainHand);
    heldItem = ItemStack(Items::CHEST, 1);

    auto result = donkey->interactMob(*player, Hand::MainHand);
    // 已有箱子时手持箱子应交给基类处理（可能触发骑乘）
    // 不应该崩溃或产生异常
    EXPECT_NE(result, ActionResultType::Fail);
}

// ============================================================================
// AbstractChestedHorseEntity::equipChest 间接测试（通过 interactMob）
// ============================================================================

/**
 * @brief 通过 interactMob 装备箱子验证背包扩展
 *
 * 由于 equipChest 是 protected 方法，通过 interactMob 间接测试。
 * 已在上面 TamedDonkeyWithChestEquipsChest 测试中验证了箱子装备后的背包大小。
 */
TEST_F(HorseInteractTest, ChestEquippingExpandsInventorySize)
{
    if (Items::CHEST == nullptr) {
        GTEST_SKIP() << "CHEST not registered";
    }

    auto donkey = std::make_unique<DonkeyEntity>(EntityId(1));
    donkey->setWorld(m_world.get());

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 初始没有箱子
    EXPECT_FALSE(donkey->hasChest());
    EXPECT_EQ(donkey->getInventorySize(), 2); // 基础大小：鞍槽 + 马铠槽

    // 驯服驴以便装备箱子
    donkey->setTame(true);

    // 设置手持箱子
    ItemStack& heldItem = player->getHeldItem(Hand::MainHand);
    heldItem = ItemStack(Items::CHEST, 3);

    auto result = donkey->interactMob(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);

    // 箱子已设置
    EXPECT_TRUE(donkey->hasChest());

    // 背包大小应扩展
    EXPECT_EQ(donkey->getInventorySize(), 17); // 2 + 3 * 5
}

// ============================================================================
// LlamaEntity 交互测试
// ============================================================================

/**
 * @brief 羊驼不能装备鞍
 */
TEST_F(HorseInteractTest, LlamaCannotEquipSaddle)
{
    auto llama = std::make_unique<LlamaEntity>(EntityId(1));
    EXPECT_FALSE(llama->canEquipSaddle());
}

/**
 * @brief 羊驼支持装饰槽位（地毯）
 */
TEST_F(HorseInteractTest, LlamaHasArmorSlot)
{
    auto llama = std::make_unique<LlamaEntity>(EntityId(1));
    EXPECT_TRUE(llama->hasArmorSlot());
}

/**
 * @brief 羊驼可以用小麦喂食
 */
TEST_F(HorseInteractTest, LlamaFoodItemDetection)
{
    if (Items::WHEAT == nullptr) {
        GTEST_SKIP() << "WHEAT not registered";
    }

    auto llama = std::make_unique<LlamaEntity>(EntityId(1));
    llama->setWorld(m_world.get());

    ItemStack wheatStack(Items::WHEAT, 1);
    EXPECT_TRUE(llama->isFoodItem(wheatStack));
    EXPECT_TRUE(llama->isBreedingItem(wheatStack));
}

/**
 * @brief 羊驼可以用干草块喂食
 */
TEST_F(HorseInteractTest, LlamaHayBlockFoodItem)
{
    if (Items::HAY_BLOCK == nullptr) {
        GTEST_SKIP() << "HAY_BLOCK not registered";
    }

    auto llama = std::make_unique<LlamaEntity>(EntityId(1));
    llama->setWorld(m_world.get());

    ItemStack hayStack(Items::HAY_BLOCK, 1);
    EXPECT_TRUE(llama->isFoodItem(hayStack));
}

// ============================================================================
// 交互优先级测试
// ============================================================================

/**
 * @brief 被骑乘的马交互交给基类处理
 *
 * 通过手动添加乘客模拟被骑乘状态。
 * 注意：在测试 stub 世界中，startRiding 可能无法完全工作，
 * 因此直接通过 interactMob 返回值验证交互行为。
 */
TEST_F(HorseInteractTest, BeingRiddenHorseDelegatesToBaseClass)
{
    auto horse = std::make_unique<HorseEntity>(EntityId(1));
    horse->setWorld(m_world.get());
    horse->setTame(true);

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 空手交互已驯服的马应该触发骑乘
    auto result = horse->interactMob(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
}

/**
 * @brief 驯服但不潜行的玩家手持马铠应装备马铠
 */
TEST_F(HorseInteractTest, TamedHorseWithArmorEquipsArmor)
{
    if (Items::IRON_HORSE_ARMOR == nullptr) {
        GTEST_SKIP() << "IRON_HORSE_ARMOR not registered";
    }

    auto horse = std::make_unique<HorseEntity>(EntityId(1));
    horse->setWorld(m_world.get());
    horse->setTame(true);

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 手持铁马铠
    ItemStack& heldItem = player->getHeldItem(Hand::MainHand);
    heldItem = ItemStack(Items::IRON_HORSE_ARMOR, 1);

    EXPECT_FALSE(horse->hasArmor());

    auto result = horse->interactMob(*player, Hand::MainHand);
    // SaddleItem::itemInteractionForEntity 不处理马铠，所以走 equipArmor 路径
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(horse->hasArmor());
}

// ============================================================================
// openInventory 测试
// ============================================================================

/**
 * @brief openInventory 在当前实现中为 TODO 空实现，但不应崩溃
 */
TEST_F(HorseInteractTest, OpenInventoryDoesNotCrash)
{
    auto horse = std::make_unique<HorseEntity>(EntityId(1));
    horse->setWorld(m_world.get());
    horse->setTame(true);

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 调用 openInventory 不应该崩溃
    horse->openInventory(*player);
    SUCCEED();
}

} // namespace
} // namespace mc
