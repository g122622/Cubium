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

// 黄金表测试：实体同步数据(SynchedEntityData)字段 id 对齐 vanilla 1.21.11。
//
// 背景：项目复刻 vanilla ClassTreeIdRegistry，每个实体类的 registerData() 内用
// ClassRegisterGuard 沿继承链分配 synched-data id。基类字段 id 在所有子类共享，
// 子类字段从基类最高 id+1 起连续编号。本测试构造各实体实例触发 registerData，
// 断言其 EntityDataManager.getAllEntries() 的 id 集合与 vanilla 1.21.11 逐字段一致，
// 防止继承链分配器回归（如叶子类漏 ClassRegisterGuard 致字段走兜底路径从 id 0 起，
// 覆盖 Entity id0-7，使真 Java 客户端 set_entity_data 类型校验崩溃）。
//
// id 基线（vanilla 1.21.11，按 defineId declare 顺序）：
//   Entity              id0..7   FLAGS/AIR/CUSTOM_NAME/CUSTOM_NAME_VISIBLE/SILENT/NO_GRAVITY/POSE/TICKS_FROZEN
//   LivingEntity        id8..14
//   LIVING_FLAGS/HEALTH/EFFECT_PARTICLES/EFFECT_AMBIENCE/ARROW_COUNT/STINGER_COUNT/SLEEPING_POS MobEntity id15
//   MOB_FLAGS Player(扁平化)      id15..20 Avatar id15-16(MAIN_HAND/MODE_CUSTOMISATION) + Player
//   id17-20(ABSORPTION/SCORE/SHOULDER_PARROT_L/SHOULDER_PARROT_R) FallingBlockEntity  id8 DATA_START_POS(BlockPos)
//   （直接继承 Entity；BlockState 走 AddEntity.data 非 SynchedEntityData） TNTEntity id8..9
//   DATA_FUSE/DATA_BLOCK_STATE(BLOCK_STATE id14) （直接继承 Entity） FishingBobberEntity id8..9
//   DATA_HOOKED_ENTITY/DATA_BITING（直接继承 Entity） AbstractMinecartEntity id8..13
//   ROLLING_AMPLITUDE/ROLLING_DIRECTION/DAMAGE/DISPLAY_TILE/DISPLAY_TILE_OFFSET/SHOW_BLOCK（直接继承 Entity）

#include "entity/core/AgeableEntity.hpp"
#include "entity/core/DataParameter.hpp"
#include "entity/core/Entity.hpp"
#include "entity/core/EntityDataManager.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/core/MobEntity.hpp"
#include "entity/entities/misc/MiscEntities.hpp"
#include "entity/entities/monster/illager/AbstractRaiderEntity.hpp"
#include "entity/entities/monster/illager/IllagerEntities.hpp"
#include "entity/entities/monster/undead/DrownedEntity.hpp"
#include "entity/entities/monster/undead/HuskEntity.hpp"
#include "entity/entities/monster/undead/ZombieEntity.hpp"
#include "entity/entities/monster/undead/ZombieVillagerEntity.hpp"
#include "entity/entities/passive/fish/AbstractFishEntity.hpp"
#include "entity/entities/passive/fish/CodEntity.hpp"
#include "entity/entities/passive/fish/PufferfishEntity.hpp"
#include "entity/entities/passive/fish/SalmonEntity.hpp"
#include "entity/entities/passive/fish/TropicalFishEntity.hpp"
#include "entity/entities/passive/nautilus/NautilusEntity.hpp"
#include "entity/entities/passive/tamable/CatEntity.hpp"
#include "entity/entities/passive/tamable/WolfEntity.hpp"
#include "entity/entities/passive/water/GlowSquidEntity.hpp"
#include "entity/entities/passive/water/SquidEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/entities/projectile/OtherProjectiles.hpp"
#include "entity/entities/vehicle/MinecartEntity.hpp"
#include "network/codec/EntityMetadataSerializer.hpp"

#include <set>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;
using namespace mc::entity;

namespace {

// 收集某实体 dataManager 中所有已注册参数 id（即 getAllEntries 的 key 集合）。
// 这些 id 由 registerData 沿继承链分配，应与 vanilla 1.21.11 逐字段一致。
[[nodiscard]] std::set<u16> collectParamIds(const EntityDataManager& manager)
{
    std::set<u16> ids;
    for (const auto& [id, entry] : manager.getAllEntries()) {
        (void)entry;
        ids.insert(id);
    }
    return ids;
}

// 构造预期 id 集合 [lo, hi] 闭区间。
[[nodiscard]] std::set<u16> expectedRange(u16 lo, u16 hi)
{
    std::set<u16> ids;
    for (u16 i = lo; i <= hi; ++i) {
        ids.insert(i);
    }
    return ids;
}

} // namespace

// ============================================================================
// 基类 id 链
// ============================================================================

TEST(EntityMetadataIdGoldenTest, EntityHasIds0To7)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    entity.registerData();
    EXPECT_EQ(collectParamIds(entity.dataManager()), expectedRange(0, 7));
}

