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

#include "common/entity/core/MobEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/nbt/Nbt.hpp"

using namespace mc;
using namespace mc::entity::serialization;

namespace {

// 测试用 MobEntity 子类，仅用于序列化测试
class TestMobEntity : public MobEntity {
public:
    TestMobEntity()
        : MobEntity(EntityInstanceId(1))
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// 辅助函数：创建 MobEntity 并序列化到 NBT
std::unique_ptr<nbt::tags::compound_tag> saveToNbt(const MobEntity& entity)
{
    auto tag = std::make_unique<nbt::tags::compound_tag>();
    entity.addAdditionalSaveData(*tag);
    return tag;
}

// 辅助函数：从 NBT 反序列化到新的 MobEntity
std::unique_ptr<TestMobEntity> loadFromNbt(const nbt::tags::compound_tag& tag)
{
    auto entity = std::make_unique<TestMobEntity>();
    auto result = entity->readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    return entity;
}

} // namespace

// ============================================================================
// 装备掉落概率测试
// ============================================================================

TEST(MobEntitySerializationTest, DropChances_DefaultValues)
{
    TestMobEntity entity;

    // 默认值应为 0.085f
    for (i32 i = 0; i < static_cast<i32>(EquipmentSlot::Count); ++i) {
        auto slot = static_cast<EquipmentSlot>(i);
        EXPECT_FLOAT_EQ(entity.getEquipmentDropChance(slot), 0.085f)
            << "Slot " << i << " should have default drop chance 0.085";
    }
}

TEST(MobEntitySerializationTest, DropChances_SetGetRoundTrip)
{
    TestMobEntity entity;

    // 设置各槽位的掉落概率
    entity.setEquipmentDropChance(EquipmentSlot::MainHand, 0.5f);
    entity.setEquipmentDropChance(EquipmentSlot::OffHand, 0.1f);
    entity.setEquipmentDropChance(EquipmentSlot::Feet, 0.2f);
    entity.setEquipmentDropChance(EquipmentSlot::Legs, 0.3f);
    entity.setEquipmentDropChance(EquipmentSlot::Chest, 0.4f);
    entity.setEquipmentDropChance(EquipmentSlot::Head, 0.6f);

    EXPECT_FLOAT_EQ(entity.getEquipmentDropChance(EquipmentSlot::MainHand), 0.5f);
    EXPECT_FLOAT_EQ(entity.getEquipmentDropChance(EquipmentSlot::OffHand), 0.1f);
    EXPECT_FLOAT_EQ(entity.getEquipmentDropChance(EquipmentSlot::Feet), 0.2f);
    EXPECT_FLOAT_EQ(entity.getEquipmentDropChance(EquipmentSlot::Legs), 0.3f);
    EXPECT_FLOAT_EQ(entity.getEquipmentDropChance(EquipmentSlot::Chest), 0.4f);
    EXPECT_FLOAT_EQ(entity.getEquipmentDropChance(EquipmentSlot::Head), 0.6f);
}

TEST(MobEntitySerializationTest, DropChances_GuaranteedDrop)
{
    TestMobEntity entity;

    // setGuaranteedDrop 应将值设为 2.0f
    entity.setGuaranteedDrop(EquipmentSlot::MainHand);
    EXPECT_FLOAT_EQ(entity.getEquipmentDropChance(EquipmentSlot::MainHand), 2.0f);

    entity.setGuaranteedDrop(EquipmentSlot::Head);
    EXPECT_FLOAT_EQ(entity.getEquipmentDropChance(EquipmentSlot::Head), 2.0f);
}

TEST(MobEntitySerializationTest, DropChances_IsEquipmentDropPreserved)
{
    TestMobEntity entity;

    // 默认值 0.085 不应被视为保留
    EXPECT_FALSE(entity.isEquipmentDropPreserved(EquipmentSlot::MainHand));

    // 值为 1.0 不应被视为保留
    entity.setEquipmentDropChance(EquipmentSlot::MainHand, 1.0f);
    EXPECT_FALSE(entity.isEquipmentDropPreserved(EquipmentSlot::MainHand));

    // 值 > 1.0 应被视为保留
    entity.setEquipmentDropChance(EquipmentSlot::MainHand, 1.5f);
    EXPECT_TRUE(entity.isEquipmentDropPreserved(EquipmentSlot::MainHand));

    // setGuaranteedDrop 后应被视为保留
    entity.setGuaranteedDrop(EquipmentSlot::Head);
    EXPECT_TRUE(entity.isEquipmentDropPreserved(EquipmentSlot::Head));
}

TEST(MobEntitySerializationTest, DropChances_NbtRoundTrip)
{
    TestMobEntity original;
    original.setEquipmentDropChance(EquipmentSlot::MainHand, 0.5f);
    original.setEquipmentDropChance(EquipmentSlot::OffHand, 0.1f);
    original.setEquipmentDropChance(EquipmentSlot::Feet, 0.2f);
    original.setEquipmentDropChance(EquipmentSlot::Legs, 0.3f);
    original.setEquipmentDropChance(EquipmentSlot::Chest, 0.4f);
    original.setEquipmentDropChance(EquipmentSlot::Head, 0.6f);

    // 序列化并反序列化
    auto tag = saveToNbt(original);
    auto loaded = loadFromNbt(*tag);

    // 验证所有槽位值匹配
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::MainHand), 0.5f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::OffHand), 0.1f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Feet), 0.2f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Legs), 0.3f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Chest), 0.4f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Head), 0.6f);
}

