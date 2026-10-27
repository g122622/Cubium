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

// SnifferEntity 繁殖产出对齐 MC Java 1.21.11 测试。
//
// 原版 Sniffer.spawnChildFromBreeding（Sniffer.java:337-344）：
//   ItemStack itemstack = new ItemStack(Items.SNIFFER_EGG);
//   ItemEntity itementity = new ItemEntity(level, getX(), getY(), getZ(), itemstack);
//   itementity.setDefaultPickUpDelay();
//   finalizeSpawnChildFromBreeding(level, partner, null);
//   playSound(SNIFFER_EGG_PLOP, 1.0F, (random.nextFloat() - random.nextFloat()) * 0.2F + 0.5F);
//   level.addFreshEntity(itementity);
// 即：繁殖产物是嗅探兽蛋物品实体，而非幼体；child 参数为 null。
//
// 此前缺陷：SnifferEntity 的 TODO 占位实现让 spawnBaby 直接返回幼体，繁殖会凭空生成
// 一只幼年嗅探兽。修复引入 AnimalEntity::spawnChildFromBreeding 可覆盖产物阶段：
// BreedGoal 先调产物阶段送入世界，再统一执行发情重置 / 繁殖冷却 / 繁殖事件 / 爱心粒子 / 经验球。
//
// 断言口径：
//   - 掉落物：world->spawnedEntities() 中唯一一个 ItemEntity，其物品为 SNIFFER_EGG，位置为本体位置；
//   - 无幼体：spawnedEntities() 中不存在 SnifferEntity；
//   - 音效：实体播放 SNIFFER_EGG_PLOP（经 Entity::playSound → IWorld::playSound 转发记录）；
//   - 繁殖后处理：双方发情清除、繁殖冷却 = 6000、经验球 1-7 个。
// 若 spawnChildFromBreeding 覆盖缺失（退回旧占位实现），第一个用例中掉落物断言会失败。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/entities/passive/special/SnifferEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>
#include <vector>

using namespace mc;

namespace {

/**
 * @brief 记录实体生成 / 音效 / 爱心粒子的测试世界
 *
 * spawnEntity 接管实体所有权（BaseTestWorld 未覆盖时默认实现直接销毁实体），
 * 使测试可以检查掉落物与经验球的类型、物品与位置。
 */
class SnifferBreedingTestWorld final : public mc::test::BaseTestWorld {
public:
    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawned.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawned.size());
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory /*category*/,
        const Vector3& /*position*/,
        f32 /*volume*/,
        f32 /*pitch*/) override
    {
        m_playedSounds.push_back(soundEventId);
    }

    void addParticle(particle::ParticleTypeId type, const Vector3& /*pos*/, const Vector3& /*velocity*/) override
    {
        if (type == particle::ParticleTypeId::Heart) {
            ++m_heartParticles;
        }
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const
    {
        return m_spawned;
    }

    [[nodiscard]] bool playedSound(const ResourceLocation& id) const
    {
        for (const auto& played : m_playedSounds) {
            if (played == id) {
                return true;
            }
        }
        return false;
    }

    /// 登记参与世界查询的实体（配偶定位走 getEntitiesInRange）。
    void addEntity(Entity* entity)
    {
        m_entities.push_back(entity);
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(
        const Vector3& pos, f32 range, const Entity* except) const override
    {
        std::vector<Entity*> result;
        for (Entity* entity : m_entities) {
            if (entity == except || entity->isRemoved()) {
                continue;
            }
            if (entity->distanceSqTo(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z)) <=
                range * range) {
                result.push_back(entity);
            }
        }
        return result;
    }

    i32 m_heartParticles = 0;

private:
    std::vector<std::unique_ptr<Entity>> m_spawned;
    std::vector<ResourceLocation> m_playedSounds;
    std::vector<Entity*> m_entities;
};

