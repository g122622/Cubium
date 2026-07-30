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
// id 基线（vanilla 1.21.11）：
//   Entity           id0..7   FLAGS/AIR/CUSTOM_NAME_VISIBLE/CUSTOM_NAME/SILENT/NO_GRAVITY/POSE/TICKS_FROZEN
//   LivingEntity     id8..14  LIVING_FLAGS/HEALTH/EFFECT_PARTICLES/EFFECT_AMBIENCE/ARROW_COUNT/STINGER_COUNT/SLEEPING_POS
//   MobEntity        id15     MOB_FLAGS
//   Player(扁平化)   id15..20 (Avatar id15-16: MAIN_HAND/MODE_CUSTOMISATION; Player id17-20: ABSORPTION/SCORE/SHOULDER_PARROT_L/SHOULDER_PARROT_R)
//   FallingBlockEntity  id8  (DATA_BLOCK_STATE，直接继承 Entity)
//   TNTEntity           id8..9 (DATA_FUSE/DATA_BLOCK_STATE，直接继承 Entity)
//   FishingBobberEntity id8..9 (DATA_HOOKED_ENTITY/DATA_BITING，直接继承 Entity)
//   AbstractMinecartEntity id8..13 (ROLLING_AMPLITUDE/ROLLING_DIRECTION/DAMAGE/DISPLAY_TILE/DISPLAY_TILE_OFFSET/SHOW_BLOCK，直接继承 Entity)

#include "entity/core/DataParameter.hpp"
#include "entity/core/Entity.hpp"
#include "entity/core/EntityDataManager.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/core/MobEntity.hpp"
#include "entity/entities/misc/MiscEntities.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/entities/projectile/OtherProjectiles.hpp"
#include "entity/entities/vehicle/MinecartEntity.hpp"

#include <gtest/gtest.h>
#include <set>

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
    Entity entity(EntityInstanceId(1));
    entity.registerData();
    EXPECT_EQ(collectParamIds(entity.dataManager()), expectedRange(0, 7));
}

TEST(EntityMetadataIdGoldenTest, LivingEntityHasIds0To14)
{
    LivingEntity living(EntityInstanceId(1));
    living.registerData();
    EXPECT_EQ(collectParamIds(living.dataManager()), expectedRange(0, 14));
}

TEST(EntityMetadataIdGoldenTest, MobEntityHasIds0To15)
{
    MobEntity mob(EntityInstanceId(1));
    mob.registerData();
    EXPECT_EQ(collectParamIds(mob.dataManager()), expectedRange(0, 15));
}

// ============================================================================
// Player 扁平化（Avatar 字段内联进 Player，classInfo parent=LivingEntity）
// vanilla Avatar(id15-16) + Player(id17-20)，共 21 字段 id0..20。
// ============================================================================

TEST(EntityMetadataIdGoldenTest, PlayerHasIds0To20)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    EXPECT_EQ(collectParamIds(player.dataManager()), expectedRange(0, 20));
    // 第 21 个槽位不应存在（Player 仅 21 字段，id 最大 20）。
    EXPECT_FALSE(player.dataManager().hasParam(21));
}

// ============================================================================
// 直接继承 Entity 的叶子类（字段从 id8 起）
// ============================================================================

TEST(EntityMetadataIdGoldenTest, FallingBlockEntityHasIds0To8)
{
    FallingBlockEntity entity;
    // FallingBlockEntity 仅 1 个自身字段 DATA_BLOCK_STATE(id8)，加 Entity id0..7。
    EXPECT_EQ(collectParamIds(entity.dataManager()), expectedRange(0, 8));
}

TEST(EntityMetadataIdGoldenTest, TNTEntityHasIds0To9)
{
    TNTEntity entity;
    // DATA_FUSE(id8) + DATA_BLOCK_STATE(id9) + Entity id0..7。
    EXPECT_EQ(collectParamIds(entity.dataManager()), expectedRange(0, 9));
}

TEST(EntityMetadataIdGoldenTest, FishingBobberEntityHasIds0To9)
{
    FishingBobberEntity entity(EntityInstanceId(1));
    // DATA_HOOKED_ENTITY(id8) + DATA_BITING(id9) + Entity id0..7。
    EXPECT_EQ(collectParamIds(entity.dataManager()), expectedRange(0, 9));
}

TEST(EntityMetadataIdGoldenTest, AbstractMinecartEntityHasIds0To13)
{
    AbstractMinecartEntity entity(AbstractMinecartEntity::Type::Rideable);
    // 6 个自身字段 id8..13 + Entity id0..7。
    EXPECT_EQ(collectParamIds(entity.dataManager()), expectedRange(0, 13));
}