// 字段→id→variant 类型三者一致性（防顺序回归）。
// vanilla 1.21.11 Entity 各 EntityDataAccessor 的 defineId declare 顺序决定 id
// （ClassTreeIdRegistry.define 按 declare 顺序分配，非 define() 默认值顺序）：
//   id0 FLAGS(Byte)/id1 AIR(Int)/id2 CUSTOM_NAME(Optional<Component>)/
//   id3 CUSTOM_NAME_VISIBLE(Boolean)/id4 SILENT(Boolean)/id5 NO_GRAVITY(Boolean)/
//   id6 POSE(Pose)/id7 TICKS_FROZEN(Int)
// registerParam 调用顺序必须与此一致，否则 wire index 与字段 serializerId 错位，
// 真客户端 set_entity_data 类型校验崩溃（如 id2 发 Boolean 而客户端按 Optional 校验）。
// DataValue variant index：0=i8(Byte) 1=i32(Int) 5=bool(Boolean)
//   10=PoseValue(Pose) 11=OptionalComponentValue(OptionalComponent)
TEST(EntityMetadataIdGoldenTest, EntityFieldIdAndTypeAlignVanilla)
{
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    entity.registerData();
    const auto& mgr = entity.dataManager();

    ASSERT_NE(mgr.getRaw(0), nullptr);
    EXPECT_EQ(mgr.getRaw(0)->index(), 0u); // FLAGS → i8/Byte
    ASSERT_NE(mgr.getRaw(1), nullptr);
    EXPECT_EQ(mgr.getRaw(1)->index(), 1u); // AIR → i32/Int
    ASSERT_NE(mgr.getRaw(2), nullptr);
    EXPECT_EQ(mgr.getRaw(2)->index(), 11u); // CUSTOM_NAME → OptionalComponentValue（必为 id2）
    ASSERT_NE(mgr.getRaw(3), nullptr);
    EXPECT_EQ(mgr.getRaw(3)->index(), 5u); // CUSTOM_NAME_VISIBLE → bool/Boolean（必为 id3）
    ASSERT_NE(mgr.getRaw(4), nullptr);
    EXPECT_EQ(mgr.getRaw(4)->index(), 5u); // SILENT → bool/Boolean
    ASSERT_NE(mgr.getRaw(5), nullptr);
    EXPECT_EQ(mgr.getRaw(5)->index(), 5u); // NO_GRAVITY → bool/Boolean
    ASSERT_NE(mgr.getRaw(6), nullptr);
    EXPECT_EQ(mgr.getRaw(6)->index(), 10u); // POSE → PoseValue
    ASSERT_NE(mgr.getRaw(7), nullptr);
    EXPECT_EQ(mgr.getRaw(7)->index(), 1u); // TICKS_FROZEN → i32/Int
}

TEST(EntityMetadataIdGoldenTest, LivingEntityHasIds0To14)
{
    LivingEntity living(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    living.registerData();
    EXPECT_EQ(collectParamIds(living.dataManager()), expectedRange(0, 14));
}

TEST(EntityMetadataIdGoldenTest, MobEntityHasIds0To15)
{
    MobEntity mob(EntityInstanceId(1), mc::test::testEcsRegistry());
    mob.registerData();
    EXPECT_EQ(collectParamIds(mob.dataManager()), expectedRange(0, 15));
}

// ============================================================================
// Player 扁平化（Avatar 字段内联进 Player，classInfo parent=LivingEntity）
// vanilla Avatar(id15-16) + Player(id17-20)，共 21 字段 id0..20。
// ============================================================================

TEST(EntityMetadataIdGoldenTest, PlayerHasIds0To20)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    EXPECT_EQ(collectParamIds(player.dataManager()), expectedRange(0, 20));
    // 第 21 个槽位不应存在（Player 仅 21 字段，id 最大 20）。
    EXPECT_FALSE(player.dataManager().hasParam(21));
}

// ============================================================================
// 直接继承 Entity 的叶子类（字段从 id8 起）
// ============================================================================

TEST(EntityMetadataIdGoldenTest, FallingBlockEntityHasIds0To8)
{
    FallingBlockEntity entity{mc::test::testEcsRegistry()};
    // FallingBlockEntity 仅 1 个自身字段 DATA_START_POS(BlockPos,id8)，加 Entity id0..7。
    // 对齐 vanilla 1.21.11：BlockState 不走 SynchedEntityData，经 AddEntity.data 下发。
    EXPECT_EQ(collectParamIds(entity.dataManager()), expectedRange(0, 8));
    // field8 必为 Vector3i（variant index 6 → BLOCK_POS 序列化器 id10）。
    // 真 Java 客户端严格校验 field8 类型，旧实现误发 Int(stateId) 致崩溃。
    ASSERT_NE(entity.dataManager().getRaw(FallingBlockEntity::getStartPosParamId()), nullptr);
    EXPECT_EQ(entity.dataManager().getRaw(FallingBlockEntity::getStartPosParamId())->index(), 6u);
}