/// 统计某类实体的生成数量。
template <typename T>
i32 countSpawned(const SnifferBreedingTestWorld& world)
{
    i32 count = 0;
    for (const auto& entity : world.spawnedEntities()) {
        if (dynamic_cast<const T*>(entity.get()) != nullptr) {
            ++count;
        }
    }
    return count;
}

/// 找出唯一生成的物品实体（掉落物）。
const ItemEntity* findSpawnedItem(const SnifferBreedingTestWorld& world)
{
    for (const auto& entity : world.spawnedEntities()) {
        if (const auto* item = dynamic_cast<const ItemEntity*>(entity.get()); item != nullptr) {
            return item;
        }
    }
    return nullptr;
}

} // namespace

class SnifferBreedingTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
        // 物品注册（SNIFFER_EGG / 火把花种子 / 瓶草荚果）先于物品标签构建
        // ——SNIFFER_FOOD 标签的成员在初始化时经 ItemRegistry 取物品指针。
        Items::ensureInitialized();
        item::tag::ItemTags::initialize();
    }

    SnifferBreedingTestWorld m_world;
};

// 繁殖产出为嗅探兽蛋物品：不生成幼体，播放 SNIFFER_EGG_PLOP。
TEST_F(SnifferBreedingTest, SpawnChildFromBreeding_DropsSnifferEggInsteadOfBaby)
{
    SnifferEntity parent(EntityInstanceId(1), mc::test::testEcsRegistry());
    SnifferEntity partner(EntityInstanceId(2), mc::test::testEcsRegistry());
    parent.setWorld(&m_world);
    partner.setWorld(&m_world);
    parent.setPosition(10.5f, 64.0f, -3.5f);
    partner.setPosition(11.5f, 64.0f, -3.5f);

    AnimalEntity* baby = parent.spawnChildFromBreeding(partner);

    ASSERT_EQ(baby, nullptr) << "sniffer breeding must not spawn a baby entity";
    ASSERT_EQ(countSpawned<SnifferEntity>(m_world), 0) << "no sniffer baby entity should be spawned";

    const ItemEntity* egg = findSpawnedItem(m_world);
    ASSERT_NE(egg, nullptr) << "sniffer breeding must drop an item entity";
    EXPECT_EQ(egg->getItemStack().getItem(), Items::SNIFFER_EGG);
    EXPECT_EQ(countSpawned<ItemEntity>(m_world), 1) << "exactly one egg should be dropped";
    EXPECT_FLOAT_EQ(egg->x(), parent.x());
    EXPECT_FLOAT_EQ(egg->y(), parent.y());
    EXPECT_FLOAT_EQ(egg->z(), parent.z());

    EXPECT_TRUE(m_world.playedSound(SoundEvents::SNIFFER_EGG_PLOP))
        << "vanilla Sniffer.spawnChildFromBreeding plays SNIFFER_EGG_PLOP";
}

// 客户端不产出繁殖物品（vanilla spawnChildFromBreeding 仅在 ServerLevel 执行）。
TEST_F(SnifferBreedingTest, SpawnChildFromBreeding_NoopWithoutServerWorld)
{
    SnifferEntity parent(EntityInstanceId(1), mc::test::testEcsRegistry());
    SnifferEntity partner(EntityInstanceId(2), mc::test::testEcsRegistry());

    // 未设置世界：不应崩溃或产出任何实体。
    EXPECT_EQ(parent.spawnChildFromBreeding(partner), nullptr);

    parent.setWorld(&m_world);
    partner.setWorld(&m_world);
    // spawnBaby 仅保留接口契约，不参与繁殖生产链路。
    EXPECT_EQ(parent.spawnBaby(partner), nullptr);
    EXPECT_TRUE(m_world.spawnedEntities().empty());
}

