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
#include "client/renderer/trident/particle/ParticleRenderType.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 发射器粒子基类
 *
 * 发射器粒子是不渲染的元粒子，在生命周期内生成其他粒子。
 * 用途：
 * - 巨型爆炸效果（生成多个爆炸粒子）
 * - 持续发射效果（火焰、烟雾等）
 * - 组合粒子效果
 *
 * 用法：
 * @code
 * class MyEmitter : public EmitterParticle {
 * public:
 *     void tick(ClientWorld* world) override {
 *         EmitterParticle::tick(world);
 *         if (shouldEmit()) {
 *             emit(world, ParticleTypeId::Flame, position(), velocity());
 *         }
 *     }
 * };
 * @endcode
 */
class EmitterParticle : public Particle {
public:
    /**
     * @brief 构造发射器粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param lifetime 生命周期（ticks）
     */
    EmitterParticle(const glm::vec3& pos, const glm::vec3& velocity, f64 lifetime);

    /**
     * @brief 构造发射器粒子（带发射数量）
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param lifetime 生命周期（ticks）
     * @param emitCount 总发射次数（0 = 无限）
     */
    EmitterParticle(const glm::vec3& pos, const glm::vec3& velocity, f64 lifetime, u32 emitCount);

    void tick(mc::client::ClientWorld* world) override;

    /**
     * @brief 发射器粒子不渲染
     */
    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::NO_RENDER; }

    /**
     * @brief 发射一个粒子
     *
     * 使用 Particle 基类的 emitCallback 发射新粒子。
     *
     * @param world 客户端世界
     * @param type 粒子类型
     * @param pos 位置
     * @param velocity 速度
     */
    void emit(mc::client::ClientWorld* world, ParticleTypeId type, const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 发射一个粒子（带随机偏移）
     *
     * @param world 客户端世界
     * @param type 粒子类型
     * @param center 中心位置
     * @param offset 最大偏移
     * @param baseVelocity 基础速度
     * @param velocitySpread 速度随机范围
     */
    void emitWithOffset(mc::client::ClientWorld* world,
        ParticleTypeId type,
        const glm::vec3& center,
        const glm::vec3& offset,
        const glm::vec3& baseVelocity,
        const glm::vec3& velocitySpread);

    /**
     * @brief 检查是否应该发射
     *
     * @return 是否应该发射粒子
     */
    [[nodiscard]] bool shouldEmit() const;

    /**
     * @brief 获取剩余发射次数
     */
    [[nodiscard]] u32 remainingEmitCount() const { return m_emitCount; }

    /**
     * @brief 获取发射间隔（ticks）
     */
    [[nodiscard]] u32 emitInterval() const { return m_emitInterval; }

    /**
     * @brief 设置发射间隔
     */
    void setEmitInterval(u32 interval) { m_emitInterval = interval; }

protected:
    u32 m_emitCount = 0;          ///< 剩余发射次数（0 = 无限）
    u32 m_emitInterval = 1;       ///< 发射间隔（ticks）
    u32 m_ticksSinceLastEmit = 0; ///< 上次发射后的 tick 数
};

/**
 * @brief 巨型爆炸发射器粒子
 *
 * 在短暂延迟后生成大型爆炸粒子，创造震撼的爆炸效果。
 */
class HugeExplosionEmitterParticle : public EmitterParticle {
public:
    /**
     * @brief 构造巨型爆炸发射器
     *
     * @param pos 爆炸位置
     * @param velocity 初始速度（通常为零）
     */
    HugeExplosionEmitterParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

private:
    static constexpr f64 EMITTER_LIFETIME = 8.0; // 爆炸延迟（约 0.4 秒）
    static constexpr u32 EMIT_DELAY = 2;         // 发射延迟（ticks）
};

/**
 * @brief 火焰发射器粒子
 *
 * 持续发射火焰粒子的发射器。
 */
class FlameEmitterParticle : public EmitterParticle {
public:
    /**
     * @brief 构造火焰发射器
     *
     * @param pos 位置
     * @param velocity 初始速度
     * @param lifetime 持续时间（ticks）
     * @param emitCount 发射次数
     */
    FlameEmitterParticle(const glm::vec3& pos, const glm::vec3& velocity, f64 lifetime, u32 emitCount);

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

private:
    static constexpr u32 EMIT_INTERVAL = 2; // 每 2 tick 发射一次
};

/**
 * @brief 烟雾发射器粒子
 *
 * 持续发射烟雾粒子的发射器。
 */
class SmokeEmitterParticle : public EmitterParticle {
public:
    SmokeEmitterParticle(const glm::vec3& pos, const glm::vec3& velocity, f64 lifetime, u32 emitCount);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

private:
    static constexpr u32 EMIT_INTERVAL = 3;
};

} // namespace mc::client::renderer::trident::particle::particles