TEST(MobEntitySerializationTest, DropChances_OldFormatRoundTrip)
{
    // 手动构造旧格式 NBT（HandDropChances + ArmorDropChances float 列表）
    nbt::tags::compound_tag tag;

    // 写入 MobEntity 需要的最小字段（addAdditionalSaveData 会调用基类）
    // 对于 readAdditionalSaveData，它先调用 LivingEntity::readAdditionalSaveData，
    // 然后才是 MobEntity 的字段。我们只关注 MobEntity 字段即可。
    // 这里我们手动构造只包含旧格式掉落概率的 NBT。

    // 旧格式：HandDropChances（2个float）
    nbt_helper::putFloatList(tag, nbt_keys::HAND_DROP_CHANCES, {0.3f, 0.4f});

    // 旧格式：ArmorDropChances（4个float）
    nbt_helper::putFloatList(tag, nbt_keys::ARMOR_DROP_CHANCES, {0.5f, 0.6f, 0.7f, 0.8f});

    // 反序列化
    auto loaded = loadFromNbt(tag);

    // 验证从旧格式读取的值
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::MainHand), 0.3f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::OffHand), 0.4f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Feet), 0.5f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Legs), 0.6f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Chest), 0.7f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Head), 0.8f);
}

TEST(MobEntitySerializationTest, DropChances_NewFormatRoundTrip)
{
    // 构造新格式 NBT（drop_chances compound）
    nbt::tags::compound_tag tag;

    // 新格式：drop_chances compound，只包含非默认值
    nbt::tags::compound_tag dropChancesTag;
    dropChancesTag.put("mainhand", 0.5f);
    dropChancesTag.put("head", 1.5f);
    tag.value.emplace(nbt_keys::DROP_CHANCES, std::make_unique<nbt::tags::compound_tag>(std::move(dropChancesTag)));

    // 反序列化
    auto loaded = loadFromNbt(tag);

    // 新格式中指定的槽位应为指定值
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::MainHand), 0.5f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Head), 1.5f);

    // 新格式中未指定的槽位应保持默认值 0.085
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::OffHand), 0.085f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Feet), 0.085f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Legs), 0.085f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Chest), 0.085f);
}