// 繁殖物品来自 SNIFFER_FOOD 标签（火把花种子 / 瓶草荚果），其它物品不可繁殖。
TEST_F(SnifferBreedingTest, IsBreedingItem_UsesSnifferFoodTag)
{
    SnifferEntity sniffer(EntityInstanceId(1), mc::test::testEcsRegistry());
    sniffer.setWorld(&m_world);

    ASSERT_NE(Items::TORCHFLOWER_SEEDS, nullptr);
    ASSERT_NE(Items::PITCHER_POD, nullptr);
    ASSERT_NE(Items::WHEAT, nullptr);

    EXPECT_TRUE(sniffer.isBreedingItem(ItemStack(Items::TORCHFLOWER_SEEDS)));
    EXPECT_TRUE(sniffer.isBreedingItem(ItemStack(Items::PITCHER_POD)));
    EXPECT_FALSE(sniffer.isBreedingItem(ItemStack(Items::WHEAT)));
    EXPECT_FALSE(sniffer.isBreedingItem(ItemStack()));
}

// BreedGoal 繁殖产出链路：掉蛋 + 双方发情清除 + 繁殖冷却 + 经验球 + 爱心粒子。
//
// 说明：BreedGoal::tick 的繁殖判定需跨 60 tick 累积 m_spawnBabyDelay，且
// shouldContinueExecuting 以同一倒计时作上限门控，单元测试无法在一次 tick 内到达；
// 故这里通过 BreedGoal 对外的 spawnBaby() 直接驱动，同时需先把配偶定位到 m_targetMate
// ——借助一次 tick() 完成寻路并触发 shouldExecute 的配偶定位，再调用 spawnBaby()。
TEST_F(SnifferBreedingTest, BreedGoal_DropsEggAndFinalizesBreeding)
{
    constexpr u64 kLoveCause = 42;

    SnifferEntity parent(EntityInstanceId(1), mc::test::testEcsRegistry());
    SnifferEntity partner(EntityInstanceId(2), mc::test::testEcsRegistry());
    parent.setWorld(&m_world);
    partner.setWorld(&m_world);
    parent.setPosition(0.5f, 64.0f, 0.5f);
    partner.setPosition(1.5f, 64.0f, 0.5f);
    m_world.addEntity(&parent);
    m_world.addEntity(&partner);

    // 双方成体、处于可交配状态并进入求爱。
    parent.setGrowingAge(0);
    partner.setGrowingAge(0);
    parent.setInLove(kLoveCause);
    partner.setInLove(kLoveCause);
    ASSERT_TRUE(parent.isInLove());
    ASSERT_TRUE(partner.isInLove());

    // shouldExecute 经 findNearbyMate 定位配偶（走 getEntitiesInRange 世界查询），
    // 定位成功后由 spawnBaby 执行产出与繁殖后处理。
    entity::ai::goal::BreedGoal goal(&parent, 1.0);
    ASSERT_TRUE(goal.shouldExecute()) << "BreedGoal must locate the partner sniffer in love";
    goal.spawnBaby();

    // 产物：恰好一枚嗅探兽蛋，无幼体。
    EXPECT_EQ(countSpawned<SnifferEntity>(m_world), 0);
    const ItemEntity* egg = findSpawnedItem(m_world);
    ASSERT_NE(egg, nullptr);
    EXPECT_EQ(egg->getItemStack().getItem(), Items::SNIFFER_EGG);

    // finalizeSpawnChildFromBreeding 的与产物无关部分：发情清除 + 繁殖冷却。
    EXPECT_FALSE(parent.isInLove());
    EXPECT_FALSE(partner.isInLove());
    EXPECT_EQ(parent.getGrowingAge(), AgeableEntity::BREEDING_COOLDOWN);
    EXPECT_EQ(partner.getGrowingAge(), AgeableEntity::BREEDING_COOLDOWN);

    // 经验球 1-7 个，生成在父母位置。
    const i32 xpCount = countSpawned<ExperienceOrbEntity>(m_world);
    EXPECT_GE(xpCount, 1);
    EXPECT_LE(xpCount, 7);

    // 双方爱心粒子。
    EXPECT_EQ(m_world.m_heartParticles, 2);
}
