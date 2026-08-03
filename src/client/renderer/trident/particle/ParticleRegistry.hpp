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

#include "Particle.hpp"
#include "ParticleRenderType.hpp"
#include "ParticleTypes.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "data/ParticleData.hpp"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/ext/vector_float3.hpp>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle {

/**
 * @brief 粒子类型信息
 *
 * 存储粒子类型的元数据。
 */
struct ParticleTypeInfo {
    ParticleTypeId id;                    ///< 类型 ID
    std::string name;                     ///< 类型名称（如 "minecraft:flame"）
    ParticleFactory factory;              ///< 工厂函数
    ParticleRenderType defaultRenderType; ///< 默认渲染类型
    bool ignoreDistance;                  ///< 是否忽略距离限制
    bool overrideLimiter;                 ///< 是否忽略粒子质量限制（重要粒子始终显示）
    f64 defaultLifetime;                  ///< 默认生命周期（ticks）
    bool hasPhysics;                      ///< 是否有物理碰撞
    ParticleDataFactory dataFactory;      ///< 携带粒子数据的工厂函数（可选）
};

/**
 * @brief 粒子类型注册表
 *
 * 单例模式，管理所有粒子类型的注册和创建。
 *
 * 用法示例：
 * @code
 * // 注册粒子类型
 * ParticleRegistry::instance().registerType(
 *     ParticleTypeId::Flame,
 *     "minecraft:flame",
 *     FlameParticle::create,
 *     ParticleRenderType::PARTICLE_SHEET_LIT
 * );
 *
 * // 创建粒子实例
 * auto particle = ParticleRegistry::instance().createParticle(
 *     ParticleTypeId::Flame,
 *     glm::vec3(0.0f),
 *     glm::vec3(0.0f, 0.1f, 0.0f)
 * );
 * @endcode
 */
class ParticleRegistry {
public:
    /**
     * @brief 获取单例实例
     *
     * @return 注册表实例引用
     */
    static ParticleRegistry& instance();

    // 禁止拷贝和移动
    ParticleRegistry(const ParticleRegistry&) = delete;
    ParticleRegistry& operator=(const ParticleRegistry&) = delete;
    ParticleRegistry(ParticleRegistry&&) = delete;
    ParticleRegistry& operator=(ParticleRegistry&&) = delete;

    // ========================================================================
    // 注册
    // ========================================================================

    /**
     * @brief 注册粒子类型
     *
     * @param id 粒子类型 ID
     * @param name 类型名称（如 "minecraft:flame"）
     * @param factory 工厂函数
     * @param defaultRenderType 默认渲染类型
     * @param defaultLifetime 默认生命周期（ticks）
     * @param hasPhysics 是否有物理碰撞
     * @param ignoreDistance 是否忽略距离限制
     * @param overrideLimiter 是否忽略粒子质量限制（重要粒子始终显示）
     */
    void registerType(ParticleTypeId id,
        const std::string& name,
        ParticleFactory factory,
        ParticleRenderType defaultRenderType,
        f64 defaultLifetime,
        bool hasPhysics,
        bool ignoreDistance,
        bool overrideLimiter = false);

    /**
     * @brief 注册简单粒子类型（无参数粒子）
     *
     * 便捷方法，用于注册不需要特殊参数的粒子类型。
     *
     * @param id 粒子类型 ID
     * @param name 类型名称
     * @param factory 工厂函数（可以为 nullptr，用于仅注册元数据）
     * @param defaultRenderType 默认渲染类型
     * @param overrideLimiter 是否忽略粒子质量限制（重要粒子始终显示）
     */
    void registerSimpleType(ParticleTypeId id,
        const std::string& name,
        ParticleFactory factory,
        ParticleRenderType defaultRenderType,
        bool overrideLimiter = false);

