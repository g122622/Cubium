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

// 爆炸改投射物归属测试（对应 vanilla 1.21.11 ServerExplosion.hurtEntities:199-200）。
//
// vanilla 在爆炸 hurt + push 之后、onExplosionHit 之前，对爆炸范围内的可偏转投射物
// （EntityTypeTags.REDIRECTABLE_PROJECTILE = fireball/wind_charge/breeze_wind_charge）
// 额外调 projectile.setOwner(this.damageSource.getEntity())——不跳过伤害，只把被波及
// 投射物的所有者改为爆炸伤害源实体。Cubium 在 Explosion.cpp 的 _redirectProjectilesInBlast
// 中以 setShooter 对应实现。本测试验证该归属改写语义。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/projectile/AbstractFireballEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/explosion/ExplosionContext.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::world::explosion;

namespace mc {
namespace {

/**
 * @brief 爆炸改归属测试用世界存根
 *
 * 在 BaseTestWorld 基础上覆写：
 * - getEntitiesInAABB：返回预设的实体列表（忽略 AABB 与 source 过滤，与
 *   ExplosionPlayerBranchTest 同范式）；
 * - getEntity：维护 id→Entity* 映射，供 ProjectileEntity::getShooter 经
 *   m_world->getEntity(shooterId) 反查发射者（爆炸改归属断言依赖此）；
 * - playSound / addParticle：空操作（避免测试噪音）。
 */
class ExplosionRedirectWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ExplosionRedirectWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ExplosionRedirectWorld::tickManager not implemented");
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return m_entities;
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        const auto it = m_entityMap.find(id);
        return it != m_entityMap.end() ? it->second : nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        const auto it = m_entityMap.find(id);
        return it != m_entityMap.end() ? it->second : nullptr;
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override
    {
        // 测试中忽略音效
    }

    void addParticle(
        particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3& = Vector3(0, 0, 0), u32 = 1) override
    {
        // 测试中忽略粒子
    }

    /// 设置 getEntitiesInAABB 返回的实体列表（爆炸迭代对象）
    void setEntities(std::vector<Entity*> entities) { m_entities = std::move(entities); }

    /// 注册实体到 id→Entity* 映射（供 getShooter 反查）
    void registerEntity(Entity* entity)
    {
        if (entity != nullptr) {
            m_entityMap[entity->id()] = entity;
        }
    }

private:
    std::vector<Entity*> m_entities;
    std::unordered_map<EntityInstanceId, Entity*> m_entityMap;
};

/**
 * @brief 测试用 LivingEntity（爆炸源实体）
 *
 * 继承 LivingEntity 以使 Explosion 默认 damageSource
 * （EntityDamageSource(Explosion, m_source)）的 getEntity() 返回本实体。
 */
class TestLivingEntity final : public LivingEntity {
public:
    explicit TestLivingEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
        : LivingEntity(id, nullptr, registry)
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

/**
 * @brief 爆炸改归属测试固件
 */
class ExplosionRedirectProjectileTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保 EntityTypeTags 已初始化（REDIRECTABLE_PROJECTILE 成员集依赖此）。
        // 与 ProjectileDeflectionTest 同范式：单测组合跑时全局静态 s_initialized 可能已被
        // 其他套件置位，此处幂等初始化保证标签成员集就绪。
        if (!EntityTypeTags::isInitialized()) {
            entity::VanillaEntities::registerAll();
            EntityTypeTags::initialize();
        }

        // 爆炸中心位于原点附近
        m_explosionPos = Vector3(0.0f, 64.0f, 0.0f);
    }

    ExplosionRedirectWorld m_world;
    Vector3 m_explosionPos{0.0f, 64.0f, 0.0f};
};

// ============================================================================
// 默认 damageSource 场景：爆炸源实体归属改写
// ============================================================================

