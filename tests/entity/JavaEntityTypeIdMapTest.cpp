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

// AddEntity.entityTypeId 对齐 vanilla 1.21.11 entity_type 注册表 id 测试。
//
// 背景：entity_type 注册表不在 Configuration 同步的 23 个注册表内，真 Java 客户端用内置
// vanilla core 包注册表 id 解析 AddEntity.entityTypeId。vanilla 1.21.11 EntityType.java 静态
// register("name",...) 调用顺序（字母序，157 条，id 0..156）即 registry id。项目内部
// EntityType::id() 是 VanillaEntities.cpp registerType 注册序（PIG=0/.../ITEM=82），与之不同。
// 直接发项目 id 会让客户端 spawn 错误实体类型——例如项目 item=82 对应 vanilla id 82=
// mangrove_chest_boat，客户端把掉落物渲染成红树木运输船，随后 ItemEntity 的 field8(ItemStack)
// 撞上 chest_boat 的 field8(Int) 致 set_entity_data 类型校验崩溃
// （disconnect-2026-07-31_09.37.02-client.txt）。
//
// JavaEntityTypeIdMap 提供 name→vanilla id 映射；船类按木种选变体。本测试锁定关键映射防回归。

#include "common/world/entity/JavaEntityTypeIdMap.hpp"
#include "entity/entities/vehicle/BoatEntity.hpp"
#include "entity/entities/vehicle/ChestBoatEntity.hpp"

#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

namespace {

// 初始化单例（幂等），返回映射表引用。
const JavaEntityTypeIdMap& ensureMap()
{
    JavaEntityTypeIdMap::instance().initialize();
    return JavaEntityTypeIdMap::instance();
}

} // namespace

// ============================================================================
// 关键 vanilla id（提取自 EntityType.java register 顺序）——任何一条错都会让对应实体
// 在客户端 spawn 成错误类型并触发 set_entity_data 类型校验崩溃。
// ============================================================================

TEST(JavaEntityTypeIdMapTest, VanillaKeyIdsMatchGolden)
{
    const auto& map = ensureMap();

    // 崩溃根因三联：项目 item=82 ↔ vanilla id 82=mangrove_chest_boat。
    EXPECT_EQ(map.toJavaRegistryId("minecraft:item"), 71u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:mangrove_chest_boat"), 82u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:mangrove_boat"), 81u);

    // 首尾与高频实体。
    EXPECT_EQ(map.toJavaRegistryId("minecraft:acacia_boat"), 0u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:fishing_bobber"), 156u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:pig"), 100u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:zombie"), 150u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:player"), 155u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:cow"), 30u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:sheep"), 111u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:chicken"), 26u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:creeper"), 32u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:skeleton"), 115u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:enderman"), 41u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:falling_block"), 51u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:tnt"), 132u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:armor_stand"), 5u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:experience_orb"), 49u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:item_frame"), 73u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:painting"), 93u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:leash_knot"), 76u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:villager"), 139u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:wolf"), 148u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:cat"), 21u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:fox"), 54u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:trident"), 135u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:arrow"), 6u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:snowball"), 120u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:egg"), 39u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:ender_pearl"), 44u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:fireball"), 52u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:small_fireball"), 118u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:wither_skull"), 147u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:firework_rocket"), 53u);
    EXPECT_EQ(map.toJavaRegistryId("minecraft:eye_of_ender"), 50u);
}

// 项目特有键别名（vanilla 无泛型 potion/spear）映射到选定的 vanilla id。
TEST(JavaEntityTypeIdMapTest, ProjectOnlyKeysUseAliases)
{
    const auto& map = ensureMap();
    EXPECT_EQ(map.toJavaRegistryId("minecraft:potion"), 105u); // → splash_potion
    EXPECT_EQ(map.toJavaRegistryId("minecraft:spear"), 135u);  // → trident
}

// 未知键兜底为 0（acacia_boat），不抛异常。
TEST(JavaEntityTypeIdMapTest, UnknownKeyFallsBackToZero)
{
    const auto& map = ensureMap();
    EXPECT_EQ(map.toJavaRegistryId("minecraft:does_not_exist"), 0u);
    EXPECT_EQ(map.toJavaRegistryId(""), 0u);
}