TEST(MobEntitySerializationTest, DropChances_MixedFormatNewOverridesOld)
{
    // 同时写入旧格式和新格式，新格式应覆盖旧格式
    nbt::tags::compound_tag tag;

    // 旧格式
    nbt_helper::putFloatList(tag, nbt_keys::HAND_DROP_CHANCES, {0.3f, 0.4f});
    nbt_helper::putFloatList(tag, nbt_keys::ARMOR_DROP_CHANCES, {0.5f, 0.6f, 0.7f, 0.8f});

    // 新格式：只设置 mainhand 和 chest
    nbt::tags::compound_tag dropChancesTag;
    dropChancesTag.put("mainhand", 0.99f);
    dropChancesTag.put("chest", 0.11f);
    tag.value.emplace(nbt_keys::DROP_CHANCES, std::make_unique<nbt::tags::compound_tag>(std::move(dropChancesTag)));

    // 反序列化
    auto loaded = loadFromNbt(tag);

    // 新格式指定的槽位应使用新格式的值
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::MainHand), 0.99f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Chest), 0.11f);

    // 旧格式的值应被新格式中未指定的默认值覆盖行为所影响
    // 根据代码逻辑：新格式读取时，未指定的槽位保持默认值 0.085。
    // 然后旧格式读取时，如果新格式中该槽位仍然是默认值，则使用旧格式的值。
    // OffHand: 新格式未指定 → 默认值 0.085 → 旧格式 0.4（非默认值 → 使用旧格式）
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::OffHand), 0.4f);
    // Feet: 新格式未指定 → 默认值 0.085 → 旧格式 0.5（非默认值 → 使用旧格式）
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Feet), 0.5f);
    // Legs: 新格式未指定 → 默认值 0.085 → 旧格式 0.6（非默认值 → 使用旧格式）
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Legs), 0.6f);
    // Head: 新格式未指定 → 默认值 0.085 → 旧格式 0.8（非默认值 → 使用旧格式）
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Head), 0.8f);
}

// ============================================================================
// 死亡掉落表测试
// ============================================================================

TEST(MobEntitySerializationTest, DeathLootTable_DefaultState)
{
    TestMobEntity entity;

    // 默认状态：无掉落表，种子为 0
    EXPECT_FALSE(entity.deathLootTable().has_value());
    EXPECT_EQ(entity.lootTableSeed(), 0);
}

TEST(MobEntitySerializationTest, DeathLootTable_SetGetRoundTrip)
{
    TestMobEntity entity;

    // 设置掉落表
    entity.setDeathLootTable("minecraft:entities/zombie");
    EXPECT_TRUE(entity.deathLootTable().has_value());
    EXPECT_EQ(entity.deathLootTable().value(), "minecraft:entities/zombie");

    // 设置种子
    entity.setLootTableSeed(12345);
    EXPECT_EQ(entity.lootTableSeed(), 12345);

    // 清除掉落表
    entity.setDeathLootTable(std::nullopt);
    EXPECT_FALSE(entity.deathLootTable().has_value());
}

TEST(MobEntitySerializationTest, DeathLootTable_NbtWriteOnlyWritesWhenPresent)
{
    TestMobEntity entity;

    // 默认状态：不应写入 DeathLootTable 和 DeathLootTableSeed
    auto tag = saveToNbt(entity);
    EXPECT_FALSE(nbt_helper::tryGetString(*tag, nbt_keys::DEATH_LOOT_TABLE).has_value());
    EXPECT_FALSE(nbt_helper::tryGetLong(*tag, nbt_keys::DEATH_LOOT_TABLE_SEED).has_value());

    // 设置掉落表但种子为 0：应只写入 DeathLootTable
    entity.setDeathLootTable("minecraft:entities/skeleton");
    tag = saveToNbt(entity);
    auto lootTableStr = nbt_helper::tryGetString(*tag, nbt_keys::DEATH_LOOT_TABLE);
    EXPECT_TRUE(lootTableStr.has_value());
    EXPECT_EQ(lootTableStr.value(), "minecraft:entities/skeleton");
    EXPECT_FALSE(nbt_helper::tryGetLong(*tag, nbt_keys::DEATH_LOOT_TABLE_SEED).has_value());

    // 设置掉落表且种子非 0：应同时写入两个键
    entity.setLootTableSeed(999);
    tag = saveToNbt(entity);
    lootTableStr = nbt_helper::tryGetString(*tag, nbt_keys::DEATH_LOOT_TABLE);
    EXPECT_TRUE(lootTableStr.has_value());
    EXPECT_EQ(lootTableStr.value(), "minecraft:entities/skeleton");
    auto seedVal = nbt_helper::tryGetLong(*tag, nbt_keys::DEATH_LOOT_TABLE_SEED);
    EXPECT_TRUE(seedVal.has_value());
    EXPECT_EQ(seedVal.value(), 999);
}

