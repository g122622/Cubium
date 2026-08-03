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

#include "ParticleRenderType.hpp"
#include "ParticleTypes.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/random/Random.hpp"
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp>

namespace mc {
class IWorld;
class PhysicsEngine;
} // namespace mc

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle {

// 前置声明
class ParticleTextureAtlas;
struct SpriteInfo;

/**
 * @brief 粒子发射回调类型
 *
 * 用于发射器粒子发射新粒子时调用。
 */
using ParticleEmitCallback = std::function<void(ParticleTypeId type, const glm::vec3& pos, const glm::vec3& velocity)>;

/**
 * @brief 粒子顶点数据
 *
 * 用于传递给 GPU 的顶点格式。
 */
struct ParticleVertex {
    glm::vec3 position; ///< 粒子位置
    glm::vec2 texCoord; ///< 纹理坐标
    glm::vec4 color;    ///< RGBA 颜色（含透明度）
    f32 size;           ///< 粒子大小
    f32 alpha;          ///< 额外的 alpha 值（用于淡出）
};

/**
 * @brief 粒子碰撞上下文
 *
 * 用于缓存碰撞检测结果，避免重复计算。
 */
struct ParticleCollisionContext {
    bool collidedX = false; ///< X 轴是否发生碰撞
    bool collidedY = false; ///< Y 轴是否发生碰撞
    bool collidedZ = false; ///< Z 轴是否发生碰撞
    bool onGround = false;  ///< 是否在地面

    void reset()
    {
        collidedX = false;
        collidedY = false;
        collidedZ = false;
        onGround = false;
    }
};

/**
 * @brief 粒子基类
 *
 * 所有粒子的基类，定义粒子的基本属性和行为。
 *
 * 生命周期：
 * 1. 构造：设置初始位置、速度、颜色等属性
 * 2. tick()：每帧更新位置、速度、年龄等
 * 3. buildVertices()：生成渲染顶点
 * 4. isAlive() == false 时销毁
 *
 * 用法示例：
 * @code
 * class MyParticle : public Particle {
 * public:
 *     MyParticle(const glm::vec3& pos, const glm::vec3& velocity)
 *         : Particle(pos, velocity) {
 *         setGravity(0.02f);
 *         setMaxAge(60.0f);
 *         setColor(glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
 *     }
 *
 *     void tick(ClientWorld* world) override {
 *         Particle::tick(world);
 *         // 自定义行为
 *     }
 *
 *     ParticleRenderType getRenderType() const override {
 *         return ParticleRenderType::PARTICLE_SHEET_LIT;
 *     }
 *
 *     ResourceLocation getTextureLocation() const override {
 *         return ResourceLocation("minecraft:particle/flame");
 *     }
 * };
 * @endcode
 */
class Particle {
public:
    /**
     * @brief 构造粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     */
    Particle(const glm::vec3& pos, const glm::vec3& velocity);

    virtual ~Particle() = default;

    // 禁止拷贝
    Particle(const Particle&) = delete;
    Particle& operator=(const Particle&) = delete;

    // 允许移动
    Particle(Particle&&) noexcept = default;
    Particle& operator=(Particle&&) noexcept = default;

    // ========================================================================
    // 生命周期
    // ========================================================================

    /**
     * @brief 更新粒子状态
     *
     * 每游戏 tick 调用。更新位置、速度、年龄等属性。
     * 子类应调用父类的 tick() 方法以保持基本行为。
     *
     * @param world 客户端世界（可选，用于碰撞检测和光照采样）
     */
    virtual void tick(mc::client::ClientWorld* world = nullptr);

    /**
     * @brief 粒子是否存活
     *
     * @return 是否存活
     */
    [[nodiscard]] bool isAlive() const { return !m_expired; }

    /**
     * @brief 标记粒子为过期
     */
    void setExpired() { m_expired = true; }

    // ========================================================================
    // 渲染
    // ========================================================================

    /**
     * @brief 获取渲染类型
     *
     * 决定粒子的渲染方式（混合模式、纹理来源、光照处理）。
     * 子类应重写此方法以指定渲染类型。
     *
     * @return 渲染类型
     */
    [[nodiscard]] virtual ParticleRenderType getRenderType() const
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    /**
     * @brief 生成渲染顶点数据
     *
     * 生成 billboard quad 的四个顶点。
     * 子类可以重写此方法以实现自定义渲染。
     *
     * @param cameraPos 相机位置（用于 billboard 计算）
     * @param partialTick 部分 tick（用于插值）
     * @param atlas 纹理图集（用于获取 UV 坐标）
     * @param outVertices 输出顶点数组（4 个顶点组成一个 quad）
     */
    virtual void buildVertices(const glm::vec3& cameraPos,
        f64 partialTick,
        const ParticleTextureAtlas& atlas,
        std::vector<ParticleVertex>& outVertices) const;

    /**
     * @brief 获取纹理资源位置
     *
     * 用于从纹理图集中获取 UV 坐标。
     * 子类应重写此方法以使用不同纹理。
     *
     * @return 纹理资源位置（如 "minecraft:particle/flame"）
     */
    [[nodiscard]] virtual ResourceLocation getTextureLocation() const;

    /**
     * @brief 获取光照值
     *
     * 从世界采样粒子位置的光照。
     * 发光粒子可以返回固定的高亮度值。
     *
     * @param world 客户端世界
     * @return 组合光照值（skyLight << 4 | blockLight），范围 0-255
     */
    [[nodiscard]] virtual u32 getLightColor(mc::client::ClientWorld* world) const;

    /**
     * @brief 获取粒子缩放比例
     *
     * 可用于实现粒子随年龄变化大小。
     *
     * @param partialTick 部分 tick
     * @return 缩放比例
     */
    [[nodiscard]] virtual f64 getScale(f64 partialTick) const;

    // ========================================================================
    // 物理属性
    // ========================================================================

    /**
     * @brief 移动并碰撞检测
     *
     * 如果 world 为 nullptr，则只移动不检测碰撞。
     *
     * @param world 客户端世界
     * @param delta 移动增量
     */
    void move(mc::client::ClientWorld* world, const glm::vec3& delta);

    /**
     * @brief 设置碰撞盒尺寸
     *
     * @param width 宽度（X/Z 轴）
     * @param height 高度（Y 轴）
     */
    void setBoundingBox(f64 width, f64 height);

    /**
     * @brief 获取碰撞盒
     *
     * @return 当前碰撞盒
     */
    [[nodiscard]] AxisAlignedBB getBoundingBox() const;

    // ========================================================================
    // 属性访问器
    // ========================================================================

    [[nodiscard]] const glm::vec3& position() const { return m_position; }
    [[nodiscard]] const glm::vec3& prevPosition() const { return m_prevPosition; }
    [[nodiscard]] const glm::vec3& velocity() const { return m_velocity; }
    [[nodiscard]] f64 age() const { return m_age; }
    [[nodiscard]] f64 maxAge() const { return m_maxAge; }
    [[nodiscard]] f64 gravity() const { return m_gravity; }
    [[nodiscard]] f64 friction() const { return m_friction; }
    [[nodiscard]] f64 size() const { return m_size; }
    [[nodiscard]] const glm::vec4& color() const { return m_color; }
    [[nodiscard]] bool onGround() const { return m_collisionContext.onGround; }
    [[nodiscard]] bool hasPhysics() const { return m_hasPhysics; }
    [[nodiscard]] f64 roll() const { return m_roll; }
    [[nodiscard]] bool collidedX() const { return m_collisionContext.collidedX; }
    [[nodiscard]] bool collidedY() const { return m_collisionContext.collidedY; }
    [[nodiscard]] bool collidedZ() const { return m_collisionContext.collidedZ; }

    void setPosition(const glm::vec3& pos);
    void setVelocity(const glm::vec3& vel) { m_velocity = vel; }
    void setGravity(f64 g) { m_gravity = g; }
    void setFriction(f64 f) { m_friction = f; }
    void setSize(f64 s) { m_size = s; }
    void setColor(const glm::vec4& c) { m_color = c; }
    void setMaxAge(f64 age) { m_maxAge = age; }
    void setHasPhysics(bool physics) { m_hasPhysics = physics; }
    void setRoll(f64 roll) { m_roll = roll; }

    /**
     * @brief 设置发射回调
     *
     * 用于发射器粒子发射新粒子时调用。
     * ParticleManager 会在 tick 前设置此回调。
     *
     * @param callback 发射回调函数
     */
    void setEmitCallback(ParticleEmitCallback callback) { m_emitCallback = std::move(callback); }

    /**
     * @brief 获取发射回调
     */
    [[nodiscard]] const ParticleEmitCallback& emitCallback() const { return m_emitCallback; }

protected:
    // ========================================================================
    // 位置和运动
    // ========================================================================

    glm::vec3 m_position;     ///< 当前位置
    glm::vec3 m_prevPosition; ///< 上一帧位置（用于插值）
    glm::vec3 m_velocity;     ///< 速度

    // ========================================================================
    // 外观
    // ========================================================================

    glm::vec4 m_color = glm::vec4(1.0f); ///< RGBA 颜色
    f64 m_size = 0.1;                    ///< 粒子大小
    f64 m_roll = 0.0;                    ///< 旋转角度（弧度）
    f64 m_prevRoll = 0.0;                ///< 上一帧旋转角度

    // ========================================================================
    // 生命周期
    // ========================================================================

    f64 m_age = 0.0;        ///< 已存活时间（ticks）
    f64 m_maxAge = 1.0;     ///< 最大存活时间（ticks）
    bool m_expired = false; ///< 是否已过期

    // ========================================================================
    // 物理
    // ========================================================================

    f64 m_gravity = 0.0;      ///< 重力加速度（方块/tick²）
    f64 m_friction = 0.98;    ///< 空气阻力系数
    bool m_hasPhysics = true; ///< 是否进行碰撞检测

    // ========================================================================
    // 碰撞盒和碰撞状态
    // ========================================================================

    f64 m_bboxWidth = 0.0;                       ///< 碰撞盒宽度
    f64 m_bboxHeight = 0.0;                      ///< 碰撞盒高度
    ParticleCollisionContext m_collisionContext; ///< 碰撞上下文

    // ========================================================================
    // 随机源
    // ========================================================================

    mc::math::Random m_random; ///< 粒子实例级随机源，供子类在 tick 和构造中使用

    // ========================================================================
    // 发射回调
    // ========================================================================

    ParticleEmitCallback m_emitCallback; ///< 发射回调（用于发射器粒子）
};

/**
 * @brief 粒子工厂函数类型
 *
 * 用于创建粒子实例的工厂函数。
 *
 * @param pos 初始位置
 * @param velocity 初始速度
 * @param world 客户端世界（可选）
 * @return 粒子实例
 */
using ParticleFactory = std::function<std::unique_ptr<Particle>(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)>;

// 前置声明
namespace data {
class ParticleData;
} // namespace data

/**
 * @brief 携带粒子数据的工厂函数类型
 *
 * 用于创建需要额外数据的粒子实例（如振动粒子需要目标位置、轨迹粒子需要颜色等）。
 * 当 ParticleData 不为 nullptr 时，工厂应从中提取额外参数；
 * 当 ParticleData 为 nullptr 时，工厂应使用默认/回退行为（与 ParticleFactory 一致）。
 *
 * @param pos 初始位置
 * @param velocity 初始速度
 * @param world 客户端世界（可选）
 * @param data 粒子数据（可为 nullptr，表示使用默认值）
 * @return 粒子实例
 */
using ParticleDataFactory = std::function<std::unique_ptr<Particle>(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world, const data::ParticleData* data)>;

} // namespace mc::client::renderer::trident::particle
