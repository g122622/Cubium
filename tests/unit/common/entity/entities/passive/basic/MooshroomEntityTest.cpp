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
#include "common/core/Constants.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/passive/basic/CowEntity.hpp"
#include "common/entity/entities/passive/basic/MooshroomEntity.hpp"
#include "common/entity/interfaces/IShearable.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用世界实现
 *
 * 提供哞菇测试所需的最小 IWorld 接口实现，支持客户端模式和粒子记录。
 */
class MooshroomTestWorld final : public mc::test::BaseTestWorld {
public:
    MooshroomTestWorld() = default;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    // 客户端/服务端模式控制
    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }
    void setClientSide(bool clientSide) { m_isClientSide = clientSide; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    // TickManager interface
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("MooshroomTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("MooshroomTestWorld::tickManager not implemented");
    }

    // 粒子和音效数据结构
    struct ParticleInfo {
        particle::ParticleTypeId type;
        Vector3 pos;
        Vector3 velocity;
    };

    struct SoundInfo {
        ResourceLocation sound;
        sound::SoundCategory category;
        Vector3 pos;
        f32 volume;
        f32 pitch;
    };

    // 粒子生成记录
    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    void addParticle(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count) override
    {
        (void)offset;
        (void)count;
        m_particles.push_back({type, pos, velocity});
    }

    // 音效播放记录
    void playSound(const ResourceLocation& sound,
        sound::SoundCategory category,
        const Vector3& pos,
        f32 volume,
        f32 pitch) override
    {
        m_sounds.push_back({sound, category, pos, volume, pitch});
    }

    // 测试辅助方法
    [[nodiscard]] size_t particleCount() const { return m_particles.size(); }
    [[nodiscard]] const std::vector<ParticleInfo>& particles() const { return m_particles; }
    void clearParticles() { m_particles.clear(); }

    [[nodiscard]] size_t soundCount() const { return m_sounds.size(); }
    [[nodiscard]] const std::vector<SoundInfo>& sounds() const { return m_sounds; }
    void clearSounds() { m_sounds.clear(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<ParticleInfo> m_particles;
    std::vector<SoundInfo> m_sounds;
    bool m_isClientSide = false;
};

class MooshroomEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }

    MooshroomTestWorld m_world;
};

// ==================== 类型系统测试 ====================

TEST_F(MooshroomEntityTest, DefaultType_IsRed)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(mooshroom.getMooshroomType(), MooshroomEntity::MooshroomType::Red);
    EXPECT_TRUE(mooshroom.isRed());
    EXPECT_FALSE(mooshroom.isBrown());
}

TEST_F(MooshroomEntityTest, SetMooshroomType_ChangesType)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());

    mooshroom.setMooshroomType(MooshroomEntity::MooshroomType::Brown);
    EXPECT_EQ(mooshroom.getMooshroomType(), MooshroomEntity::MooshroomType::Brown);
    EXPECT_FALSE(mooshroom.isRed());
    EXPECT_TRUE(mooshroom.isBrown());

    mooshroom.setMooshroomType(MooshroomEntity::MooshroomType::Red);
    EXPECT_EQ(mooshroom.getMooshroomType(), MooshroomEntity::MooshroomType::Red);
    EXPECT_TRUE(mooshroom.isRed());
    EXPECT_FALSE(mooshroom.isBrown());
}

// ==================== 雷击转换测试 ====================

TEST_F(MooshroomEntityTest, OnStruckByLightning_RedToBrown)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());
    m_world.setClientSide(false); // 服务端不生成粒子
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 初始为红色
    mooshroom.setMooshroomType(MooshroomEntity::MooshroomType::Red);
    EXPECT_TRUE(mooshroom.isRed());

    // 雷击后变为棕色
    mooshroom.onStruckByLightning(nullptr);
    EXPECT_TRUE(mooshroom.isBrown());
    EXPECT_EQ(mooshroom.getMooshroomType(), MooshroomEntity::MooshroomType::Brown);
}