TEST(EntityMetadataIdGoldenTest, TNTEntityHasIds0To9)
{
    TNTEntity entity{mc::test::testEcsRegistry()};
    // DATA_FUSE(id8,Int) + DATA_BLOCK_STATE(id9,BlockStateValue→BLOCK_STATE id14) + Entity id0..7。
    EXPECT_EQ(collectParamIds(entity.dataManager()), expectedRange(0, 9));
    // field8 = Int（variant index 1）。
    ASSERT_NE(entity.dataManager().getRaw(TNTEntity::getFuseParamId()), nullptr);
    EXPECT_EQ(entity.dataManager().getRaw(TNTEntity::getFuseParamId())->index(), 1u);
    // field9 必为 BlockStateValue（variant index 15 → BLOCK_STATE 序列化器 id14）。
    // 旧实现误发 Int(stateId) 致真客户端 field9 类型校验崩溃。
    ASSERT_NE(entity.dataManager().getRaw(TNTEntity::getBlockStateParamId()), nullptr);
    EXPECT_EQ(entity.dataManager().getRaw(TNTEntity::getBlockStateParamId())->index(), 15u);
}

TEST(EntityMetadataIdGoldenTest, FishingBobberEntityHasIds0To9)
{
    FishingBobberEntity entity(EntityInstanceId(1), mc::test::testEcsRegistry());
    // DATA_HOOKED_ENTITY(id8) + DATA_BITING(id9) + Entity id0..7。
    EXPECT_EQ(collectParamIds(entity.dataManager()), expectedRange(0, 9));
}

TEST(EntityMetadataIdGoldenTest, AbstractMinecartEntityHasIds0To12)
{
    AbstractMinecartEntity entity(AbstractMinecartEntity::Type::Rideable, mc::test::testEcsRegistry());
    // 对齐 vanilla 1.21.11：Entity id0..7 + 5 字段 id8..12（HURT/HURTDIR/DAMAGE/CUSTOM_DISPLAY_BLOCK/DISPLAY_OFFSET）。
    // 旧实现 6 字段 id8..13 含 rolling/show_block ghost 字段,且 display_tile 误用 i32 致真客户端
    // field11 类型校验崩(Optional<BlockState> vs Int);删 ghost + 改 OptionalBlockState 修复。
    EXPECT_EQ(collectParamIds(entity.dataManager()), expectedRange(0, 12));
    EXPECT_FALSE(entity.dataManager().hasParam(13));

    // wire11 CUSTOM_DISPLAY_BLOCK 必为 OptionalBlockStateValue（variant index 14 → serializerId 15
    // OPTIONAL_BLOCK_STATE）。
    const u16 displayBlockId = AbstractMinecartEntity::getCustomDisplayBlockParam().id();
    EXPECT_EQ(displayBlockId, 11u);
    ASSERT_NE(entity.dataManager().getRaw(displayBlockId), nullptr);
    EXPECT_EQ(entity.dataManager().getRaw(displayBlockId)->index(), 14u); // OptionalBlockStateValue

    // wire10 DAMAGE 必为 f32/Float（variant index 3 → serializerId 3 FLOAT）。
    const u16 damageId = AbstractMinecartEntity::getDamageParam().id();
    EXPECT_EQ(damageId, 10u);
    ASSERT_NE(entity.dataManager().getRaw(damageId), nullptr);
    EXPECT_EQ(entity.dataManager().getRaw(damageId)->index(), 3u); // f32/Float
}

// ============================================================================
// 链3: Zombie 三字段（vanilla 1.21.11 Zombie 在 Mob id15 之后注册三字段）
//   Zombie id16 DATA_BABY(Boolean) / id17 DATA_SPECIAL_TYPE(Int) / id18 DATA_DROWNED_CONVERSION(Boolean)
// Husk/Drowned 透传层无自身字段，id 集合与 Zombie 一致(id0..18)。
// ZombieVillager 在 Zombie 之上再加 4 字段(id19..22)：CONVERTING/VILLAGER_TYPE/VILLAGER_PROFESSION/VILLAGER_LEVEL。
// 此链对齐直接关系真客户端 set_entity_data 类型校验：若 Zombie 缺这三字段，
// ZombieVillager 的 CONVERTING(Boolean) 会错占 id16，撞上客户端期望的 DATA_BABY(Boolean) 虽类型
// 恰同但语义错位，更危险的是经此链的实体 id 全体前移致与 vanilla 不一致。
// ============================================================================

TEST(EntityMetadataIdGoldenTest, ZombieEntityHasIds0To18)
{
    ZombieEntity zombie(EntityInstanceId(1), mc::test::testEcsRegistry());
    // Entity 0..7 + LivingEntity 8..14 + Mob 15 + Zombie 16..18。
    EXPECT_EQ(collectParamIds(zombie.dataManager()), expectedRange(0, 18));
    // 第 19 个槽位不应存在。
    EXPECT_FALSE(zombie.dataManager().hasParam(19));
}