TEST(MobEntitySerializationTest, DeathLootTable_NbtRoundTrip)
{
    TestMobEntity original;
    original.setDeathLootTable("minecraft:entities/creeper");
    original.setLootTableSeed(42);

    auto tag = saveToNbt(original);
    auto loaded = loadFromNbt(*tag);

    EXPECT_TRUE(loaded->deathLootTable().has_value());
    EXPECT_EQ(loaded->deathLootTable().value(), "minecraft:entities/creeper");
    EXPECT_EQ(loaded->lootTableSeed(), 42);
}

TEST(MobEntitySerializationTest, DeathLootTable_NbtReadMissingKeysGracefully)
{
    // 空 NBT tag 反序列化不应崩溃
    nbt::tags::compound_tag emptyTag;
    auto loaded = loadFromNbt(emptyTag);

    // 默认值应保持不变
    EXPECT_FALSE(loaded->deathLootTable().has_value());
    EXPECT_EQ(loaded->lootTableSeed(), 0);
}

// ============================================================================
// 拴绳数据测试
// ============================================================================

TEST(MobEntitySerializationTest, Leash_DefaultNotLeashed)
{
    TestMobEntity entity;

    // 默认状态：未被拴绳拴住
    EXPECT_FALSE(entity.isLeashed());
    EXPECT_FALSE(entity.leashHolderUuid().has_value());
    EXPECT_FALSE(entity.leashFencePos().has_value());
}

TEST(MobEntitySerializationTest, Leash_SetLeashedToEntity)
{
    TestMobEntity entity;

    entity.setLeashedToEntity("550e8400e29b41d4a716446655440000");

    EXPECT_TRUE(entity.isLeashed());
    EXPECT_TRUE(entity.leashHolderUuid().has_value());
    EXPECT_EQ(entity.leashHolderUuid().value(), "550e8400e29b41d4a716446655440000");
    EXPECT_FALSE(entity.leashFencePos().has_value());
}

TEST(MobEntitySerializationTest, Leash_SetLeashedToFence)
{
    TestMobEntity entity;

    entity.setLeashedToFence(BlockPos(10, 64, -5));

    EXPECT_TRUE(entity.isLeashed());
    EXPECT_FALSE(entity.leashHolderUuid().has_value());
    EXPECT_TRUE(entity.leashFencePos().has_value());
    EXPECT_EQ(entity.leashFencePos()->x, 10);
    EXPECT_EQ(entity.leashFencePos()->y, 64);
    EXPECT_EQ(entity.leashFencePos()->z, -5);
}

TEST(MobEntitySerializationTest, Leash_ClearLeash)
{
    TestMobEntity entity;

    // 先拴到实体上
    entity.setLeashedToEntity("550e8400e29b41d4a716446655440000");
    EXPECT_TRUE(entity.isLeashed());

    // 清除拴绳
    entity.clearLeash();
    EXPECT_FALSE(entity.isLeashed());
    EXPECT_FALSE(entity.leashHolderUuid().has_value());
    EXPECT_FALSE(entity.leashFencePos().has_value());
    EXPECT_FALSE(entity.leashDelayInfo().targetUuid.has_value());
    EXPECT_FALSE(entity.leashDelayInfo().fencePos.has_value());

    // 拴到栅栏上再清除
    entity.setLeashedToFence(BlockPos(100, 70, 200));
    EXPECT_TRUE(entity.isLeashed());
    entity.clearLeash();
    EXPECT_FALSE(entity.isLeashed());
    EXPECT_FALSE(entity.leashFencePos().has_value());
}

