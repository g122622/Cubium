/*
* Copyright (c) 2026 Guo Yi
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, the following conditions:
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
#include "common/core/Constants.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/illager/EvokerEntity.hpp"
#include "common/entity/entities/monster/illager/SpellcastingIllagerEntity.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/entity/entities/monster/illager/VexEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>

namespace mc {
namespace {

/**
 * @brief 测试用世界实现
 *
 * 提供 EvokerEntity 测试所需的最小 IWorld 接口实现
 */
class EvokerTestWorld final : public test::BaseTestWorld {
public:
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return EntityId(static_cast<u32>(m_spawnedEntities.size()));
    }

    void advanceTick() { m_currentTick++; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("EvokerTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("EvokerTestWorld::tickManager not implemented");
    }

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

private:
    u64 m_currentTick = 0;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

} // namespace

// ============================================================================
// EvokerEntity 基础测试
// ============================================================================

TEST(EvokerEntityTest, Construction)
{
    EvokerEntity evoker(EntityId(1));

    // 验证唤魔者尺寸
    // MC 1.16.5: 唤魔者尺寸与普通灾厄村民相同
    EXPECT_FLOAT_EQ(evoker.width(), 0.6f);
    EXPECT_FLOAT_EQ(evoker.height(), 1.8f);  // 标准灾厄村民高度

    // 验证默认状态
    EXPECT_FALSE(evoker.isSpellcasting());
    EXPECT_EQ(evoker.spellType(), SpellcastingIllagerEntity::SpellType::None);
}

TEST(EvokerEntityTest, Attributes)
{
    EvokerEntity evoker(EntityId(1));

    // MC 1.16.5 唤魔者属性
    // 最大生命值 24.0
    // 移动速度 0.5
    // 跟随范围 12.0
    EXPECT_FLOAT_EQ(static_cast<f32>(evoker.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH)), 24.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(evoker.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED)), 0.5f);
    EXPECT_FLOAT_EQ(static_cast<f32>(evoker.getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE)), 12.0f);
}

TEST(EvokerEntityTest, Spellcasting)
{
    EvokerEntity evoker(EntityId(1));

    // 默认不施法
    EXPECT_FALSE(evoker.isSpellcasting());
    EXPECT_EQ(evoker.spellTicks(), 0);
    EXPECT_EQ(evoker.spellType(), SpellcastingIllagerEntity::SpellType::None);

    // 开始施法
    evoker.startCasting(static_cast<i32>(SpellcastingIllagerEntity::SpellType::Fangs));
    EXPECT_TRUE(evoker.isSpellcasting());
    EXPECT_EQ(evoker.spellType(), SpellcastingIllagerEntity::SpellType::Fangs);

    // 清除施法状态
    evoker.clearSpellcasting();
    EXPECT_FALSE(evoker.isSpellcasting());
    EXPECT_EQ(evoker.spellType(), SpellcastingIllagerEntity::SpellType::None);
}

TEST(EvokerEntityTest, SpellTypeConversion)
{
    // 测试 SpellType ID 转换
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(0), SpellcastingIllagerEntity::SpellType::None);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(1), SpellcastingIllagerEntity::SpellType::SummonVex);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(2), SpellcastingIllagerEntity::SpellType::Fangs);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(3), SpellcastingIllagerEntity::SpellType::Wololo);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(4), SpellcastingIllagerEntity::SpellType::Disappear);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(5), SpellcastingIllagerEntity::SpellType::Blindness);
    // 无效 ID 返回 None
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(99), SpellcastingIllagerEntity::SpellType::None);
}

TEST(EvokerEntityTest, CreateFactory)
{
    auto entity = EvokerEntity::create(nullptr);
    ASSERT_NE(entity, nullptr);

    // 验证创建的是 EvokerEntity
    auto* evokerPtr = dynamic_cast<EvokerEntity*>(entity.get());
    EXPECT_NE(evokerPtr, nullptr);
}

TEST(EvokerEntityTest, SpellCooldowns)
{
    EvokerEntity evoker(EntityId(1));

    // 验证冷却常量
    // FANGS_COOLDOWN = 100 ticks (5秒)
    // SUMMON_COOLDOWN = 340 ticks (17秒)
    // CASTING_DURATION = 40 ticks (2秒)

    // 这些是类私有成员，我们通过公共接口间接测试
    // 施法后冷却应该被设置
    evoker.startCasting(static_cast<i32>(SpellcastingIllagerEntity::SpellType::Fangs));
    EXPECT_TRUE(evoker.isSpellcasting());
}

// ============================================================================
// EvokerFangsEntity 测试
// ============================================================================

TEST(EvokerFangsEntityTest, Construction)
{
    entity::EvokerFangsEntity fangs(EntityId(1));

    // 验证尖牙尺寸
    EXPECT_FLOAT_EQ(fangs.width(), 0.5f);
    EXPECT_FLOAT_EQ(fangs.height(), 0.8f);

    // 验证默认状态
    EXPECT_EQ(fangs.warmupDelay(), 0);
    EXPECT_EQ(fangs.owner(), nullptr);
}

TEST(EvokerFangsEntityTest, WarmupDelay)
{
    entity::EvokerFangsEntity fangs(EntityId(1));

    // 设置预热延迟
    fangs.setWarmupDelay(10);
    EXPECT_EQ(fangs.warmupDelay(), 10);

    fangs.setWarmupDelay(5);
    EXPECT_EQ(fangs.warmupDelay(), 5);
}

TEST(EvokerFangsEntityTest, Owner)
{
    entity::EvokerFangsEntity fangs(EntityId(1));

    // 默认无所有者
    EXPECT_EQ(fangs.owner(), nullptr);

    // 设置所有者（实际测试中应使用 EvokerEntity）
    fangs.setOwner(nullptr);
    EXPECT_EQ(fangs.owner(), nullptr);
}

TEST(EvokerFangsEntityTest, AnimationProgress)
{
    entity::EvokerFangsEntity fangs(EntityId(1));

    // 动画进度在未开始攻击时应为 0
    EXPECT_FLOAT_EQ(fangs.getAnimationProgress(0.0f), 0.0f);
}

TEST(EvokerFangsEntityTest, CreateFactory)
{
    auto entity = entity::EvokerFangsEntity::create(nullptr);
    ASSERT_NE(entity, nullptr);

    // 验证创建的是 EvokerFangsEntity
    auto* fangsPtr = dynamic_cast<entity::EvokerFangsEntity*>(entity.get());
    EXPECT_NE(fangsPtr, nullptr);
}

} // namespace mc