// Zombie 三字段 id 与类型逐字段锁定（防顺序/类型回归）。
// DataValue variant index：1=i32(Int) 5=bool(Boolean)
TEST(EntityMetadataIdGoldenTest, ZombieEntityFieldIdAndTypeAlignVanilla)
{
    ZombieEntity zombie(EntityInstanceId(1), mc::test::testEcsRegistry());
    const auto& mgr = zombie.dataManager();

    const u16 babyId = ZombieEntity::getBabyParamId();
    const u16 specialTypeId = ZombieEntity::getSpecialTypeParamId();
    const u16 drownedId = ZombieEntity::getDrownedConversionParamId();

    EXPECT_EQ(babyId, 16u);
    EXPECT_EQ(specialTypeId, 17u);
    EXPECT_EQ(drownedId, 18u);

    ASSERT_NE(mgr.getRaw(babyId), nullptr);
    EXPECT_EQ(mgr.getRaw(babyId)->index(), 5u); // DATA_BABY → bool/Boolean
    ASSERT_NE(mgr.getRaw(specialTypeId), nullptr);
    EXPECT_EQ(mgr.getRaw(specialTypeId)->index(), 1u); // DATA_SPECIAL_TYPE → i32/Int
    ASSERT_NE(mgr.getRaw(drownedId), nullptr);
    EXPECT_EQ(mgr.getRaw(drownedId)->index(), 5u); // DATA_DROWNED_CONVERSION → bool/Boolean
}

