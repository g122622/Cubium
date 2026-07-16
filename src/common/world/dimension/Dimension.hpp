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

#include "../../core/Constants.hpp"
#include "../../core/Types.hpp"
#include "../../util/math/Vector3.hpp"
#include "../biome/BiomeSource.hpp"
#include "../gen/chunk/IChunkGenerator.hpp"
#include "DimensionType.hpp"
#include <memory>

namespace mc {

// 前向声明
class WorldLightManager;

/**
 * @brief 维度实例
 *
 * 将维度类型与区块生成器、生物群系提供者组合，
 * 表示一个完整的维度实例。
 *
 * @note 维度实例是不可变的，应在初始化时创建。
 */
class Dimension {
public:
    /**
     * @brief 构造维度实例
     *
     * @param id 维度ID
     * @param type 维度类型
     * @param generator 区块生成器；对于仅承载运行时世界句柄的维度包装器可以为空
     */
    Dimension(DimensionId id, DimensionType type, std::unique_ptr<IChunkGenerator> generator);

    virtual ~Dimension() = default;

    // 禁止拷贝
    Dimension(const Dimension&) = delete;
    Dimension& operator=(const Dimension&) = delete;

    // 允许移动
    Dimension(Dimension&&) noexcept = default;
    Dimension& operator=(Dimension&&) noexcept = default;

    // ========== 标识 ==========

    /**
     * @brief 获取维度ID
     */
    [[nodiscard]] DimensionId id() const { return m_id; }

    /**
     * @brief 获取维度类型
     */
    [[nodiscard]] const DimensionType& type() const { return m_type; }

    // ========== 生成器访问 ==========

    /**
     * @brief 获取区块生成器
     */
    [[nodiscard]] IChunkGenerator* generator() { return m_generator.get(); }
    [[nodiscard]] const IChunkGenerator* generator() const { return m_generator.get(); }

    // ========== 生物群系 ==========

    /**
     * @brief 获取生物群系源（MC 1.18+）
     */
    [[nodiscard]] world::biome::IBiomeSource* biomeSource()
    {
        return m_generator != nullptr ? m_generator->getBiomeSource() : nullptr;
    }
    [[nodiscard]] const world::biome::IBiomeSource* biomeSource() const
    {
        return m_generator != nullptr ? m_generator->getBiomeSource() : nullptr;
    }

    // ========== 出生点 ==========

    /**
     * @brief 获取出生点位置
     */
    [[nodiscard]] Vector3d spawnPoint() const { return m_spawnPoint; }

    /**
     * @brief 设置出生点位置
     */
    void setSpawnPoint(const Vector3d& pos) { m_spawnPoint = pos; }

    // ========== 维度能力 ==========

    /**
     * @brief 是否有天空光照
     */
    [[nodiscard]] bool hasSkyLight() const { return m_type.hasSkyLight(); }

    /**
     * @brief 是否有天花板
     */
    [[nodiscard]] bool hasCeiling() const { return m_type.hasCeiling(); }

    /**
     * @brief 最低建筑高度
     */
    [[nodiscard]] i32 minHeight() const { return m_type.minHeight(); }

    /**
     * @brief 最高建筑高度
     */
    [[nodiscard]] i32 maxHeight() const { return m_type.maxHeight(); }

    /**
     * @brief 逻辑高度上限（传送门放置等）
     */
    [[nodiscard]] i32 logicalHeight() const { return m_type.logicalHeight(); }

    // ========== 坐标转换 ==========

    /**
     * @brief 从主世界坐标转换到当前维度
     */
    [[nodiscard]] Vector3d fromOverworld(const Vector3d& pos) const { return m_type.scaleFromOverworld(pos); }

    /**
     * @brief 从当前维度转换到主世界坐标
     */
    [[nodiscard]] Vector3d toOverworld(const Vector3d& pos) const { return m_type.scaleToOverworld(pos); }

    // ========== 维度特性 ==========

    /**
     * @brief 是否超热（水会蒸发）
     */
    [[nodiscard]] bool ultraWarm() const { return m_type.ultraWarm(); }

    /**
     * @brief 床是否可用
     */
    [[nodiscard]] bool bedWorks() const { return m_type.bedWorks(); }

    /**
     * @brief 重生锚是否可用
     */
    [[nodiscard]] bool respawnAnchorWorks() const { return m_type.respawnAnchorWorks(); }

    /**
     * @brief 是否自然维度
     */
    [[nodiscard]] bool natural() const { return m_type.natural(); }

    /**
     * @brief 是否有末影龙战斗
     */
    [[nodiscard]] bool hasEnderDragonFight() const { return m_type.hasEnderDragonFight(); }

    /**
     * @brief 获取环境光照强度
     */
    [[nodiscard]] f32 ambientLight() const { return m_type.ambientLight(); }

    /**
     * @brief 获取固定时间值（如果有）
     */
    [[nodiscard]] std::optional<i64> fixedTime() const { return m_type.fixedTimeValue(); }

    // ========== 更新 ==========

    /**
     * @brief 维度刻更新
     *
     * 子类可覆盖以实现维度特定逻辑（如天气）。
     */
    virtual void tick();

protected:
    DimensionId m_id;
    DimensionType m_type;
    std::unique_ptr<IChunkGenerator> m_generator;
    Vector3d m_spawnPoint{0.0, static_cast<f64>(world::SEA_LEVEL) + 1.0, 0.0};
};

} // namespace mc
