#include <gtest/gtest.h>

#include "common/entity/entities/passive/basic/MooshroomEntity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"
#include "common/sound/SoundEvents.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用世界实现
 *
 * 提供哞菇测试所需的最小 IWorld 接口实现，支持客户端模式和粒子记录。
 */
class MooshroomTestWorld final : public IWorld {
public:
    MooshroomTestWorld() : m_isClientSide(false), m_random(12345) {}

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 0; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }

    // 客户端/服务端模式控制
    [[nodiscard]] bool isClientSide() override { return m_isClientSide; }
    void setClientSide(bool clientSide) { m_isClientSide = clientSide; }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    // TickManager interface
    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("MooshroomTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("MooshroomTestWorld::tickManager not implemented");
    }

    // Random interface
    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    // WorldBorder interface
    [[nodiscard]] world::border::WorldBorder& worldBorder() override {
        throw std::runtime_error("MooshroomTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override {
        throw std::runtime_error("MooshroomTestWorld::worldBorder not implemented");
    }

    // 粒子和音效数据结构
    struct ParticleInfo {
        client::renderer::trident::particle::ParticleTypeId type;
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
    void addParticle(
        client::renderer::trident::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity) override {
        m_particles.push_back({type, pos, velocity});
    }

    void addParticle(
        client::renderer::trident::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count) override {
        (void)offset;
        (void)count;
        m_particles.push_back({type, pos, velocity});
    }

    // 音效播放记录
    void playSound(
        const ResourceLocation& sound,
        sound::SoundCategory category,
        const Vector3& pos,
        f32 volume,
        f32 pitch) override {
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
    bool m_isClientSide;
    math::Random m_random;
};

class MooshroomEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }

    MooshroomTestWorld m_world;
};

// ==================== 类型系统测试 ====================

TEST_F(MooshroomEntityTest, DefaultType_IsRed) {
    MooshroomEntity mooshroom(LegacyEntityType::Mooshroom, EntityId(1));
    EXPECT_EQ(mooshroom.getMooshroomType(), MooshroomEntity::MooshroomType::Red);
    EXPECT_TRUE(mooshroom.isRed());
    EXPECT_FALSE(mooshroom.isBrown());
}

TEST_F(MooshroomEntityTest, SetMooshroomType_ChangesType) {
    MooshroomEntity mooshroom(LegacyEntityType::Mooshroom, EntityId(1));

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

TEST_F(MooshroomEntityTest, OnStruckByLightning_RedToBrown) {
    MooshroomEntity mooshroom(LegacyEntityType::Mooshroom, EntityId(1));
    m_world.setClientSide(false);  // 服务端不生成粒子
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 初始为红色
    mooshroom.setMooshroomType(MooshroomEntity::MooshroomType::Red);
    EXPECT_TRUE(mooshroom.isRed());

    // 雷击后变为棕色
    mooshroom.onStruckByLightning();
    EXPECT_TRUE(mooshroom.isBrown());
    EXPECT_EQ(mooshroom.getMooshroomType(), MooshroomEntity::MooshroomType::Brown);
}

TEST_F(MooshroomEntityTest, OnStruckByLightning_BrownToRed) {
    MooshroomEntity mooshroom(LegacyEntityType::Mooshroom, EntityId(1));
    m_world.setClientSide(false);
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 初始为棕色
    mooshroom.setMooshroomType(MooshroomEntity::MooshroomType::Brown);
    EXPECT_TRUE(mooshroom.isBrown());

    // 雷击后变为红色
    mooshroom.onStruckByLightning();
    EXPECT_TRUE(mooshroom.isRed());
    EXPECT_EQ(mooshroom.getMooshroomType(), MooshroomEntity::MooshroomType::Red);
}

TEST_F(MooshroomEntityTest, OnStruckByLightning_PlaysConvertSound) {
    MooshroomEntity mooshroom(LegacyEntityType::Mooshroom, EntityId(1));
    m_world.setClientSide(false);
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 执行雷击
    mooshroom.onStruckByLightning();

    // 验证播放了转换音效
    EXPECT_EQ(m_world.soundCount(), 1u);
    const auto& sound = m_world.sounds()[0];
    EXPECT_EQ(sound.sound.toString(), "minecraft:entity.mooshroom.convert");
    EXPECT_FLOAT_EQ(sound.volume, 2.0f);
    EXPECT_FLOAT_EQ(sound.pitch, 1.0f);
}

TEST_F(MooshroomEntityTest, OnStruckByLightning_GeneratesParticles_ClientSide) {
    MooshroomEntity mooshroom(LegacyEntityType::Mooshroom, EntityId(1));
    m_world.setClientSide(true);  // 客户端生成粒子
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 执行雷击
    mooshroom.onStruckByLightning();

    // 验证生成了爆炸粒子
    // 参考 MC 1.16.5: 生成 20 个 Explosion 粒子
    EXPECT_EQ(m_world.particleCount(), 20u);

    // 验证粒子类型
    for (const auto& particle : m_world.particles()) {
        EXPECT_EQ(particle.type, client::renderer::trident::particle::ParticleTypeId::Explosion);
    }
}

TEST_F(MooshroomEntityTest, OnStruckByLightning_NoParticles_ServerSide) {
    MooshroomEntity mooshroom(LegacyEntityType::Mooshroom, EntityId(1));
    m_world.setClientSide(false);  // 服务端不生成粒子
    mooshroom.setWorld(&m_world);
    mooshroom.setPosition(100.0, 64.0, 100.0);

    // 执行雷击
    mooshroom.onStruckByLightning();

    // 服务端不应该生成粒子
    EXPECT_EQ(m_world.particleCount(), 0u);

    // 但类型应该改变
    EXPECT_TRUE(mooshroom.isBrown());
}

TEST_F(MooshroomEntityTest, OnStruckByLightning_ParticlePosition_WithinEntityBounds) {
    MooshroomEntity mooshroom(LegacyEntityType::Mooshroom, EntityId(1));
    m_world.setClientSide(true);
    mooshroom.setWorld(&m_world);

    const f64 posX = 100.0;
    const f64 posY = 64.0;
    const f64 posZ = 200.0;
    mooshroom.setPosition(posX, posY, posZ);

    // 执行雷击
    mooshroom.onStruckByLightning();

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

TEST_F(MooshroomEntityTest, InheritsFromCowEntity) {
    MooshroomEntity mooshroom(LegacyEntityType::Mooshroom, EntityId(1));

    // 验证哞菇继承自牛
    CowEntity* cow = dynamic_cast<CowEntity*>(&mooshroom);
    EXPECT_NE(cow, nullptr);

    // 验证哞菇实现 IShearable 接口
    entity::IShearable* shearable = dynamic_cast<entity::IShearable*>(&mooshroom);
    EXPECT_NE(shearable, nullptr);
}

TEST_F(MooshroomEntityTest, IsShearable_ReturnsTrue) {
    MooshroomEntity mooshroom(LegacyEntityType::Mooshroom, EntityId(1));

    // 哞菇总是可以被剪毛
    EXPECT_TRUE(mooshroom.isShearable());
}

} // namespace
} // namespace mc