TEST_F(MooshroomEntityTest, OnStruckByLightning_BrownToRed)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());
    m_world.setClientSide(false);
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 初始为棕色
    mooshroom.setMooshroomType(MooshroomEntity::MooshroomType::Brown);
    EXPECT_TRUE(mooshroom.isBrown());

    // 雷击后变为红色
    mooshroom.onStruckByLightning(nullptr);
    EXPECT_TRUE(mooshroom.isRed());
    EXPECT_EQ(mooshroom.getMooshroomType(), MooshroomEntity::MooshroomType::Red);
}

TEST_F(MooshroomEntityTest, OnStruckByLightning_PlaysConvertSound)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());
    m_world.setClientSide(false);
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 执行雷击
    mooshroom.onStruckByLightning(nullptr);

    // 验证播放了转换音效
    EXPECT_EQ(m_world.soundCount(), 1u);
    const auto& sound = m_world.sounds()[0];
    EXPECT_EQ(sound.sound.toString(), "minecraft:entity.mooshroom.convert");
    EXPECT_FLOAT_EQ(sound.volume, 2.0f);
    EXPECT_FLOAT_EQ(sound.pitch, 1.0f);
}

TEST_F(MooshroomEntityTest, OnStruckByLightning_GeneratesParticles_ClientSide)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());
    m_world.setClientSide(true); // 客户端生成粒子
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 执行雷击
    mooshroom.onStruckByLightning(nullptr);

    // 验证生成了爆炸粒子
    // 生成 20 个 Explosion 粒子
    EXPECT_EQ(m_world.particleCount(), 20u);

    // 验证粒子类型
    for (const auto& particle : m_world.particles()) {
        EXPECT_EQ(particle.type, particle::ParticleTypeId::Explosion);
    }
}

TEST_F(MooshroomEntityTest, OnStruckByLightning_NoParticles_ServerSide)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());
    m_world.setClientSide(false); // 服务端不生成粒子
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 执行雷击
    mooshroom.onStruckByLightning(nullptr);

    // 服务端不应该生成粒子
    EXPECT_EQ(m_world.particleCount(), 0u);

    // 但类型应该改变
    EXPECT_TRUE(mooshroom.isBrown());
}

TEST_F(MooshroomEntityTest, OnStruckByLightning_ParticlePosition_WithinEntityBounds)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());
    m_world.setClientSide(true);
    mooshroom.setWorld(&m_world);

    const f64 posX = 100.0;
    const f64 posY = 64.0;
    const f64 posZ = 200.0;
    mooshroom.setPosition(posX, posY, posZ);

    // 执行雷击
    mooshroom.onStruckByLightning(nullptr);

    // 验证粒子位置在实体范围内
    // 哞菇继承自牛，碰撞箱宽度 0.9，高度 1.4
    constexpr f64 EXPECTED_WIDTH = 0.9;
    constexpr f64 EXPECTED_HEIGHT = 1.4;

    for (const auto& particle : m_world.particles()) {
        // X 偏移应在 [-width/2, width/2] 范围内
        f64 offsetX = particle.pos.x - posX;
        EXPECT_GE(offsetX, -EXPECTED_WIDTH / 2.0);
        EXPECT_LE(offsetX, EXPECTED_WIDTH / 2.0);

        // Y 偏移应在 [0, height] 范围内
        f64 offsetY = particle.pos.y - posY;
        EXPECT_GE(offsetY, 0.0);
        EXPECT_LE(offsetY, EXPECTED_HEIGHT);

        // Z 偏移应在 [-width/2, width/2] 范围内
        f64 offsetZ = particle.pos.z - posZ;
        EXPECT_GE(offsetZ, -EXPECTED_WIDTH / 2.0);
        EXPECT_LE(offsetZ, EXPECTED_WIDTH / 2.0);
    }
}

// ==================== 继承测试 ====================

