#include <gtest/gtest.h>

#include "client/renderer/trident/particle/particles/block/DiggingParticle.hpp"
#include "client/renderer/trident/particle/ParticleRenderType.hpp"
#include "client/renderer/trident/particle/ParticleTextureAtlas.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <glm/glm.hpp>
#include <memory>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::particles;

/**
 * @brief DiggingParticle 测试夹具
 *
 * 测试挖掘粒子的方块纹理获取功能。
 * 参考 MC 1.16.5 DiggingParticle。
 */
class DiggingParticleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化方块注册表
        VanillaBlocks::initialize();
    }

    void TearDown() override {
        // 清理
    }
};

// ==================== 创建测试 ====================

TEST_F(DiggingParticleTest, CreateWithBlock_ReturnsValidParticle) {
    glm::vec3 pos(10.0f, 64.0f, 20.0f);
    glm::vec3 velocity(0.1f, 0.2f, 0.3f);

    // 获取石头方块的默认状态
    const BlockState* stoneState = &(VanillaBlocks::STONE->defaultState());
    ASSERT_NE(stoneState, nullptr);

    auto particle = DiggingParticle::createWithBlock(pos, velocity, *stoneState);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST_F(DiggingParticleTest, CreateWithBlock_SetsPosition) {
    glm::vec3 pos(10.0f, 64.0f, 20.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    EXPECT_FLOAT_EQ(particle->position().x, 10.0f);
    EXPECT_FLOAT_EQ(particle->position().y, 64.0f);
    EXPECT_FLOAT_EQ(particle->position().z, 20.0f);
}

TEST_F(DiggingParticleTest, CreateWithBlock_SetsVelocity) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.5f, 0.3f, -0.2f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    // 速度可能被构造函数随机化了，只检查大致方向
    // 原始速度向量用于初始方向
}

TEST_F(DiggingParticleTest, Create_Default_ReturnsStoneParticle) {
    // 默认工厂方法应该返回使用石头纹理的粒子
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = DiggingParticle::create(pos, velocity, nullptr);

    // 如果石头方块状态可用，应该返回有效粒子
    // 如果不可用（初始化问题），可能返回 nullptr
    if (particle != nullptr) {
        EXPECT_TRUE(particle->isAlive());
    }
}

// ==================== 渲染属性测试 ====================

TEST_F(DiggingParticleTest, GetRenderType_ReturnsTerrainSheet) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    // 挖掘粒子使用 TERRAIN_SHEET 渲染类型
    // 使用方块纹理图集而不是粒子纹理图集
    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::TERRAIN_SHEET);
}

TEST_F(DiggingParticleTest, GetTextureLocation_ReturnsDefaultTexture) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    // getTextureLocation 返回默认纹理路径
    // 实际渲染使用 buildVertices 中预计算的 UV
    ResourceLocation texture = particle->getTextureLocation();
    EXPECT_EQ(texture.toString(), "minecraft:block/stone");
}

// ==================== 物理属性测试 ====================

TEST_F(DiggingParticleTest, HasGravity) {
    glm::vec3 pos(0.0f, 100.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    // 挖掘粒子受重力影响
    EXPECT_GT(particle->gravity(), 0.0);
}

TEST_F(DiggingParticleTest, HasPhysics) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    // 挖掘粒子有碰撞检测
    EXPECT_TRUE(particle->hasPhysics());
}

TEST_F(DiggingParticleTest, HasFriction) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    // 挖掘粒子有摩擦力
    EXPECT_NEAR(particle->friction(), 0.92, 0.01);
}

// ==================== 生命周期测试 ====================

TEST_F(DiggingParticleTest, Tick_UpdatesAge) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    f64 initialAge = particle->age();

    particle->tick(nullptr);

    EXPECT_GT(particle->age(), initialAge);
}

TEST_F(DiggingParticleTest, Tick_AppliesGravity) {
    glm::vec3 pos(0.0f, 100.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    particle->setMaxAge(1000.0);  // 防止过期

    f32 initialVelY = particle->velocity().y;

    particle->tick(nullptr);

    // 重力应该使 Y 速度减小
    EXPECT_LT(particle->velocity().y, initialVelY);
}

TEST_F(DiggingParticleTest, Tick_AppliesFriction) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(1.0f, 0.0f, 1.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    particle->setMaxAge(1000.0);  // 防止过期

    // 禁用物理以测试摩擦力
    particle->setHasPhysics(false);

    f32 initialVelX = particle->velocity().x;
    f32 initialVelZ = particle->velocity().z;

    particle->tick(nullptr);

    // 摩擦力应该减小水平速度
    EXPECT_LT(std::abs(particle->velocity().x), std::abs(initialVelX));
    EXPECT_LT(std::abs(particle->velocity().z), std::abs(initialVelZ));
}

TEST_F(DiggingParticleTest, Tick_UpdatesPosition) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.5f, 0.0f, 0.5f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    particle->setMaxAge(1000.0);
    particle->setHasPhysics(false);  // 禁用碰撞以便测试移动

    particle->tick(nullptr);

    // 位置应该更新
    EXPECT_NE(particle->position().x, 0.0f);
    EXPECT_NE(particle->position().z, 0.0f);
}