TEST(EntityMetadataIdGoldenTest, HuskEntityHasIds0To18)
{
    HuskEntity husk(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 透传层无自身字段，与 Zombie 一致。
    EXPECT_EQ(collectParamIds(husk.dataManager()), expectedRange(0, 18));
}

TEST(EntityMetadataIdGoldenTest, DrownedEntityHasIds0To18)
{
    DrownedEntity drowned(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 透传层无自身字段，与 Zombie 一致。
    EXPECT_EQ(collectParamIds(drowned.dataManager()), expectedRange(0, 18));
}

TEST(EntityMetadataIdGoldenTest, ZombieVillagerEntityHasIds0To20)
{
    ZombieVillagerEntity zv(EntityInstanceId(1), mc::test::testEcsRegistry());
    // Zombie 0..18 + ZombieVillager 19..20（CONVERTING + 单一复合 VILLAGER_DATA）。
    // 旧实现拆 3 个独立 i32(type/profession/level)致 id 19..22 多 2 槽且类型不符 vanilla
    // DATA_VILLAGER_DATA(VillagerData),已收敛为单一 VillagerDataValue 复合字段(id20)。
    EXPECT_EQ(collectParamIds(zv.dataManager()), expectedRange(0, 20));
    EXPECT_FALSE(zv.dataManager().hasParam(21));

    // wire20 VILLAGER_DATA 必为 VillagerDataValue（variant index 19 → serializerId 18 VILLAGER_DATA）。
    const u16 vdId = ZombieVillagerEntity::getVillagerDataParamId();
    EXPECT_EQ(vdId, 20u);
    ASSERT_NE(zv.dataManager().getRaw(vdId), nullptr);
    EXPECT_EQ(zv.dataManager().getRaw(vdId)->index(), 19u); // VillagerDataValue
}

// ============================================================================
// 链2: AbstractFish.FROM_BUCKET（vanilla 1.21.11 鱼链不经 AgeableMob，FROM_BUCKET 在 Mob id15 之后）
//   AbstractFish id16 FROM_BUCKET(Boolean)
//   Pufferfish id17 DATA_PUFF_STATE(Int)（经 AbstractFish）
//   Cod 透传层无自身字段，id 集合与 AbstractFish 一致(id0..16)
//   Salmon id17 DATA_TYPE(Int，体型 small/medium/large)
//   TropicalFish id17 DATA_VARIANT(Int，packed shape|baseColor<<8|patternColor<<16)
// 此链对齐关系真客户端 set_entity_data：FROM_BUCKET 是 Boolean(id16)，与 AgeableMob.DATA_BABY
// 在不同类树分支，各自独立编号不冲突。
// ============================================================================

TEST(EntityMetadataIdGoldenTest, AbstractFishEntityHasIds0To16)
{
    AbstractFishEntity fish(EntityInstanceId(1), mc::test::testEcsRegistry());
    // Entity 0..7 + LivingEntity 8..14 + Mob 15 + AbstractFish 16(FROM_BUCKET)。
    EXPECT_EQ(collectParamIds(fish.dataManager()), expectedRange(0, 16));
    EXPECT_FALSE(fish.dataManager().hasParam(17));
}

TEST(EntityMetadataIdGoldenTest, AbstractFishFromBucketFieldIdAndTypeAlignVanilla)
{
    AbstractFishEntity fish(EntityInstanceId(1), mc::test::testEcsRegistry());
    const auto& mgr = fish.dataManager();

    const u16 fromBucketId = AbstractFishEntity::getFromBucketParamId();
    EXPECT_EQ(fromBucketId, 16u);
    ASSERT_NE(mgr.getRaw(fromBucketId), nullptr);
    EXPECT_EQ(mgr.getRaw(fromBucketId)->index(), 5u); // FROM_BUCKET → bool/Boolean
}

TEST(EntityMetadataIdGoldenTest, PufferfishEntityHasIds0To17)
{
    PufferfishEntity puffer(EntityInstanceId(1), mc::test::testEcsRegistry());
    // AbstractFish 0..16 + Pufferfish 17(DATA_PUFF_STATE)。
    EXPECT_EQ(collectParamIds(puffer.dataManager()), expectedRange(0, 17));
    EXPECT_FALSE(puffer.dataManager().hasParam(18));
}

TEST(EntityMetadataIdGoldenTest, CodEntityHasIds0To16)
{
    CodEntity cod(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 透传层无自身字段，与 AbstractFish 一致。
    EXPECT_EQ(collectParamIds(cod.dataManager()), expectedRange(0, 16));
}

TEST(EntityMetadataIdGoldenTest, SalmonEntityHasIds0To17)
{
    SalmonEntity salmon(EntityInstanceId(1), mc::test::testEcsRegistry());
    // AbstractFish 0..16 + Salmon 17(DATA_TYPE，体型 small/medium/large)，对齐 vanilla
    // 1.21.11 Salmon.DATA_TYPE(Int)。
    EXPECT_EQ(collectParamIds(salmon.dataManager()), expectedRange(0, 17));
    EXPECT_FALSE(salmon.dataManager().hasParam(18));
}

// TropicalFish 自带 DATA_VARIANT@17(Int，packed shape|baseColor<<8|patternColor<<16)，
// 对齐 vanilla 1.21.11 TropicalFish.DATA_ID_TYPE_VARIANT(Int)。构造时 randomizeVariant
// 写 m_variant 成员，不影响 registerParam 注册的 id 集合（id 由继承链分配，初始值 0）。
TEST(EntityMetadataIdGoldenTest, TropicalFishEntityHasIds0To17)
{
    TropicalFishEntity tropical(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(collectParamIds(tropical.dataManager()), expectedRange(0, 17));
    EXPECT_FALSE(tropical.dataManager().hasParam(18));
}

// ============================================================================
// 链1: AgeableMob.DATA_BABY → AgeableEntity 层（vanilla 1.21.11 AgeableMob 在 Mob id15 之后）
//   AgeableEntity id16 DATA_BABY(Boolean)
//   Wolf 经 Wolf→Tameable→Animal→AgeableEntity 链，DATA_TAMED(id17) 续接 DATA_BABY 之后
// 此链是 GlowSquid field16 崩溃的同源跳层模式：AgeableEntity 占位缺 DATA_BABY 致所有
// AgeableEntity 子类字段 id 比 vanilla 少 1。补齐后 DATA_BABY 占 id16，Wolf DATA_TAMED=17。
// AnimalEntity 空 override 串联调用链是核心陷阱修复点（见 AgeableEntity.hpp 注释）。
// ============================================================================

TEST(EntityMetadataIdGoldenTest, AgeableEntityHasIds0To16)
{
    AgeableEntity ageable(EntityInstanceId(1), mc::test::testEcsRegistry());
    // Entity 0..7 + LivingEntity 8..14 + Mob 15 + AgeableEntity 16(DATA_BABY)。
    EXPECT_EQ(collectParamIds(ageable.dataManager()), expectedRange(0, 16));
    EXPECT_FALSE(ageable.dataManager().hasParam(17));
}

TEST(EntityMetadataIdGoldenTest, AgeableEntityBabyFieldIdAndTypeAlignVanilla)
{
    AgeableEntity ageable(EntityInstanceId(1), mc::test::testEcsRegistry());
    const auto& mgr = ageable.dataManager();

    const u16 babyId = AgeableEntity::getBabyParamId();
    EXPECT_EQ(babyId, 16u);
    ASSERT_NE(mgr.getRaw(babyId), nullptr);
    EXPECT_EQ(mgr.getRaw(babyId)->index(), 5u); // DATA_BABY → bool/Boolean
}

// Wolf 验证经 AnimalEntity 空 override 串联到 AgeableEntity：DATA_FLAGS 应为 id17（DATA_BABY id16 之后）。
// 对齐 vanilla TamableAnimal：id17=DATA_FLAGS(Byte) + id18=DATA_OWNERUUID(OptionalLivingEntityRef)。
// Wolf 自身三字段(DATA_INTERESTED/DATA_COLLAR_COLOR/DATA_ANGER_TIME) → id19/20/21，共 id0..21。
TEST(EntityMetadataIdGoldenTest, WolfEntityHasIds0To21AndFlagsAt17)
{
    WolfEntity wolf(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(collectParamIds(wolf.dataManager()), expectedRange(0, 21));
    EXPECT_FALSE(wolf.dataManager().hasParam(22));

    // DATA_FLAGS 必为 id17（Byte，对齐 vanilla TamableAnimal.DATA_FLAGS_ID）。
    // getTamedParamId() 语义为「flags 字段 id」（保留旧名兼容调用方）。
    const u16 flagsId = WolfEntity::getTamedParamId();
    EXPECT_EQ(flagsId, 17u);
    ASSERT_NE(wolf.dataManager().getRaw(flagsId), nullptr);
    EXPECT_EQ(wolf.dataManager().getRaw(flagsId)->index(), 0u); // DATA_FLAGS → i8/Byte

    // DATA_OWNERUUID 必为 id18（OptionalUuidValue，对齐 vanilla DATA_OWNERUUID_ID）。
    const u16 ownerUuidId = WolfEntity::getOwnerUuidParamId();
    EXPECT_EQ(ownerUuidId, 18u);
    ASSERT_NE(wolf.dataManager().getRaw(ownerUuidId), nullptr);
    EXPECT_EQ(wolf.dataManager().getRaw(ownerUuidId)->index(), 21u); // OptionalUuidValue

    // DATA_ANGER_END_TIME 必为 id21 且类型为 i64/Long（variant index 2 → serializerId 2 VAR_LONG）。
    // vanilla Wolf.DATA_ANGER_END_TIME = Long(默认 -1L)；旧实现误用 i32(Int, serializerId 1) 致
    // 真 Java 客户端 set_entity_data field21 类型校验崩（disconnect-2026-08-04：old=-1(Long) new=0(Integer)）。
    const u16 angerId = WolfEntity::getAngerTimeParamId();
    EXPECT_EQ(angerId, 21u);
    ASSERT_NE(wolf.dataManager().getRaw(angerId), nullptr);
    EXPECT_EQ(wolf.dataManager().getRaw(angerId)->index(), 2u); // i64/Long（非 i32/Int 的 index 1）
}

// ============================================================================
// 链5b: Cat 经 Cat→TamableAnimal→Animal→AgeableEntity
//   对齐 vanilla Cat.defineSynchedData 4 字段(id19..22)：
//     DATA_VARIANT_ID(19,Holder<CatVariant> serializerId 21) / IS_LYING(20,Boolean)
//     RELAX_STATE_ONE(21,Boolean) / DATA_COLLAR_COLOR(22,Int 默认 14)
//   旧实现仅注册 lying/relax 两字段,致 wire 19 错发 Boolean,真客户端期望 CatVariant(serializerId 21)
//   类型校验崩;补 variant(HolderVariantValue→21 恰为 CAT_VARIANT)/collar 两字段并对齐顺序修复。
// ============================================================================
TEST(EntityMetadataIdGoldenTest, CatEntityHasIds0To22AndVariantAt19)
{
    CatEntity cat(EntityInstanceId(1), mc::test::testEcsRegistry());
    // Entity 0..7 + LivingEntity 8..14 + Mob 15 + AgeableMob 16 + TamableAnimal 17/18 + Cat 19..22。
    EXPECT_EQ(collectParamIds(cat.dataManager()), expectedRange(0, 22));
    EXPECT_FALSE(cat.dataManager().hasParam(23));

    // wire19 DATA_VARIANT_ID 必为 HolderVariantValue（variant index 18 → serializerId 21 CAT_VARIANT）。
    const u16 variantId = CatEntity::getVariantParamId();
    EXPECT_EQ(variantId, 19u);
    ASSERT_NE(cat.dataManager().getRaw(variantId), nullptr);
    EXPECT_EQ(cat.dataManager().getRaw(variantId)->index(), 18u); // HolderVariantValue

    // wire20 IS_LYING 必为 bool（variant index 5 → serializerId 8 BOOLEAN）。
    const u16 lyingId = CatEntity::getLyingParamId();
    EXPECT_EQ(lyingId, 20u);
    ASSERT_NE(cat.dataManager().getRaw(lyingId), nullptr);
    EXPECT_EQ(cat.dataManager().getRaw(lyingId)->index(), 5u); // bool/Boolean

    // wire21 RELAX_STATE_ONE 必为 bool。
    const u16 relaxId = CatEntity::getRelaxStateOneParamId();
    EXPECT_EQ(relaxId, 21u);
    ASSERT_NE(cat.dataManager().getRaw(relaxId), nullptr);
    EXPECT_EQ(cat.dataManager().getRaw(relaxId)->index(), 5u); // bool/Boolean

    // wire22 DATA_COLLAR_COLOR 必为 i32/Int（variant index 1 → serializerId 1 INT）。
    const u16 collarId = CatEntity::getCollarColorParamId();
    EXPECT_EQ(collarId, 22u);
    ASSERT_NE(cat.dataManager().getRaw(collarId), nullptr);
    EXPECT_EQ(cat.dataManager().getRaw(collarId)->index(), 1u); // i32/Int
}

// ============================================================================
// 链6: Nautilus 经 Nautilus→AbstractNautilus→TameableEntity→Animal→AgeableEntity
//   对齐 vanilla TamableAnimal：id17=DATA_FLAGS(Byte) + id18=DATA_OWNERUUID(Optional)
//   AbstractNautilus id19=DATA_DASH(Boolean)，共 id0..19。
//   真客户端 set_entity_data 严格校验每字段 serializerId：旧实现 id17 误发 Boolean
//   撞客户端期望的 Byte 致 Nautilus field17 崩溃（disconnect-2026-07-31）。
// ============================================================================
TEST(EntityMetadataIdGoldenTest, NautilusEntityHasIds0To19AndFlagsAt17)
{
    NautilusEntity nautilus(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(collectParamIds(nautilus.dataManager()), expectedRange(0, 19));
    EXPECT_FALSE(nautilus.dataManager().hasParam(20));

    // DATA_FLAGS(Byte) 必为 id17（对齐 vanilla TamableAnimal.DATA_FLAGS_ID）。
    const u16 flagsId = NautilusEntity::getTamedParamId();
    EXPECT_EQ(flagsId, 17u);
    ASSERT_NE(nautilus.dataManager().getRaw(flagsId), nullptr);
    EXPECT_EQ(nautilus.dataManager().getRaw(flagsId)->index(), 0u); // DATA_FLAGS → i8/Byte

    // DATA_OWNERUUID(OptionalLivingEntityRef) 必为 id18。
    const u16 ownerUuidId = NautilusEntity::getOwnerUuidParamId();
    EXPECT_EQ(ownerUuidId, 18u);
    ASSERT_NE(nautilus.dataManager().getRaw(ownerUuidId), nullptr);
    EXPECT_EQ(nautilus.dataManager().getRaw(ownerUuidId)->index(), 21u); // OptionalUuidValue

    // DATA_DASH(Boolean) 必为 id19（AbstractNautilusEntity 层）。
    const u16 dashId = AbstractNautilusEntity::getDashParamId();
    EXPECT_EQ(dashId, 19u);
    ASSERT_NE(nautilus.dataManager().getRaw(dashId), nullptr);
    EXPECT_EQ(nautilus.dataManager().getRaw(dashId)->index(), 5u); // DATA_DASH → bool/Boolean
}

// ============================================================================
// 链4: Raider.IS_CELEBRATING → AbstractRaiderEntity 层
//   AbstractRaiderEntity id16 IS_CELEBRATING(Boolean)（经 PatrollerEntity→MonsterEntity）
//   Pillager 经 AbstractIllager→AbstractRaider 透传，id 集合一致(id0..16)
// 此链对齐关系真客户端 set_entity_data：项目整条 Raider 链原无 classInfo，补齐中间层
// (Patroller/AbstractIllager/SpellcastingIllager) + 叶子 classInfo 后，IS_CELEBRATING 落 id16。
// ============================================================================

TEST(EntityMetadataIdGoldenTest, AbstractRaiderEntityHasIds0To16)
{
    AbstractRaiderEntity raider(EntityInstanceId(1), mc::test::testEcsRegistry());
    // Entity 0..7 + LivingEntity 8..14 + Mob 15 + AbstractRaider 16(IS_CELEBRATING)。
    EXPECT_EQ(collectParamIds(raider.dataManager()), expectedRange(0, 16));
    EXPECT_FALSE(raider.dataManager().hasParam(17));
}

TEST(EntityMetadataIdGoldenTest, AbstractRaiderIsCelebratingFieldIdAndTypeAlignVanilla)
{
    AbstractRaiderEntity raider(EntityInstanceId(1), mc::test::testEcsRegistry());
    const auto& mgr = raider.dataManager();

    const u16 celebratingId = AbstractRaiderEntity::getIsCelebratingParamId();
    EXPECT_EQ(celebratingId, 16u);
    ASSERT_NE(mgr.getRaw(celebratingId), nullptr);
    EXPECT_EQ(mgr.getRaw(celebratingId)->index(), 5u); // IS_CELEBRATING → bool/Boolean
}

TEST(EntityMetadataIdGoldenTest, PillagerEntityHasIds0To16)
{
    PillagerEntity pillager(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 经 AbstractIllager→AbstractRaider 透传，与 AbstractRaider 一致。
    EXPECT_EQ(collectParamIds(pillager.dataManager()), expectedRange(0, 16));
}

// ============================================================================
// 链5: 水生链 GlowSquid 对齐（崩溃直接根因）
// vanilla 1.21.11 GlowSquid 经 Squid→AgeableWaterCreature→AgeableMob：
//   id16 DATA_BABY(Boolean) / id17 DATA_DARK_TICKS_REMAINING(Int)
// 项目 WaterMobEntity 不经 AgeableEntity，故在 SquidEntity 层补 Boolean 占位对齐 id16，
// 使 GlowSquid.DATA_DARK_TICKS 落 id17。
// 崩溃现场(disconnect-2026-07-31_11.59.24)：field16 old=false(Boolean) ↔ new=0(Integer)，
// 即项目 GlowSquid.DATA_DARK_TICKS(Int) 错占 id16，撞客户端期望的 id16=DATA_BABY(Boolean)。
// 补占位后 id16=Boolean(占位)、id17=Int(DARK_TICKS)，与 vanilla 逐字段对齐。
// ============================================================================

TEST(EntityMetadataIdGoldenTest, SquidEntityHasIds0To16)
{
    SquidEntity squid(EntityInstanceId(1), mc::test::testEcsRegistry());
    // Entity 0..7 + LivingEntity 8..14 + Mob 15 + Squid 16(DATA_BABY 占位)。
    EXPECT_EQ(collectParamIds(squid.dataManager()), expectedRange(0, 16));
    EXPECT_FALSE(squid.dataManager().hasParam(17));
}

TEST(EntityMetadataIdGoldenTest, SquidBabyPlaceholderFieldIdAndTypeAlignVanilla)
{
    SquidEntity squid(EntityInstanceId(1), mc::test::testEcsRegistry());
    const auto& mgr = squid.dataManager();

    const u16 placeholderId = SquidEntity::getBabyPlaceholderParamId();
    EXPECT_EQ(placeholderId, 16u);
    ASSERT_NE(mgr.getRaw(placeholderId), nullptr);
    EXPECT_EQ(mgr.getRaw(placeholderId)->index(), 5u); // DATA_BABY 占位 → bool/Boolean
}

// 崩溃现场复现断言：GlowSquid.DATA_DARK_TICKS 必为 id17(Int)，id16 必为 Boolean(占位)。
// 修复前 DATA_DARK_TICKS 错占 id16(Int) 撞客户端 DATA_BABY(Boolean) 致类型校验崩。
TEST(EntityMetadataIdGoldenTest, GlowSquidEntityDarkTicksAt17AndBabyPlaceholderAt16)
{
    GlowSquidEntity glowSquid(EntityInstanceId(1), mc::test::testEcsRegistry());
    const auto& mgr = glowSquid.dataManager();

    // DATA_DARK_TICKS 必为 id17（修复前错占 id16）。
    const u16 darkTicksId = GlowSquidEntity::getDarkTicksRemainingParamId();
    EXPECT_EQ(darkTicksId, 17u);
    ASSERT_NE(mgr.getRaw(darkTicksId), nullptr);
    EXPECT_EQ(mgr.getRaw(darkTicksId)->index(), 1u); // DATA_DARK_TICKS → i32/Int

    // id16 必为 Boolean 占位（修复前为 Int，撞客户端 DATA_BABY(Boolean)）。
    ASSERT_NE(mgr.getRaw(16), nullptr);
    EXPECT_EQ(mgr.getRaw(16)->index(), 5u); // 占位 → bool/Boolean
}

// ============================================================================
// OptionalLivingEntityRef（serializerId 13）双向 codec 测试
//   对齐 vanilla TamableAnimal.DATA_OWNERUUID_ID wire 格式：
//   1 byte present + 若 present 则 16 字节大端连续 UUID。
//   present=0 → 编码 0D 00；present=1 → 编码 0D 01 + 16 字节 UUID。
// ============================================================================
TEST(EntityMetadataIdGoldenTest, OptionalLivingEntityRefCodecRoundTrip)
{
    using namespace mc::network;

    entity::EntityDataManager mgr;
    auto param = entity::EntityDataManager::createKey<entity::OptionalUuidValue>();
    mgr.registerParam(param, entity::OptionalUuidValue{false, {}});
    const u16 ownerId = param.id(); // 无 classInfo 上下文时全局自增分配

    // —— present=false ——
    {
        std::vector<u8> out;
        EntityMetadataSerializer::serializeEntry(ownerId, entity::DataValue(entity::OptionalUuidValue{false, {}}), out);
        // index(1 byte) + serializerId VarInt(13=0x0D) + present(0x00)
        ASSERT_GE(out.size(), 3u);
        EXPECT_EQ(out[0], ownerId); // index
        EXPECT_EQ(out[1], 0x0Du);   // serializerId 13 = OptionalLivingEntityRef
        EXPECT_EQ(out[2], 0x00u);   // present=false
    }

    // —— present=true ——
    Uuid uuid{};
    for (u8 i = 0; i < 16; ++i) {
        uuid[i] = static_cast<u8>(0x10 + i); // 0x10..0x1F
    }
    std::vector<u8> out;
    EntityMetadataSerializer::serializeEntry(ownerId, entity::DataValue(entity::OptionalUuidValue{true, uuid}), out);
    ASSERT_GE(out.size(), 19u);
    EXPECT_EQ(out[0], ownerId); // index
    EXPECT_EQ(out[1], 0x0Du);   // serializerId 13
    EXPECT_EQ(out[2], 0x01u);   // present=true
    for (u8 i = 0; i < 16; ++i) {
        EXPECT_EQ(out[3u + i], uuid[i]); // 16 字节 UUID 连续大端
    }

    // —— 反序列化往返：构造含一条 OptionalUuidValue 条目的元数据包 ——
    // 完整包格式：index + serializerId + value + 0xFF 终止符（参考 set_entity_data）。
    std::vector<u8> packet = out;
    packet.push_back(0xFFu); // 结束标记

    ASSERT_TRUE(EntityMetadataSerializer::deserialize(packet, mgr));

    const auto* raw = mgr.getRaw(ownerId);
    ASSERT_NE(raw, nullptr);
    ASSERT_EQ(raw->index(), 21u); // OptionalUuidValue
    const auto restored = raw->get<entity::OptionalUuidValue>();
    EXPECT_TRUE(restored.present);
    EXPECT_EQ(restored.uuid, uuid);
}
