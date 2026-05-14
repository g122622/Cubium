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

#include "../../Particle.hpp"
#include "client/renderer/MeshTypes.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/Block.hpp"

namespace mc {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 挖掘粒子（破坏方块时产生）
 *
 * 参考 MC 1.16.5 DiggingParticle / TerrainParticle
 *
 * 特性：
 * - 使用方块纹理（从 BlockModelCache 获取）
 * - 从 16x16 纹理中随机选取 4x4 区域（模拟 MC 的 field_217587_G/field_217588_H）
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
     * 对于 TERRAIN_SHEET 类型粒子，此方法返回的路径用于
     * 在方块纹理图集中查找 UV 坐标。
     * 实际渲染使用 buildVertices 中预计算的 m_textureRegion。
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

    /**
     * @brief 初始化方块纹理
     *
     * 从 BlockModelCache 获取方块的纹理 UV 坐标。
     * 如果获取失败，使用默认纹理。
     */
    void initializeBlockTexture();
};

} // namespace mc::client::renderer::trident::particle::particles