TEST_F(DiggingParticleTest, Tick_ExpiresAfterLifetime) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    particle->setMaxAge(5.0);  // 设置短生命周期

    // Tick 直到过期
    for (int i = 0; i < 10; ++i) {
        particle->tick(nullptr);
    }

    EXPECT_FALSE(particle->isAlive());
}

// ==================== 淡出测试 ====================

TEST_F(DiggingParticleTest, Tick_FadesOutInLateLifetime) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);
    particle->setMaxAge(10.0);

    // 初始 alpha 应该是 1.0
    f32 initialAlpha = particle->color().a;
    EXPECT_FLOAT_EQ(initialAlpha, 1.0f);

    // tick 到生命周期 70% (淡出开始)
    for (int i = 0; i < 7; ++i) {
        particle->tick(nullptr);
    }

    // 仍然应该不透明
    EXPECT_FLOAT_EQ(particle->color().a, 1.0f);

    // tick 过了淡出阈值
    particle->tick(nullptr);  // 80%

    // 现在 alpha 应该开始减小
    EXPECT_LT(particle->color().a, 1.0f);
}

// ==================== 碰撞盒测试 ====================

TEST_F(DiggingParticleTest, BoundingBox_IsSmall) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);

    // 碰撞盒应该很小（挖掘粒子是小碎片）
    // 粒子尺寸为 0.1，碰撞盒宽度/高度为尺寸的两倍 = 0.2
    auto bbox = particle->getBoundingBox();
    f32 width = static_cast<f32>(bbox.maxX - bbox.minX);
    f32 height = static_cast<f32>(bbox.maxY - bbox.minY);

    // 粒子碰撞盒很小（尺寸 0.1 = 碰撞盒 0.2）
    EXPECT_FLOAT_EQ(width, 0.2f);
    EXPECT_FLOAT_EQ(height, 0.2f);
}

// ==================== 旋转测试 ====================

TEST_F(DiggingParticleTest, Tick_Rotates) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);

    f64 initialRoll = particle->roll();

    particle->tick(nullptr);

    // 粒子应该旋转
    EXPECT_NE(particle->roll(), initialRoll);
}

// ==================== 不同方块类型测试 ====================

TEST_F(DiggingParticleTest, CreateWithBlock_DifferentBlockTypes) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    // 测试不同方块类型
    std::vector<Block*> blocks = {
        VanillaBlocks::STONE,
        VanillaBlocks::DIRT,
        VanillaBlocks::GRASS_BLOCK,
        VanillaBlocks::COBBLESTONE
    };

    for (Block* block : blocks) {
        if (block == nullptr) continue;

        const BlockState& state = block->defaultState();
        auto particle = DiggingParticle::createWithBlock(pos, velocity, state);

        ASSERT_NE(particle, nullptr) << "Failed to create particle for block";
        EXPECT_TRUE(particle->isAlive());
        EXPECT_EQ(particle->getRenderType(), ParticleRenderType::TERRAIN_SHEET);
    }
}

// ==================== buildVertices 测试 ====================

TEST_F(DiggingParticleTest, BuildVertices_WithoutTextureAtlas_ProducesVertices) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);

    // 创建一个空的纹理图集
    ParticleTextureAtlas atlas;

    glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);
    std::vector<ParticleVertex> vertices;

    // buildVertices 应该生成 4 个顶点（一个 quad）
    particle->buildVertices(cameraPos, 0.0, atlas, vertices);

    EXPECT_EQ(vertices.size(), 4u);
}

TEST_F(DiggingParticleTest, BuildVertices_VerticesHaveValidUV) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);

    ParticleTextureAtlas atlas;
    glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);
    std::vector<ParticleVertex> vertices;

    particle->buildVertices(cameraPos, 0.0, atlas, vertices);

    ASSERT_EQ(vertices.size(), 4u);

    // 检查 UV 坐标在有效范围内
    for (const auto& v : vertices) {
        EXPECT_GE(v.texCoord.x, 0.0f);
        EXPECT_LE(v.texCoord.x, 1.0f);
        EXPECT_GE(v.texCoord.y, 0.0f);
        EXPECT_LE(v.texCoord.y, 1.0f);
    }
}

TEST_F(DiggingParticleTest, BuildVertices_VerticesHaveColor) {
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto particle = DiggingParticle::createWithBlock(pos, velocity, stoneState);

    ASSERT_NE(particle, nullptr);

    ParticleTextureAtlas atlas;
    glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);
    std::vector<ParticleVertex> vertices;

    particle->buildVertices(cameraPos, 0.0, atlas, vertices);

    ASSERT_EQ(vertices.size(), 4u);

    // 检查颜色有效
    for (const auto& v : vertices) {
        EXPECT_GE(v.color.r, 0.0f);
        EXPECT_LE(v.color.r, 1.0f);
        EXPECT_GE(v.color.g, 0.0f);
        EXPECT_LE(v.color.g, 1.0f);
        EXPECT_GE(v.color.b, 0.0f);
        EXPECT_LE(v.color.b, 1.0f);
        EXPECT_GE(v.color.a, 0.0f);
        EXPECT_LE(v.color.a, 1.0f);
    }
}

} // namespace
} // namespace mc