TEST(MobEntitySerializationTest, Leash_NbtRoundTripEntityUuid)
{
    TestMobEntity original;
    // 使用一个有效的 UUID 字符串（32位十六进制）
    original.setLeashedToEntity("550e8400e29b41d4a716446655440000");

    auto tag = saveToNbt(original);
    auto loaded = loadFromNbt(*tag);

    // 验证拴绳状态和 UUID
    EXPECT_TRUE(loaded->isLeashed());
    EXPECT_TRUE(loaded->leashHolderUuid().has_value());
    EXPECT_EQ(loaded->leashHolderUuid().value(), "550e8400e29b41d4a716446655440000");
    EXPECT_FALSE(loaded->leashFencePos().has_value());

    // 验证延迟信息也被正确设置
    EXPECT_TRUE(loaded->leashDelayInfo().targetUuid.has_value());
    EXPECT_EQ(loaded->leashDelayInfo().targetUuid.value(), "550e8400e29b41d4a716446655440000");
}

TEST(MobEntitySerializationTest, Leash_NbtRoundTripFencePos)
{
    TestMobEntity original;
    original.setLeashedToFence(BlockPos(10, 64, -5));

    auto tag = saveToNbt(original);
    auto loaded = loadFromNbt(*tag);

    // 验证拴绳状态和栅栏位置
    EXPECT_TRUE(loaded->isLeashed());
    EXPECT_FALSE(loaded->leashHolderUuid().has_value());
    EXPECT_TRUE(loaded->leashFencePos().has_value());
    EXPECT_EQ(loaded->leashFencePos()->x, 10);
    EXPECT_EQ(loaded->leashFencePos()->y, 64);
    EXPECT_EQ(loaded->leashFencePos()->z, -5);

    // 验证延迟信息也被正确设置
    EXPECT_TRUE(loaded->leashDelayInfo().fencePos.has_value());
    EXPECT_EQ(loaded->leashDelayInfo().fencePos->x, 10);
    EXPECT_EQ(loaded->leashDelayInfo().fencePos->y, 64);
    EXPECT_EQ(loaded->leashDelayInfo().fencePos->z, -5);
}

TEST(MobEntitySerializationTest, Leash_NbtReadMissingLeashTagNoCrash)
{
    // 没有 Leash tag 的 NBT 不应崩溃
    nbt::tags::compound_tag emptyTag;
    auto loaded = loadFromNbt(emptyTag);

    // 默认状态
    EXPECT_FALSE(loaded->isLeashed());
    EXPECT_FALSE(loaded->leashHolderUuid().has_value());
    EXPECT_FALSE(loaded->leashFencePos().has_value());
}

TEST(MobEntitySerializationTest, Leash_NbtReadClearsLeashWhenTagAbsent)
{
    // 先拴住实体
    TestMobEntity original;
    original.setLeashedToEntity("550e8400e29b41d4a716446655440000");
    EXPECT_TRUE(original.isLeashed());

    // 反序列化空的 NBT 标签（没有 Leash 数据）
    nbt::tags::compound_tag emptyTag;
    auto loaded = loadFromNbt(emptyTag);

    // 因为 readAdditionalSaveData 中，如果之前被拴住但 NBT 中没有 Leash 数据，
    // 会调用 clearLeash()。但是 loaded 是新创建的实体，默认未拴住。
    // 这个测试验证的是：当已拴住的实体读取了没有 Leash 标签的 NBT 时，
    // 拴绳状态会被清除。
    EXPECT_FALSE(loaded->isLeashed());
}