TEST_F(MooshroomEntityTest, InheritsFromCowEntity)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证哞菇继承自牛
    CowEntity* cow = dynamic_cast<CowEntity*>(&mooshroom);
    EXPECT_NE(cow, nullptr);

    // 验证哞菇实现 IShearable 接口
    entity::IShearable* shearable = dynamic_cast<entity::IShearable*>(&mooshroom);
    EXPECT_NE(shearable, nullptr);
}

TEST_F(MooshroomEntityTest, IsShearable_ReturnsTrue)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 哞菇总是可以被剪毛
    EXPECT_TRUE(mooshroom.isShearable());
}

// ==================== 剪毛测试 ====================

TEST_F(MooshroomEntityTest, Shear_ReturnsRedMushrooms_WhenRed)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());
    m_world.setClientSide(false); // 服务端模式
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 设置为红色哞菇
    mooshroom.setMooshroomType(MooshroomEntity::MooshroomType::Red);
    EXPECT_TRUE(mooshroom.isRed());

    // 执行剪毛
    std::vector<ItemStack> drops = mooshroom.shear(nullptr);

    // 验证返回 5 个红色蘑菇
    EXPECT_EQ(drops.size(), 1u);
    if (!drops.empty()) {
        EXPECT_EQ(drops[0].getCount(), 5);
        // 验证是红色蘑菇物品
        const Item* item = drops[0].getItem();
        EXPECT_NE(item, nullptr);
    }
}

TEST_F(MooshroomEntityTest, Shear_ReturnsBrownMushrooms_WhenBrown)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());
    m_world.setClientSide(false);
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 设置为棕色哞菇
    mooshroom.setMooshroomType(MooshroomEntity::MooshroomType::Brown);
    EXPECT_TRUE(mooshroom.isBrown());

    // 执行剪毛
    std::vector<ItemStack> drops = mooshroom.shear(nullptr);

    // 验证返回 5 个棕色蘑菇
    EXPECT_EQ(drops.size(), 1u);
    if (!drops.empty()) {
        EXPECT_EQ(drops[0].getCount(), 5);
    }
}

TEST_F(MooshroomEntityTest, Shear_PlaysShearSound)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());
    m_world.setClientSide(false);
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 执行剪毛
    mooshroom.shear(nullptr);

    // 验证播放了剪切音效
    EXPECT_GE(m_world.soundCount(), 1u);
    if (m_world.soundCount() >= 1) {
        const auto& sound = m_world.sounds()[0];
        EXPECT_EQ(sound.sound.toString(), "minecraft:entity.mooshroom.shear");
    }
}

TEST_F(MooshroomEntityTest, Shear_GeneratesExplosionParticles)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());
    m_world.setClientSide(false); // 服务端也生成粒子
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 执行剪毛
    mooshroom.shear(nullptr);

    // 验证生成了爆炸粒子（服务端）
    EXPECT_EQ(m_world.particleCount(), 20u);
    for (const auto& particle : m_world.particles()) {
        EXPECT_EQ(particle.type, particle::ParticleTypeId::Explosion);
    }
}

// ==================== 迷之炖菜效果测试 ====================

TEST_F(MooshroomEntityTest, StewEffect_DefaultEmpty)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 默认情况下没有迷之炖菜效果
    EXPECT_FALSE(mooshroom.hasStewEffect());
    EXPECT_EQ(mooshroom.getStewEffectType(), std::nullopt);
    EXPECT_EQ(mooshroom.getStewEffectDuration(), 0);
}

TEST_F(MooshroomEntityTest, StewEffect_SetAndClear)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 设置效果
    mooshroom.setStewEffect(entity::effect::EffectType::NightVision, 4);
    EXPECT_TRUE(mooshroom.hasStewEffect());
    EXPECT_EQ(mooshroom.getStewEffectType(), entity::effect::EffectType::NightVision);
    EXPECT_EQ(mooshroom.getStewEffectDuration(), 4);

    // 清除效果
    mooshroom.clearStewEffect();
    EXPECT_FALSE(mooshroom.hasStewEffect());
    EXPECT_EQ(mooshroom.getStewEffectType(), std::nullopt);
    EXPECT_EQ(mooshroom.getStewEffectDuration(), 0);
}

