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
 * @file ChestedHorseNbtPersistTest.cpp
 * @brief 驴/骡的 `ChestedHorse` 标志存盘
 *
 * 这个标志此前完全没有持久化（1.16.5 残留缺陷）：装过箱子的驴读档后会退回"没箱子"，
 * 库存规模从 2 + 3*columns 掉回 2 格，箱子里的东西一并丢失。
 * 原版 `AbstractChestedHorse.addAdditionalSaveData` 就是一句
 * `putBoolean("ChestedHorse", hasChest())`。
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/passive/horse/DonkeyEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"

#include <gtest/gtest.h>

using namespace mc;

namespace {

class ChestedHorseNbtTestWorld final : public mc::test::BaseTestWorld {};

class ChestedHorseNbtPersistTest : public testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }

    ChestedHorseNbtTestWorld m_world;
};

} // namespace

// 装了箱子的驴：存盘必须写出 ChestedHorse，读回后仍是 true
TEST_F(ChestedHorseNbtPersistTest, LoadedDonkeyKeepsChestFlag)
{
    DonkeyEntity donkey(EntityInstanceId(1), mc::test::testEcsRegistry());
    donkey.setWorld(&m_world);
    donkey.setChest(true);
    ASSERT_TRUE(donkey.hasChest());

    nbt::tags::compound_tag tag;
    donkey.writeToNBT(tag);

    ASSERT_NE(entity::serialization::nbt_helper::tryGetBool(tag, entity::serialization::nbt_keys::HORSE_CHESTED),
        std::nullopt)
        << "ChestedHorse must be written to NBT";

    DonkeyEntity restored(EntityInstanceId(2), mc::test::testEcsRegistry());
    restored.setWorld(&m_world);
    ASSERT_FALSE(restored.hasChest()) << "a fresh donkey starts without a chest";
    restored.readFromNBT(tag);

    EXPECT_TRUE(restored.hasChest()) << "the chest flag must survive a save/load round trip";
}

// 没装箱的驴：读回后仍是 false（不能因为读到缺省键就变成 true）
TEST_F(ChestedHorseNbtPersistTest, DonkeyWithoutChestStaysFalse)
{
    DonkeyEntity donkey(EntityInstanceId(1), mc::test::testEcsRegistry());
    donkey.setWorld(&m_world);
    ASSERT_FALSE(donkey.hasChest());

    nbt::tags::compound_tag tag;
    donkey.writeToNBT(tag);

    DonkeyEntity restored(EntityInstanceId(2), mc::test::testEcsRegistry());
    restored.setWorld(&m_world);
    restored.setChest(true);
    restored.readFromNBT(tag);

    EXPECT_FALSE(restored.hasChest()) << "a saved false must overwrite an existing true";
}
