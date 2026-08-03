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

#pragma once

#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/ParticleRenderType.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>
#include <vector>
#include <glm/ext/vector_float3.hpp>

namespace mc {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 挖掘粒子（破坏方块时产生）
 *
 * 特性：
 * - 使用方块纹理（从 BlockModelCache 获取）
 * - 优先使用模型中 textures.particle 指定的粒子纹理，
 *   若无则回退到随机选取一个面的纹理
 * - 从 16x16 纹理中随机选取 4x4 区域，模拟碎片效果
 * - 受重力影响
 * - 使用 TERRAIN_SHEET 渲染类型
 */
class DiggingParticle : public Particle {
public:
    /**
     * @brief 构造函数
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param blockState 方块状态（用于获取纹理）
     */
    DiggingParticle(const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState);

    /**
     * @brief 默认工厂方法（不推荐使用）
     *
     * 创建使用石头纹理的粒子。推荐使用 createWithBlock。
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 带方块状态的工厂方法
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param blockState 方块状态
     * @return 粒子实例
     */
    static std::unique_ptr<Particle> createWithBlock(
        const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::TERRAIN_SHEET; }

    /**
     * @brief 获取纹理位置
     *
     * 优先返回方块模型中 textures.particle 指定的纹理路径，
     * 若无粒子纹理则返回随机选中的面纹理路径，
     * 若均不可用则返回默认石头纹理路径。
     * 对于 TERRAIN_SHEET 类型粒子，实际渲染使用 buildVertices 中预计算的 m_textureRegion。
     */
    [[nodiscard]] ResourceLocation getTextureLocation() const override;

    /**
     * @brief 生成渲染顶点数据
     *
     * 重写以使用预计算的方块纹理 UV 坐标。
     */
    void buildVertices(const glm::vec3& cameraPos,
        f64 partialTick,
        const ParticleTextureAtlas& atlas,
        std::vector<ParticleVertex>& outVertices) const override;

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.03f;
    static constexpr f64 DEFAULT_SIZE = 0.1f;
    static constexpr f64 DEFAULT_LIFETIME = 20.0f;

    /// 方块状态
    BlockState m_blockState;

    /// 预计算的纹理 UV 区域
    TextureRegion m_textureRegion;

    /// 随机 UV 偏移（0-3），用于从 16x16 纹理中选取 4x4 区域
    f32 m_uvOffsetU = 0.0f;
    f32 m_uvOffsetV = 0.0f;

    /// 是否成功获取了方块纹理
    bool m_hasValidTexture = false;

    /// 纹理位置标识（用于 TERRAIN_SHEET 渲染类型）
    ResourceLocation m_textureLocation{"minecraft:block/stone"};

    /**
     * @brief 初始化方块纹理
     *
     * 从 BlockModelCache 获取方块的纹理 UV 坐标。
     * 如果获取失败，使用默认纹理。
     */
    void _initializeBlockTexture();
};

/**
 * @brief 方块标记粒子
 *
 * 用于结构方块显示的静态位置标记粒子。
 * 不移动、不受重力影响，仅在生命周期结束后淡出。
 * 使用方块纹理渲染，与 DiggingParticle 共享纹理逻辑。
 *
 * TODO: 粒子数据管线尚未支持BlockState传递，当前 create() 工厂方法使用默认值/回退行为。
 * 待 ParticleFactory 签名扩展后，应通过 createWithBlockState() 方法传递真实数据。
 */
class BlockMarkerParticle : public Particle {
public:
    /**
     * @brief 构造方块标记粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度（忽略）
     * @param blockState 方块状态（用于获取纹理）
     */
    BlockMarkerParticle(const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState);

    /**
     * @brief 默认工厂方法
     *
     * 创建使用石头纹理的标记粒子。推荐使用 createWithBlock。
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 带方块状态的工厂方法
     */
    static std::unique_ptr<Particle> createWithBlock(
        const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::TERRAIN_SHEET; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override;

    void buildVertices(const glm::vec3& cameraPos,
        f64 partialTick,
        const ParticleTextureAtlas& atlas,
        std::vector<ParticleVertex>& outVertices) const override;

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 80.0;

    /// 方块状态
    BlockState m_blockState;

    /// 预计算的纹理 UV 区域
    TextureRegion m_textureRegion;

    /// 随机 UV 偏移（0-3）
    f32 m_uvOffsetU = 0.0f;
    f32 m_uvOffsetV = 0.0f;

    /// 是否成功获取了方块纹理
    bool m_hasValidTexture = false;

    /// 纹理位置标识
    ResourceLocation m_textureLocation{"minecraft:block/stone"};

    /**
     * @brief 初始化方块纹理
     */
    void _initializeBlockTexture();
};

/**
 * @brief 方块碎裂粒子
 *
 * 比挖掘粒子更小、生命周期更短的方块碎片粒子。
 * 受重力影响，使用方块纹理渲染，与 DiggingParticle 共享纹理逻辑。
 *
 * TODO: 粒子数据管线尚未支持BlockState传递，当前 create() 工厂方法使用默认值/回退行为。
 * 待 ParticleFactory 签名扩展后，应通过 createWithBlockState() 方法传递真实数据。
 */
class BlockCrumbleParticle : public Particle {
public:
    /**
     * @brief 构造方块碎裂粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param blockState 方块状态（用于获取纹理）
     */
    BlockCrumbleParticle(const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState);

    /**
     * @brief 默认工厂方法
     *
     * 创建使用石头纹理的碎裂粒子。推荐使用 createWithBlock。
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 带方块状态的工厂方法
     */
    static std::unique_ptr<Particle> createWithBlock(
        const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::TERRAIN_SHEET; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override;

    void buildVertices(const glm::vec3& cameraPos,
        f64 partialTick,
        const ParticleTextureAtlas& atlas,
        std::vector<ParticleVertex>& outVertices) const override;

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.03;
    static constexpr f64 DEFAULT_SIZE = 0.05;
    static constexpr f64 DEFAULT_LIFETIME = 15.0;

    /// 方块状态
    BlockState m_blockState;

    /// 预计算的纹理 UV 区域
    TextureRegion m_textureRegion;

    /// 随机 UV 偏移（0-3）
    f32 m_uvOffsetU = 0.0f;
    f32 m_uvOffsetV = 0.0f;

    /// 是否成功获取了方块纹理
    bool m_hasValidTexture = false;

    /// 纹理位置标识
    ResourceLocation m_textureLocation{"minecraft:block/stone"};

    /**
     * @brief 初始化方块纹理
     */
    void _initializeBlockTexture();
};

} // namespace mc::client::renderer::trident::particle::particles