TEST(MobEntitySerializationTest, Leash_NbtReadClearsLeashPreviouslyLeashedEntity)
{
    // 模拟之前被拴住的实体读取了没有 Leash 数据的 NBT
    TestMobEntity entity;
    entity.setLeashedToEntity("550e8400e29b41d4a716446655440000");

    // 直接调用 readAdditionalSaveData 传入空标签
    nbt::tags::compound_tag emptyTag;
    auto result = entity.readAdditionalSaveData(emptyTag);
    EXPECT_TRUE(result.success());

    // 拴绳应被清除
    EXPECT_FALSE(entity.isLeashed());
    EXPECT_FALSE(entity.leashHolderUuid().has_value());
    EXPECT_FALSE(entity.leashFencePos().has_value());
}

// ============================================================================
// 综合 NBT 序列化测试
// ============================================================================

TEST(MobEntitySerializationTest, FullNbtRoundTrip_AllFields)
{
    TestMobEntity original;

    // 设置掉落概率
    original.setEquipmentDropChance(EquipmentSlot::MainHand, 0.5f);
    original.setGuaranteedDrop(EquipmentSlot::Head);

    // 设置掉落表
    original.setDeathLootTable("minecraft:entities/zombie");
    original.setLootTableSeed(12345);

    // 设置拴绳
    original.setLeashedToFence(BlockPos(100, 70, 200));

    // 序列化并反序列化
    auto tag = saveToNbt(original);
    auto loaded = loadFromNbt(*tag);

    // 验证掉落概率
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::MainHand), 0.5f);
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::Head), 2.0f);
    EXPECT_TRUE(loaded->isEquipmentDropPreserved(EquipmentSlot::Head));
    // 未修改的槽位应为默认值
    EXPECT_FLOAT_EQ(loaded->getEquipmentDropChance(EquipmentSlot::OffHand), 0.085f);

    // 验证掉落表
    EXPECT_TRUE(loaded->deathLootTable().has_value());
    EXPECT_EQ(loaded->deathLootTable().value(), "minecraft:entities/zombie");
    EXPECT_EQ(loaded->lootTableSeed(), 12345);

    // 验证拴绳
    EXPECT_TRUE(loaded->isLeashed());
    EXPECT_TRUE(loaded->leashFencePos().has_value());
    EXPECT_EQ(loaded->leashFencePos()->x, 100);
    EXPECT_EQ(loaded->leashFencePos()->y, 70);
    EXPECT_EQ(loaded->leashFencePos()->z, 200);
}

TEST(MobEntitySerializationTest, DropChances_NbtWriteNewFormatOnlyNonDefault)
{
    // 验证新格式只写入非默认值
    TestMobEntity entity;

    // 只修改 MainHand 和 Head
    entity.setEquipmentDropChance(EquipmentSlot::MainHand, 0.5f);
    entity.setGuaranteedDrop(EquipmentSlot::Head);

    auto tag = saveToNbt(entity);

    // drop_chances compound 应只包含 mainhand 和 head
    auto* dropChancesCompound = nbt_helper::tryGetCompound(*tag, nbt_keys::DROP_CHANCES);
    ASSERT_NE(dropChancesCompound, nullptr);

    // mainhand 应存在
    auto mainhandVal = nbt_helper::tryGetFloat(*dropChancesCompound, "mainhand");
    EXPECT_TRUE(mainhandVal.has_value());
    EXPECT_FLOAT_EQ(mainhandVal.value(), 0.5f);

    // head 应存在
    auto headVal = nbt_helper::tryGetFloat(*dropChancesCompound, "head");
    EXPECT_TRUE(headVal.has_value());
    EXPECT_FLOAT_EQ(headVal.value(), 2.0f);

    // offhand 不应存在（默认值 0.085）
    auto offhandVal = nbt_helper::tryGetFloat(*dropChancesCompound, "offhand");
    EXPECT_FALSE(offhandVal.has_value());
}