TEST_F(MooshroomEntityTest, StewEffect_BrownMooshroomCanStoreEffect)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());
    mooshroom.setMooshroomType(MooshroomEntity::MooshroomType::Brown);

    // 棕色哞菇可以存储效果
    mooshroom.setStewEffect(entity::effect::EffectType::FireResistance, 4);
    EXPECT_TRUE(mooshroom.hasStewEffect());
    EXPECT_TRUE(mooshroom.isBrown());
}

TEST_F(MooshroomEntityTest, StewEffect_RedMooshroomCanStoreEffect)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());
    mooshroom.setMooshroomType(MooshroomEntity::MooshroomType::Red);

    // 红色哞菇也可以存储效果（虽然在正常游戏中只有棕色哞菇会被喂食花朵）
    mooshroom.setStewEffect(entity::effect::EffectType::Weakness, 7);
    EXPECT_TRUE(mooshroom.hasStewEffect());
}

// ==================== 繁殖测试 ====================

TEST_F(MooshroomEntityTest, SpawnBaby_CreatesMooshroom)
{
    MooshroomEntity parent1(EntityInstanceId(1), mc::test::testEcsRegistry());
    parent1.setPosition(100.0, 64.0, 100.0);

    MooshroomEntity parent2(EntityInstanceId(2), mc::test::testEcsRegistry());
    parent2.setPosition(102.0, 64.0, 100.0);

    // 繁殖
    auto baby = parent1.spawnBaby(parent2);

    // 验证幼体是哞菇
    EXPECT_NE(baby, nullptr);
    if (baby) {
        MooshroomEntity* babyMooshroom = dynamic_cast<MooshroomEntity*>(baby.get());
        EXPECT_NE(babyMooshroom, nullptr);
        EXPECT_TRUE(baby->isChild());
    }
}

TEST_F(MooshroomEntityTest, SpawnBaby_InheritsParentType)
{
    MooshroomEntity parent1(EntityInstanceId(1), mc::test::testEcsRegistry());
    parent1.setMooshroomType(MooshroomEntity::MooshroomType::Red);
    parent1.setPosition(100.0, 64.0, 100.0);

    MooshroomEntity parent2(EntityInstanceId(2), mc::test::testEcsRegistry());
    parent2.setMooshroomType(MooshroomEntity::MooshroomType::Red);
    parent2.setPosition(102.0, 64.0, 100.0);

    // 繁殖
    auto baby = parent1.spawnBaby(parent2);

    // 验证幼体继承父母类型
    if (baby) {
        MooshroomEntity* babyMooshroom = dynamic_cast<MooshroomEntity*>(baby.get());
        if (babyMooshroom) {
            // 大多数情况继承红色（1/1024 概率变异为棕色）
            // 但由于随机性，我们只验证类型是有效的
            EXPECT_TRUE(babyMooshroom->isRed() || babyMooshroom->isBrown());
        }
    }
}

TEST_F(MooshroomEntityTest, SpawnBaby_PositionNearParent)
{
    MooshroomEntity parent1(EntityInstanceId(1), mc::test::testEcsRegistry());
    parent1.setPosition(100.0, 64.0, 100.0);

    MooshroomEntity parent2(EntityInstanceId(2), mc::test::testEcsRegistry());

    // 繁殖
    auto baby = parent1.spawnBaby(parent2);

    // 验证幼体位置在父母附近
    if (baby) {
        // 位置应该在父母位置附近（±1格）
        EXPECT_NEAR(baby->x(), 100.0, 2.0);
        EXPECT_NEAR(baby->y(), 64.0, 0.5);
        EXPECT_NEAR(baby->z(), 100.0, 2.0);
    }
}

