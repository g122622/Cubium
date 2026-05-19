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

#include "DiggingParticle.hpp"
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "client/resource/BlockModelCache.hpp"
#include "client/resource/ResourceManager.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include <optional>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident::particle::particles {

namespace {

/**
 * @brief 默认石头纹理 UV 坐标
 *
 * 如果无法获取方块纹理，使用此回退值。
 * 这是方块纹理图集中石头纹理的大致位置。
 * 实际值需要在运行时从 BlockModelCache 获取。
 */
constexpr f64 DEFAULT_U0 = 0.0;
constexpr f64 DEFAULT_V0 = 0.0;
constexpr f64 DEFAULT_U1 = 1.0;
constexpr f64 DEFAULT_V1 = 1.0;

/**
 * @brief 从 BlockAppearance 中随机选择一个面的纹理
 *
 * @param appearance 方块外观
 * @param rng 随机数生成器
 * @return 纹理区域，如果没有可用纹理返回 nullopt
 */
std::optional<TextureRegion> selectRandomFaceTexture(const BlockAppearance* appearance, math::Random& rng)
{
    if (!appearance || appearance->faceTextures.empty()) {
        return std::nullopt;
    }

    // 收集所有可用的面名称
    std::vector<std::string> faceNames;
    for (const auto& [name, region] : appearance->faceTextures) {
        if (!name.empty()) {
            faceNames.push_back(name);
        }
    }

    if (faceNames.empty()) {
        return std::nullopt;
    }

    // 随机选择一个面
    size_t randomIndex = static_cast<size_t>(rng.nextInt(static_cast<i32>(faceNames.size())));
    const std::string& selectedFace = faceNames[randomIndex];

    auto it = appearance->faceTextures.find(selectedFace);
    if (it != appearance->faceTextures.end()) {
        return it->second;
    }

    return std::nullopt;
}

} // namespace

DiggingParticle::DiggingParticle(const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState)
    : Particle(pos, velocity)
    , m_blockState(blockState)
{
    math::Random rng;

    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.5f + rng.nextFloat() * 0.5f));
    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // 使用纹理原色
    setFriction(0.92f);
    setHasPhysics(true); // 方块粒子有物理碰撞
    setMaxAge(DEFAULT_LIFETIME * (0.8f + rng.nextFloat() * 0.4f));

    // 随机 UV 偏移：将 16x16 纹理划分为 4x4 区域，随机选取一个
    // 参考 MC 1.16.5 DiggingParticle 的 field_217587_G 和 field_217588_H
    m_uvOffsetU = static_cast<f32>(rng.nextInt(4)); // 0, 1, 2, 或 3
    m_uvOffsetV = static_cast<f32>(rng.nextInt(4));

    // 初始化方块纹理
    initializeBlockTexture();
}

std::unique_ptr<Particle> DiggingParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 默认使用石头方块状态
    // 实际使用时应通过 createWithBlock 创建
    static const BlockState* defaultStoneState = nullptr;
    if (!defaultStoneState) {
        // 尝试获取石头方块状态
        if (VanillaBlocks::STONE != nullptr) {
            defaultStoneState = &(VanillaBlocks::STONE->defaultState());
        }
    }

    if (defaultStoneState != nullptr) {
        return std::make_unique<DiggingParticle>(pos, velocity, *defaultStoneState);
    }

    // 如果石头状态不可用，返回 nullptr
    return nullptr;
}

std::unique_ptr<Particle> DiggingParticle::createWithBlock(
    const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState)
{
    return std::make_unique<DiggingParticle>(pos, velocity, blockState);
}

void DiggingParticle::tick(mc::client::ClientWorld* world)
{
    m_prevPosition = m_position;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= static_cast<f32>(m_gravity * 0.04);

    // 随机旋转
    m_roll += 0.1;

    // 移动并碰撞
    move(world, m_velocity);

    // 应用阻力
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // 地面摩擦
    if (onGround()) {
        m_velocity.x *= 0.7f;
        m_velocity.z *= 0.7f;
    }

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.7) {
        m_color.a = static_cast<f32>(1.0 - (lifeRatio - 0.7) / 0.3);
    }
}

ResourceLocation DiggingParticle::getTextureLocation() const
{
    // 对于 TERRAIN_SHEET 类型粒子，返回方块纹理路径
    // 实际渲染使用 buildVertices 中预计算的 m_textureRegion
    return ResourceLocation("minecraft:block/stone");
}

