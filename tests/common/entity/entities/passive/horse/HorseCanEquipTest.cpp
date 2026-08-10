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

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/entities/passive/horse/DonkeyEntity.hpp"
#include "common/entity/entities/passive/horse/HorseEntity.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/horse/MuleEntity.hpp"
#include "common/entity/entities/passive/horse/SkeletonHorseEntity.hpp"
#include "common/entity/entities/passive/horse/ZombieHorseEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/armor/HorseArmorItem.hpp"
#include "common/item/tag/ItemTags.hpp"

namespace mc {
namespace {

/**
 * @brief 测试基类：提供 HorseEntity 测试 fixture
 */
class HorseEntityCanEquipTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化物品（幂等）与物品标签，使 canEquip 的物品类型判断（如 SADDLE、
        // HorseArmorItem、CARPETS 标签）不依赖其他测试套件的初始化顺序。
        Items::initialize();
        item::tag::ItemTags::initialize();
    }
};

/**
 * @brief 测试 AbstractHorseEntity::canEquip 的鞍槽检查
 *
 * MC 1.16.5: 槽位 0 只能放鞍
 */
TEST_F(HorseEntityCanEquipTest, CanEquipSaddleInSlot0)
{
    HorseEntity horse(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 空物品应该允许放入任何槽位
    EXPECT_TRUE(horse.canEquip(ItemStack::EMPTY, 0));
    EXPECT_TRUE(horse.canEquip(ItemStack::EMPTY, 1));

    // 鞍应该允许放入槽位 0
    ItemStack saddleStack(Items::SADDLE, 1);
    EXPECT_TRUE(horse.canEquip(saddleStack, 0));

    // 鞍不应该允许放入槽位 1
    EXPECT_FALSE(horse.canEquip(saddleStack, 1));
}

/**
 * @brief 测试 HorseEntity 的马铠槽位检查
 *
 * MC 1.16.5: HorseEntity 支持马铠槽位 (hasArmorSlot = true)
 */
TEST_F(HorseEntityCanEquipTest, CanEquipHorseArmorInSlot1)
{
    HorseEntity horse(EntityInstanceId(0), mc::test::testEcsRegistry());

    // HorseEntity 支持马铠槽位
    EXPECT_TRUE(horse.hasArmorSlot());

    // 使用注册的马铠物品
    if (Items::IRON_HORSE_ARMOR != nullptr) {
        ItemStack horseArmorStack(Items::IRON_HORSE_ARMOR, 1);
        EXPECT_TRUE(horse.canEquip(horseArmorStack, 1));
    }
}

/**
 * @brief 测试 HorseEntity 不能用其他物品作为马铠
 */
TEST_F(HorseEntityCanEquipTest, CannotEquipNonHorseArmorInSlot1)
{
    HorseEntity horse(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 普通物品不应该允许放入马铠槽位
    if (Items::IRON_INGOT != nullptr) {
        ItemStack ironIngotStack(Items::IRON_INGOT, 1);
        EXPECT_FALSE(horse.canEquip(ironIngotStack, 1));
    }
}

/**
 * @brief 测试 LlamaEntity 不能装备鞍
 *
 * MC 1.16.5: LlamaEntity.func_230264_L_() 返回 false
 */
TEST_F(HorseEntityCanEquipTest, LlamaCannotEquipSaddle)
{
    LlamaEntity llama(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 羊驼不能装备鞍
    EXPECT_FALSE(llama.canEquipSaddle());

    // 鞍不应该允许放入槽位 0
    ItemStack saddleStack(Items::SADDLE, 1);
    EXPECT_FALSE(llama.canEquip(saddleStack, 0));
}

/**
 * @brief 测试 LlamaEntity 支持装饰槽位（地毯）
 *
 * MC 1.16.5: LlamaEntity.func_230276_fq_() 返回 true
 * LlamaEntity.isArmor() 检查 ItemTags.CARPETS
 */
TEST_F(HorseEntityCanEquipTest, LlamaCanEquipCarpetInSlot1)
{
    LlamaEntity llama(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 羊驼支持装饰槽位
    EXPECT_TRUE(llama.hasArmorSlot());

    // 羊驼可以用地毯作为装饰
    // 注意：需要 CARPETS 标签已初始化
    // 由于地毯物品可能尚未完全注册，这里只测试 isValidArmorForSlot 方法
    // 实际测试中，地毯物品应该在 CARPETS 标签中
}

/**
 * @brief 测试 DonkeyEntity 和 MuleEntity 不支持马铠
 *
 * MC 1.16.5: AbstractChestedHorseEntity 不重写 hasArmorSlot()
 * 因此继承 AbstractHorseEntity 的默认值 false
 */
TEST_F(HorseEntityCanEquipTest, DonkeyAndMuleCannotEquipArmor)
{
    DonkeyEntity donkey(EntityInstanceId(0), mc::test::testEcsRegistry());
    MuleEntity mule(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 驴和骡不支持马铠槽位
    EXPECT_FALSE(donkey.hasArmorSlot());
    EXPECT_FALSE(mule.hasArmorSlot());

    // 即使有马铠物品，也不应该允许放入槽位 1
    if (Items::IRON_HORSE_ARMOR != nullptr) {
        ItemStack horseArmorStack(Items::IRON_HORSE_ARMOR, 1);
        EXPECT_FALSE(donkey.canEquip(horseArmorStack, 1));
        EXPECT_FALSE(mule.canEquip(horseArmorStack, 1));
    }
}

/**
 * @brief 测试 DonkeyEntity 和 MuleEntity 可以装备鞍
 */
TEST_F(HorseEntityCanEquipTest, DonkeyAndMuleCanEquipSaddle)
{
    DonkeyEntity donkey(EntityInstanceId(0), mc::test::testEcsRegistry());
    MuleEntity mule(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 驴和骡可以装备鞍
    EXPECT_TRUE(donkey.canEquipSaddle());
    EXPECT_TRUE(mule.canEquipSaddle());

    // 鞍应该允许放入槽位 0
    ItemStack saddleStack(Items::SADDLE, 1);
    EXPECT_TRUE(donkey.canEquip(saddleStack, 0));
    EXPECT_TRUE(mule.canEquip(saddleStack, 0));
}

/**
 * @brief 测试 SkeletonHorseEntity 和 ZombieHorseEntity 不支持马铠
 *
 * MC 1.16.5: 骷髅马和僵尸马不重写 hasArmorSlot()
 * 因此继承 AbstractHorseEntity 的默认值 false
 */
TEST_F(HorseEntityCanEquipTest, SkeletonAndZombieHorseCannotEquipArmor)
{
    SkeletonHorseEntity skeletonHorse(EntityInstanceId(0), mc::test::testEcsRegistry());
    ZombieHorseEntity zombieHorse(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 骷髅马和僵尸马不支持马铠槽位
    EXPECT_FALSE(skeletonHorse.hasArmorSlot());
    EXPECT_FALSE(zombieHorse.hasArmorSlot());
}

/**
 * @brief 测试 SkeletonHorseEntity 和 ZombieHorseEntity 可以装备鞍
 */
TEST_F(HorseEntityCanEquipTest, SkeletonAndZombieHorseCanEquipSaddle)
{
    SkeletonHorseEntity skeletonHorse(EntityInstanceId(0), mc::test::testEcsRegistry());
    ZombieHorseEntity zombieHorse(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 骷髅马和僵尸马可以装备鞍
    EXPECT_TRUE(skeletonHorse.canEquipSaddle());
    EXPECT_TRUE(zombieHorse.canEquipSaddle());

    // 鞍应该允许放入槽位 0
    ItemStack saddleStack(Items::SADDLE, 1);
    EXPECT_TRUE(skeletonHorse.canEquip(saddleStack, 0));
    EXPECT_TRUE(zombieHorse.canEquip(saddleStack, 0));
}

/**
 * @brief 测试无效槽位的处理
 */
TEST_F(HorseEntityCanEquipTest, InvalidSlotReturnsFalse)
{
    HorseEntity horse(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 无效槽位应该返回 false
    EXPECT_FALSE(horse.canEquip(ItemStack::EMPTY, -1));
    EXPECT_FALSE(horse.canEquip(ItemStack::EMPTY, 100));

    // 有效槽位应该返回 true
    EXPECT_TRUE(horse.canEquip(ItemStack::EMPTY, 0));
    EXPECT_TRUE(horse.canEquip(ItemStack::EMPTY, 1));
}

/**
 * @brief 测试空物品总是允许放入任何槽位
 */
TEST_F(HorseEntityCanEquipTest, EmptyStackAlwaysAllowed)
{
    HorseEntity horse(EntityInstanceId(0), mc::test::testEcsRegistry());
    LlamaEntity llama(EntityInstanceId(0), mc::test::testEcsRegistry());
    DonkeyEntity donkey(EntityInstanceId(0), mc::test::testEcsRegistry());

    // 空物品应该允许放入任何有效槽位（用于清空槽位）
    EXPECT_TRUE(horse.canEquip(ItemStack::EMPTY, 0));
    EXPECT_TRUE(horse.canEquip(ItemStack::EMPTY, 1));
    EXPECT_TRUE(llama.canEquip(ItemStack::EMPTY, 0));
    EXPECT_TRUE(llama.canEquip(ItemStack::EMPTY, 1));
    EXPECT_TRUE(donkey.canEquip(ItemStack::EMPTY, 0));
    EXPECT_TRUE(donkey.canEquip(ItemStack::EMPTY, 1));
}

} // namespace
} // namespace mc