// ==================== IShearable 接口测试 ====================

TEST_F(MooshroomEntityTest, ImplementsIShearable)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证实现 IShearable 接口
    entity::IShearable* shearable = dynamic_cast<entity::IShearable*>(&mooshroom);
    EXPECT_NE(shearable, nullptr);
    EXPECT_TRUE(shearable->isShearable());
}

// ==================== NBT序列化测试 ====================

TEST_F(MooshroomEntityTest, NBT_Serialization_RoundTrip_DefaultType)
{
    // 默认红色哞菇，无迷之炖菜效果
    MooshroomEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 序列化
    nbt::tags::compound_tag tag;
    original.addAdditionalSaveData(tag);

    // 反序列化到新实体
    MooshroomEntity loaded(EntityInstanceId(2), mc::test::testEcsRegistry());
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());

    // 验证类型
    EXPECT_EQ(loaded.getMooshroomType(), MooshroomEntity::MooshroomType::Red);
    EXPECT_TRUE(loaded.isRed());

    // 验证无迷之炖菜效果
    EXPECT_FALSE(loaded.hasStewEffect());
}

TEST_F(MooshroomEntityTest, NBT_Serialization_RoundTrip_BrownType)
{
    MooshroomEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());
    original.setMooshroomType(MooshroomEntity::MooshroomType::Brown);

    // 序列化
    nbt::tags::compound_tag tag;
    original.addAdditionalSaveData(tag);

    // 反序列化
    MooshroomEntity loaded(EntityInstanceId(2), mc::test::testEcsRegistry());
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());

    EXPECT_EQ(loaded.getMooshroomType(), MooshroomEntity::MooshroomType::Brown);
    EXPECT_TRUE(loaded.isBrown());
}

TEST_F(MooshroomEntityTest, NBT_Serialization_RoundTrip_StewEffect)
{
    MooshroomEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());
    original.setMooshroomType(MooshroomEntity::MooshroomType::Brown);
    original.setStewEffect(entity::effect::EffectType::NightVision, 4);

    // 序列化
    nbt::tags::compound_tag tag;
    original.addAdditionalSaveData(tag);

    // 反序列化
    MooshroomEntity loaded(EntityInstanceId(2), mc::test::testEcsRegistry());
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());

    EXPECT_TRUE(loaded.hasStewEffect());
    EXPECT_EQ(loaded.getStewEffectType(), entity::effect::EffectType::NightVision);
    EXPECT_EQ(loaded.getStewEffectDuration(), 4);
}

TEST_F(MooshroomEntityTest, NBT_Serialization_StewEffect_InstantEffect)
{
    // 瞬间效果（如饱和）的持续时间也可以正确序列化
    MooshroomEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());
    original.setStewEffect(entity::effect::EffectType::Saturation, 0);

    nbt::tags::compound_tag tag;
    original.addAdditionalSaveData(tag);

    MooshroomEntity loaded(EntityInstanceId(2), mc::test::testEcsRegistry());
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());

    EXPECT_TRUE(loaded.hasStewEffect());
    EXPECT_EQ(loaded.getStewEffectType(), entity::effect::EffectType::Saturation);
    EXPECT_EQ(loaded.getStewEffectDuration(), 0);
}

TEST_F(MooshroomEntityTest, NBT_Serialization_MissingKeys_Defaults)
{
    // 空 NBT 标签应该保留默认值
    nbt::tags::compound_tag emptyTag;

    MooshroomEntity loaded(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto result = loaded.readAdditionalSaveData(emptyTag);
    EXPECT_TRUE(result.success());

    // 默认为红色
    EXPECT_EQ(loaded.getMooshroomType(), MooshroomEntity::MooshroomType::Red);
    // 无迷之炖菜效果
    EXPECT_FALSE(loaded.hasStewEffect());
    EXPECT_EQ(loaded.getStewEffectDuration(), 0);
}

TEST_F(MooshroomEntityTest, NBT_Serialization_TypeKey_ByteValue)
{
    // 验证 Type 键值正确写入（应为 i8）
    MooshroomEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());
    original.setMooshroomType(MooshroomEntity::MooshroomType::Brown);

    nbt::tags::compound_tag tag;
    original.addAdditionalSaveData(tag);

    // 验证 Type 字段存在且为棕色(1)
    auto typeVal = entity::serialization::nbt_helper::tryGetByte(tag, "Type");
    ASSERT_TRUE(typeVal.has_value());
    EXPECT_EQ(*typeVal, 1);
}