TEST_F(ExplosionRedirectProjectileTest, Fireball_OwnerChangedToExplosionSource)
{
    // 验证修复后：爆炸波及 fireball 时，fireball 的所有者改为爆炸伤害源实体
    // （对应 vanilla ServerExplosion:199-200 projectile.setOwner(damageSource.getEntity())）。
    //
    // 默认爆炸（无自定义 damageSource）走 EntityDamageSource(Explosion, m_source)，
    // getEntity()=m_source（爆炸源实体）。故 fireball 新 owner 应为 source。
    //
    // 确定性前提：BaseTestWorld::getBlockState 返回 nullptr（空气），_getBlockDensity
    // 中所有射线 isMiss，seenPercent=1.0；fireball 位于 (0.5,64,0)，爆炸中心 (0,64,0)，
    // range=4*2=8，distanceRatio=0.5/8=0.0625<1，damage>0，进入伤害分支 →
    // _redirectProjectilesInBlast 被调用。

    // 爆炸源实体（LivingEntity，作为 m_source）
    TestLivingEntity source(EntityInstanceId(2), mc::test::testEcsRegistry());
    source.setPosition(0.0f, 64.0f, 0.0f);
    source.setWorld(&m_world);
    m_world.registerEntity(&source);

    // 被波及的火球（REDIRECTABLE_PROJECTILE 成员）。直接构造后 setTypeId 对齐工厂路径
    // （getTypeId 仅在 EntityType::create 工厂里 setTypeId，直接构造须手动补，否则
    // REDIRECTABLE_PROJECTILE.contains(typeId) 查空串不在标签内，归属改写被跳过——
    // 同记忆 #7「直接构造实体须 setTypeId 对齐工厂路径」范式）。
    entity::FireballEntity fireball(EntityInstanceId(3), mc::test::testEcsRegistry());
    fireball.setTypeId(entity::EntityTypeKeys::FIREBALL);
    fireball.setPosition(0.5f, 64.0f, 0.0f);
    fireball.setWorld(&m_world);
    m_world.registerEntity(&fireball);

    // 必须把受波及实体纳入 getEntitiesInAABB 返回列表，爆炸才会迭代到它。
    // source 与 fireball 都需在列表中（source 作为爆炸源也会被迭代，但其自身 owner 改写无意义，
    // 仅验证不崩溃）。
    m_world.setEntities({&source, &fireball});

    // 前置：火球初始无发射者
    EXPECT_EQ(fireball.getShooter(), nullptr);

    // 半径 4.0，ExplosionMode::None 不破坏方块，专注实体归属改写。
    // source 作为爆炸源传入。
    Explosion explosion(m_world, m_explosionPos, 4.0f, ExplosionMode::None, false, &source);
    explosion.explode();

    // 爆炸后火球所有者应改为爆炸源实体
    EXPECT_EQ(fireball.getShooter(), &source);
}

TEST_F(ExplosionRedirectProjectileTest, Fireball_OwnerPreservedWhenNoExplosionSource)
{
    // 验证：爆炸无源实体（m_source=nullptr）时，damageSource.getEntity()=nullptr，
    // _redirectProjectilesInBlast 因 newOwner==nullptr 跳过 setShooter，火球 owner 不变。
    // 对应 vanilla：damageSource.getEntity() 为 null 时不改归属。

    entity::FireballEntity fireball(EntityInstanceId(3), mc::test::testEcsRegistry());
    fireball.setTypeId(entity::EntityTypeKeys::FIREBALL);
    fireball.setPosition(0.5f, 64.0f, 0.0f);
    fireball.setWorld(&m_world);
    m_world.registerEntity(&fireball);
    m_world.setEntities({&fireball});

    EXPECT_EQ(fireball.getShooter(), nullptr);

    // 无 source（nullptr）的爆炸
    Explosion explosion(m_world, m_explosionPos, 4.0f, ExplosionMode::None, false, nullptr);
    explosion.explode();

    // 无爆炸源 → 不改归属 → owner 仍为 nullptr
    EXPECT_EQ(fireball.getShooter(), nullptr);
}

TEST_F(ExplosionRedirectProjectileTest, NonRedirectableEntity_OwnerNotChanged)
{
    // 对照测试：非 REDIRECTABLE_PROJECTILE 成员的实体（LivingEntity）被爆炸波及时，
    // 其本身无 shooter 概念，且 _redirectProjectilesInBlast 因 typeId 不在标签内提前返回，
    // 不应触发任何异常。此测试锁定「标签门控正确」——仅 fireball/wind_charge/
    // breeze_wind_charge 三类被改归属，其他实体不受影响。

    TestLivingEntity source(EntityInstanceId(2), mc::test::testEcsRegistry());
    source.setPosition(0.0f, 64.0f, 0.0f);
    source.setWorld(&m_world);
    m_world.registerEntity(&source);

    // 另一个 LivingEntity 作为被波及的非投射物实体（typeId 非 REDIRECTABLE_PROJECTILE）
    TestLivingEntity bystander(EntityInstanceId(4), mc::test::testEcsRegistry());
    bystander.setTypeId("minecraft:zombie"); // 非 REDIRECTABLE_PROJECTILE 成员
    bystander.setPosition(0.5f, 64.0f, 0.0f);
    bystander.setWorld(&m_world);
    m_world.registerEntity(&bystander);
    m_world.setEntities({&source, &bystander});

    Explosion explosion(m_world, m_explosionPos, 4.0f, ExplosionMode::None, false, &source);
    explosion.explode();

    // 非投射物实体不受归属改写影响（无 shooter 概念，仅验证不崩溃、标签门控正确）
    // bystander 仍存活且未受 setShooter 影响（LivingEntity 无该方法）
    EXPECT_TRUE(bystander.isAlive());
}

// ============================================================================
// 自定义 damageSource 场景：归属改写取 damageSource.getEntity()
// ============================================================================