void DiggingParticle::buildVertices(const glm::vec3& cameraPos,
    f64 partialTick,
    const ParticleTextureAtlas& /*atlas*/,
    std::vector<ParticleVertex>& outVertices) const
{
    // 插值位置
    glm::dvec3 interpPos =
        glm::dvec3(m_prevPosition) + (glm::dvec3(m_position) - glm::dvec3(m_prevPosition)) * partialTick;

    // 插值旋转
    f64 interpRoll = m_prevRoll + (m_roll - m_prevRoll) * partialTick;

    // 计算 billboard 基向量
    glm::dvec3 toCamera = glm::dvec3(cameraPos) - interpPos;
    f64 dist = glm::length(toCamera);
    if (dist < 0.001) {
        return; // 太近了，跳过
    }
    toCamera = glm::normalize(toCamera);

    // 计算右向量和上向量（camera-facing billboard）
    glm::dvec3 right = glm::cross(glm::dvec3(0.0, 1.0, 0.0), toCamera);
    if (glm::length(right) < 0.001) {
        // 相机在正上方或正下方
        right = glm::dvec3(1.0, 0.0, 0.0);
    } else {
        right = glm::normalize(right);
    }
    glm::dvec3 up = glm::cross(toCamera, right);

    // 应用旋转
    if (std::abs(interpRoll) > 0.001) {
        f64 cosR = std::cos(interpRoll);
        f64 sinR = std::sin(interpRoll);
        glm::dvec3 newRight = right * cosR + up * sinR;
        glm::dvec3 newUp = -right * sinR + up * cosR;
        right = newRight;
        up = newUp;
    }

    glm::vec3 interpPosF(static_cast<f32>(interpPos.x), static_cast<f32>(interpPos.y), static_cast<f32>(interpPos.z));
    glm::vec3 rightF(static_cast<f32>(right.x), static_cast<f32>(right.y), static_cast<f32>(right.z));
    glm::vec3 upF(static_cast<f32>(up.x), static_cast<f32>(up.y), static_cast<f32>(up.z));

    // 获取缩放
    f64 scale = getScale(partialTick);
    f64 halfSize = m_size * scale * 0.5;
    const f32 halfSizeF = static_cast<f32>(halfSize);

    // 计算 UV 坐标
    // MC 1.16.5 DiggingParticle 从 16x16 纹理中选取 4x4 区域
    // m_uvOffsetU/V 在 0-3 范围内，每个偏移代表 4 像素
    // UV 坐标归一化到 0-1 范围
    f64 u0, v0, u1, v1;

    if (m_hasValidTexture) {
        // 使用预计算的纹理区域
        // 纹理区域是整个 16x16 纹理的 UV 坐标
        // 我们需要从中选取一个 4x4 的子区域
        // 4x4 子区域占整个纹理的 1/4
        f64 regionWidth = m_textureRegion.u1 - m_textureRegion.u0;
        f64 regionHeight = m_textureRegion.v1 - m_textureRegion.v0;

        // 计算子区域的起始位置
        f64 subU0 = m_textureRegion.u0 + (regionWidth * m_uvOffsetU / 4.0);
        f64 subV0 = m_textureRegion.v0 + (regionHeight * m_uvOffsetV / 4.0);
        f64 subU1 = subU0 + regionWidth / 4.0;
        f64 subV1 = subV0 + regionHeight / 4.0;

        u0 = subU0;
        v0 = subV0;
        u1 = subU1;
        v1 = subV1;
    } else {
        // 使用默认全纹理
        u0 = DEFAULT_U0;
        v0 = DEFAULT_V0;
        u1 = DEFAULT_U1;
        v1 = DEFAULT_V1;
    }

    // 四个顶点（quad）
    // 左下
    outVertices.push_back({interpPosF - rightF * halfSizeF - upF * halfSizeF,
        glm::vec2(static_cast<f32>(u0), static_cast<f32>(v1)), // UV: 左下
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a});
    // 右下
    outVertices.push_back({interpPosF + rightF * halfSizeF - upF * halfSizeF,
        glm::vec2(static_cast<f32>(u1), static_cast<f32>(v1)), // UV: 右下
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a});
    // 右上
    outVertices.push_back({interpPosF + rightF * halfSizeF + upF * halfSizeF,
        glm::vec2(static_cast<f32>(u1), static_cast<f32>(v0)), // UV: 右上
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a});
    // 左上
    outVertices.push_back({interpPosF - rightF * halfSizeF + upF * halfSizeF,
        glm::vec2(static_cast<f32>(u0), static_cast<f32>(v0)), // UV: 左上
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a});
}

void DiggingParticle::initializeBlockTexture()
{
    // 获取 BlockModelCache
    BlockModelCache* modelCache = ChunkMesher::modelCache();
    if (!modelCache) {
        spdlog::warn("DiggingParticle: BlockModelCache not available, using default texture");
        m_hasValidTexture = false;
        return;
    }

    // 获取方块外观
    const BlockAppearance* appearance = modelCache->getBlockAppearance(&m_blockState);
    if (!appearance) {
        // 尝试使用缺失模型外观
        appearance = modelCache->getMissingAppearance();
        if (!appearance) {
            spdlog::debug("DiggingParticle: No appearance found for block state, using default texture");
            m_hasValidTexture = false;
            return;
        }
    }

    // 随机选择一个面的纹理
    math::Random rng;
    auto textureRegion = selectRandomFaceTexture(appearance, rng);

    if (textureRegion.has_value()) {
        m_textureRegion = textureRegion.value();
        m_hasValidTexture = true;
    } else {
        m_hasValidTexture = false;
    }
}

} // namespace mc::client::renderer::trident::particle::particles