// 反向映射（调试用）。
TEST(JavaEntityTypeIdMapTest, ReverseLookup)
{
    const auto& map = ensureMap();
    EXPECT_EQ(map.fromJavaRegistryId(71u), "minecraft:item");
    EXPECT_EQ(map.fromJavaRegistryId(82u), "minecraft:mangrove_chest_boat");
    EXPECT_EQ(map.fromJavaRegistryId(0u), "minecraft:acacia_boat");
    EXPECT_EQ(map.fromJavaRegistryId(156u), "minecraft:fishing_bobber");
}

// ============================================================================
// 船类按木种选 vanilla 变体 id（BoatEntity/ChestBoatEntity override getJavaEntityTypeId）
// vanilla 无泛型 boat/chest_boat，按 m_type 拼 <wood>_boat/<wood>_chest_boat/bamboo_raft。
// ============================================================================

TEST(JavaEntityTypeIdMapTest, BoatEntitySelectsVariantByWoodType)
{
    const auto& map = ensureMap();

    struct Case {
        BoatEntity::Type type;
        u32 expectedBoatId;
        u32 expectedChestId;
    };
    // expected ids 取自 vanilla EntityType.java：oak_boat=89/oak_chest_boat=90、
    // spruce_boat=125/spruce_chest_boat=126、birch_boat=12/birch_chest_boat=13、
    // jungle_boat=74/jungle_chest_boat=75、acacia_boat=0/acacia_chest_boat=1、
    // dark_oak_boat=33/dark_oak_chest_boat=34、mangrove_boat=81/mangrove_chest_boat=82、
    // cherry_boat=23/cherry_chest_boat=24、pale_oak_boat=94/pale_oak_chest_boat=95、
    // bamboo_raft=9/bamboo_chest_raft=8。
    const Case cases[] = {
        {BoatEntity::Type::OAK, 89u, 90u},
        {BoatEntity::Type::SPRUCE, 125u, 126u},
        {BoatEntity::Type::BIRCH, 12u, 13u},
        {BoatEntity::Type::JUNGLE, 74u, 75u},
        {BoatEntity::Type::ACACIA, 0u, 1u},
        {BoatEntity::Type::DARK_OAK, 33u, 34u},
        {BoatEntity::Type::MANGROVE, 81u, 82u},
        {BoatEntity::Type::CHERRY, 23u, 24u},
        {BoatEntity::Type::PALE_OAK, 94u, 95u},
        {BoatEntity::Type::BAMBOO, 9u, 8u},
    };

    for (const auto& c : cases) {
        BoatEntity boat(c.type);
        ChestBoatEntity chestBoat(c.type);
        EXPECT_EQ(boat.getJavaEntityTypeId(), c.expectedBoatId) << "boat wood type " << static_cast<int>(c.type);
        EXPECT_EQ(chestBoat.getJavaEntityTypeId(), c.expectedChestId)
            << "chest boat wood type " << static_cast<int>(c.type);
        // 船变体名应能在映射表中查到（即 override 拼出的 name 是 vanilla 合法名）。
        EXPECT_NE(map.fromJavaRegistryId(boat.getJavaEntityTypeId()), "")
            << "boat variant name not in vanilla table for wood " << static_cast<int>(c.type);
    }
}

// 崩溃现场复现断言：项目 item 的 vanilla id(71) 绝不能等于 mangrove_chest_boat 的 id(82)。
// 若映射回归（item 错映到 82），客户端会把掉落物渲染成红树木运输船并崩溃。
TEST(JavaEntityTypeIdMapTest, ItemIdDiffersFromMangroveChestBoat)
{
    const auto& map = ensureMap();
    const u32 itemId = map.toJavaRegistryId("minecraft:item");
    const u32 mangroveChestBoatId = map.toJavaRegistryId("minecraft:mangrove_chest_boat");
    EXPECT_NE(itemId, mangroveChestBoatId);
    EXPECT_EQ(itemId, 71u);
    EXPECT_EQ(mangroveChestBoatId, 82u);
}