TEST_F(ExplosionRedirectProjectileTest, Fireball_OwnerChangedToCustomDamageSourceEntity)
{
    // 验证：爆炸传入自定义 damageSource 时，归属改写取 damageSource->getEntity()
    // （对应 vanilla damageSource.getEntity()），而非 m_source。
    //
    // 场景：末影水晶被玩家破坏后爆炸——m_source=末影水晶实体，自定义 damageSource
    // 经 DamageSources::explosion(source, cause) 构造为 IndirectEntityDamageSource，
    // 其 getEntity()=cause（破坏者玩家）。火球被波及后 owner 应改为 cause 而非 m_source。
    //
    // 本测试用 IndirectEntityDamageSource(Explosion, cause, source) 模拟：m_source=source 实体，
    // damageSource 的 getEntity()=cause（IndirectEntityDamageSource::getEntity 返回 source 字段=cause），
    // 断言火球 owner 改为 cause。

    TestLivingEntity source(EntityInstanceId(2), mc::test::testEcsRegistry());
    source.setPosition(0.0f, 64.0f, 0.0f);
    source.setWorld(&m_world);
    m_world.registerEntity(&source);

    // cause 实体（自定义 damageSource 绑定的归属实体，如破坏末影水晶的玩家）
    TestLivingEntity cause(EntityInstanceId(5), mc::test::testEcsRegistry());
    cause.setPosition(2.0f, 64.0f, 0.0f);
    cause.setWorld(&m_world);
    m_world.registerEntity(&cause);

    entity::FireballEntity fireball(EntityInstanceId(3), mc::test::testEcsRegistry());
    fireball.setTypeId(entity::EntityTypeKeys::FIREBALL);
    fireball.setPosition(0.5f, 64.0f, 0.0f);
    fireball.setWorld(&m_world);
    m_world.registerEntity(&fireball);
    m_world.setEntities({&source, &cause, &fireball});

    EXPECT_EQ(fireball.getShooter(), nullptr);

    // 自定义 damageSource 绑定 cause（IndirectEntityDamageSource 语义：
    // getEntity()=source 字段=cause）。setExplosion 标记为爆炸伤害源。
    auto customDamageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Explosion, &cause, &source);
    customDamageSource->setExplosion();

    Explosion explosion(
        m_world, m_explosionPos, 4.0f, ExplosionMode::None, false, &source, std::move(customDamageSource));
    explosion.explode();

    // 火球 owner 应改为 damageSource.getEntity()=cause，而非 m_source=source
    EXPECT_EQ(fireball.getShooter(), &cause);
}

// ============================================================================
// 仅击退段场景：shouldDamage=false 但有击退倍率时仍改归属
// ============================================================================

TEST_F(ExplosionRedirectProjectileTest, Fireball_OwnerChangedInKnockbackOnlyBranch)
{
    // 验证：爆炸走仅击退段（shouldDamage=false、knockbackMultiplier>0）时，
    // _redirectProjectilesInBlast 仍执行（对应 vanilla 无条件 setOwner）。
    //
    // 默认 EntityExplosionContext 下 shouldDamage=true、knockbackMul=1.0，永不走仅击退段。
    // 本测试构造 shouldDamage=false 的自定义 ExplosionContext 触发仅击退段，
    // 验证该段同样改写火球归属。

    // 自定义 ExplosionContext：不造成伤害，仅击退
    class KnockbackOnlyContext : public EntityExplosionContext {
    public:
        explicit KnockbackOnlyContext(const Entity* src)
            : EntityExplosionContext(src)
        {}

        [[nodiscard]] bool shouldDamageEntity(const Explosion&, const Entity&) const override { return false; }
        [[nodiscard]] f32 getKnockbackMultiplier(const Explosion&, const Entity&) const override { return 1.0f; }
    };

    TestLivingEntity source(EntityInstanceId(2), mc::test::testEcsRegistry());
    source.setPosition(0.0f, 64.0f, 0.0f);
    source.setWorld(&m_world);
    m_world.registerEntity(&source);

    entity::FireballEntity fireball(EntityInstanceId(3), mc::test::testEcsRegistry());
    fireball.setTypeId(entity::EntityTypeKeys::FIREBALL);
    fireball.setPosition(0.5f, 64.0f, 0.0f);
    fireball.setWorld(&m_world);
    m_world.registerEntity(&fireball);
    m_world.setEntities({&source, &fireball});

    EXPECT_EQ(fireball.getShooter(), nullptr);

    auto context = std::make_unique<KnockbackOnlyContext>(&source);
    // 使用接受自定义 context 的 Explosion 构造重载
    Explosion explosion(m_world,
        m_explosionPos,
        4.0f,
        ExplosionMode::None,
        false,
        &source,
        nullptr, // 无自定义 damageSource → 仅击退段用 EntityDamageSource(Explosion, m_source) 回退
        nullptr, // lootTableManager
        std::move(context));
    explosion.explode();

    // 仅击退段同样改写归属：火球 owner 应改为 m_source（回退 damageSource 的 getEntity()）
    EXPECT_EQ(fireball.getShooter(), &source);
}

} // namespace
} // namespace mc