    /**
     * @brief 注册携带粒子数据的工厂函数
     *
     * 为已注册的粒子类型设置数据工厂函数。
     * 数据工厂函数可以接收 ParticleData 参数，从中提取额外数据创建粒子。
     * 当 data 为 nullptr 时，应使用默认/回退行为。
     *
     * @param id 粒子类型 ID
     * @param dataFactory 携带粒子数据的工厂函数
     */
    void registerDataFactory(ParticleTypeId id, ParticleDataFactory dataFactory);

    // ========================================================================
    // 创建粒子
    // ========================================================================

    /**
     * @brief 创建粒子实例
     *
     * @param id 粒子类型 ID
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param world 客户端世界（可选）
     * @return 粒子实例，如果类型无效返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<Particle> createParticle(ParticleTypeId id,
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world = nullptr) const;

    /**
     * @brief 通过名称创建粒子实例
     *
     * @param name 粒子类型名称（如 "minecraft:flame"）
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param world 客户端世界（可选）
     * @return 粒子实例，如果名称无效返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<Particle> createParticle(const std::string& name,
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world = nullptr) const;

    /**
     * @brief 携带粒子数据创建粒子实例
     *
     * 如果该类型注册了数据工厂函数且 data 不为 nullptr，使用数据工厂创建粒子；
     * 否则回退到普通工厂函数（忽略 data 参数）。
     *
     * @param id 粒子类型 ID
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param world 客户端世界（可选）
     * @param data 粒子数据（可为 nullptr）
     * @return 粒子实例，如果类型无效返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<Particle> createParticleWithData(ParticleTypeId id,
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world,
        const data::ParticleData* data) const;

    // ========================================================================
    // 查询
    // ========================================================================

    /**
     * @brief 通过名称获取粒子类型 ID
     *
     * @param name 粒子类型名称
     * @return 粒子类型 ID，如果不存在返回 nullopt
     */
    [[nodiscard]] std::optional<ParticleTypeId> getTypeId(const std::string& name) const;

    /**
     * @brief 通过资源位置获取粒子类型 ID
     *
     * @param location 资源位置
     * @return 粒子类型 ID，如果不存在返回 nullopt
     */
    [[nodiscard]] std::optional<ParticleTypeId> getTypeId(const ResourceLocation& location) const;

    /**
     * @brief 获取粒子类型名称
     *
     * @param id 粒子类型 ID
     * @return 类型名称，如果无效返回 "minecraft:invalid"
     */
    [[nodiscard]] const std::string& getTypeName(ParticleTypeId id) const;

    /**
     * @brief 获取粒子类型信息
     *
     * @param id 粒子类型 ID
     * @return 类型信息指针，如果无效返回 nullptr
     */
    [[nodiscard]] const ParticleTypeInfo* getTypeInfo(ParticleTypeId id) const;

    /**
     * @brief 检查粒子类型是否已注册
     *
     * @param id 粒子类型 ID
     * @return 是否已注册
     */
    [[nodiscard]] bool isRegistered(ParticleTypeId id) const;

    /**
     * @brief 检查粒子名称是否已注册
     *
     * @param name 粒子类型名称
     * @return 是否已注册
     */
    [[nodiscard]] bool isRegistered(const std::string& name) const;

    /**
     * @brief 获取所有已注册的粒子类型 ID
     *
     * @return 粒子类型 ID 列表
     */
    [[nodiscard]] std::vector<ParticleTypeId> getAllTypeIds() const;

    /**
     * @brief 获取已注册粒子类型数量
     *
     * @return 类型数量
     */
    [[nodiscard]] Size typeCount() const { return m_types.size(); }

private:
    ParticleRegistry();
    ~ParticleRegistry() = default;

    /**
     * @brief 注册内置粒子类型
     */
    void _registerBuiltinTypes();

    // 类型存储
    std::unordered_map<ParticleTypeId, ParticleTypeInfo> m_types;
    std::unordered_map<std::string, ParticleTypeId> m_nameToId;

    // 默认名称（用于无效类型）
    std::string m_invalidTypeName = "minecraft:invalid";
};

} // namespace mc::client::renderer::trident::particle
