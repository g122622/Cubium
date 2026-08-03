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

#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/particles/block/DiggingParticle.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 尘柱粒子（DustPillar）
 *
 * 对应 MC Java 的 TerrainParticle.DustPillarProvider。
 * 用于重锤砸地攻击（Mace Smash Attack）等场景，生成携带方块状态纹理的尘柱粒子。
 *
 * 特性：
 * - 继承 DiggingParticle 的方块纹理渲染能力（TERRAIN_SHEET 渲染类型）
 * - 重力 1.0（粒子会先上升后下落）
 * - 构造时重写速度：水平分量替换为高斯分布 sigma=1/30（极低水平扩散），
 *   垂直分量保留传入的 Y 分量并叠加高斯偏移 sigma=0.5（先扬后抑的抛物线）
 * - 生命周期 20-40 tick（1-2 秒）
 * - 颜色继承自方块染色（乘以 0.6 基础亮度）
 */
class DustPillarParticle : public DiggingParticle {
public:
    /**
     * @brief 构造尘柱粒子
     *
     * 构造时会重写速度（匹配 MC Java DustPillarProvider 行为）：
     * - X/Z 速度替换为 nextGaussian() / 30.0
     * - Y 速度保留传入值并叠加 nextGaussian() / 2.0
     *
     * @param pos 初始位置
     * @param velocity 初始速度（Y 分量用于控制上升高度）
     * @param blockState 方块状态（用于纹理和颜色）
     */
    DustPillarParticle(const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState);

    /**
     * @brief 默认工厂方法（使用石头方块状态，不推荐）
     *
     * TODO: 当方块状态可通过世界查询自动获取时，应移除此默认回退，
     * 改为始终使用 createWithBlock 传入正确的方块状态
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 带方块状态的工厂方法（推荐）
     */
    static std::unique_ptr<Particle> createWithBlock(
        const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState);

    void tick(mc::client::ClientWorld* world) override;

private:
    /// 尘柱粒子的基础重力（与 DiggingParticle 的 0.03 不同，使用 1.0 实现先扬后抑）
    static constexpr f64 DUST_PILLAR_GRAVITY = 1.0;
};

} // namespace mc::client::renderer::trident::particle::particles