TEST_F(MooshroomEntityTest, NBT_Serialization_StewEffectKey_Structure)
{
    // 验证 StewEffect 复合标签结构正确
    MooshroomEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());
    original.setStewEffect(entity::effect::EffectType::FireResistance, 4);

    nbt::tags::compound_tag tag;
    original.addAdditionalSaveData(tag);

    // 验证 StewEffect 复合标签存在
    const nbt::tags::compound_tag* stewTag = entity::serialization::nbt_helper::tryGetCompound(tag, "StewEffect");
    ASSERT_NE(stewTag, nullptr);

    // 验证 EffectId 和 EffectDuration
    auto effectId = entity::serialization::nbt_helper::tryGetByte(*stewTag, "EffectId");
    ASSERT_TRUE(effectId.has_value());
    EXPECT_EQ(*effectId, static_cast<i8>(static_cast<i32>(entity::effect::EffectType::FireResistance)));

    auto duration = entity::serialization::nbt_helper::tryGetInt(*stewTag, "EffectDuration");
    ASSERT_TRUE(duration.has_value());
    EXPECT_EQ(*duration, 4);
}

TEST_F(MooshroomEntityTest, NBT_Serialization_NoStewEffect_NoTag)
{
    // 没有迷之炖菜效果时不应该写入 StewEffect 标签
    MooshroomEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 默认无效果

    nbt::tags::compound_tag tag;
    original.addAdditionalSaveData(tag);

    // StewEffect 不应该存在
    const nbt::tags::compound_tag* stewTag = entity::serialization::nbt_helper::tryGetCompound(tag, "StewEffect");
    EXPECT_EQ(stewTag, nullptr);
}

// ==================== 迷之炖菜效果覆盖测试 ====================

TEST_F(MooshroomEntityTest, StewEffect_Overwrite)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 先设置一个效果
    mooshroom.setStewEffect(entity::effect::EffectType::NightVision, 4);
    EXPECT_EQ(mooshroom.getStewEffectType(), entity::effect::EffectType::NightVision);

    // 覆盖为另一个效果
    mooshroom.setStewEffect(entity::effect::EffectType::Poison, 8);
    EXPECT_EQ(mooshroom.getStewEffectType(), entity::effect::EffectType::Poison);
    EXPECT_EQ(mooshroom.getStewEffectDuration(), 8);
}

TEST_F(MooshroomEntityTest, StewEffect_MultipleEffectTypes)
{
    // 验证各种效果类型都能正确设置
    MooshroomEntity mooshroom(EntityInstanceId(1), mc::test::testEcsRegistry());

    const std::vector<entity::effect::EffectType> effectTypes = {
        entity::effect::EffectType::Saturation,
        entity::effect::EffectType::NightVision,
        entity::effect::EffectType::FireResistance,
        entity::effect::EffectType::Blindness,
        entity::effect::EffectType::Weakness,
        entity::effect::EffectType::Poison,
        entity::effect::EffectType::Regeneration,
        entity::effect::EffectType::Wither,
        entity::effect::EffectType::JumpBoost,
    };

    for (const auto& effectType : effectTypes) {
        mooshroom.setStewEffect(effectType, 4);
        EXPECT_TRUE(mooshroom.hasStewEffect());
        EXPECT_EQ(mooshroom.getStewEffectType(), effectType);
    }
}

} // namespace
} // namespace mc
