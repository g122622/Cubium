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

#include "common/advancement/MinMaxBounds.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// 前向声明
namespace mc {
class IWorld;
class BlockPos;
class BlockState;
namespace world::biome {
class Biome;
} // namespace world::biome
using world::biome::Biome;
} // namespace mc

namespace mc::advancement {

/**
 * @brief 位置谓词
 *
 * 用于匹配位置的条件谓词，检查坐标、生物群系、维度等。
 */
class LocationPredicate {
public:
    /**
     * @brief 默认构造（匹配任意位置）
     */
    LocationPredicate() = default;

    /**
     * @brief 检查位置是否匹配
     * @param world 世界
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const IWorld& world, f64 x, f64 y, f64 z) const;

    /**
     * @brief 检查方块位置是否匹配
     */
    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 检查是否匹配任意位置
     */
    [[nodiscard]] bool isAny() const noexcept { return m_isAny; }

    /**
     * @brief 从JSON解析
     */
    static Result<LocationPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    // ========== Getters ==========

    [[nodiscard]] const std::optional<ResourceLocation>& getBiome() const noexcept { return m_biome; }
    [[nodiscard]] const std::optional<ResourceLocation>& getDimension() const noexcept { return m_dimension; }
    [[nodiscard]] const DoubleBounds& getX() const noexcept { return m_x; }
    [[nodiscard]] const DoubleBounds& getY() const noexcept { return m_y; }
    [[nodiscard]] const DoubleBounds& getZ() const noexcept { return m_z; }

private:
    std::optional<ResourceLocation> m_biome;     ///< 生物群系（如 "minecraft:plains"）
    std::optional<ResourceLocation> m_dimension; ///< 维度（如 "minecraft:overworld"）
    DoubleBounds m_x;                            ///< X坐标范围
    DoubleBounds m_y;                            ///< Y坐标范围
    DoubleBounds m_z;                            ///< Z坐标范围
    bool m_isAny = true;
};

/**
 * @brief 距离谓词
 *
 * 用于匹配距离的条件谓词。
 */
class DistancePredicate {
public:
    /**
     * @brief 默认构造（匹配任意距离）
     */
    DistancePredicate() = default;

    /**
     * @brief 构造距离谓词
     */
    explicit DistancePredicate(DoubleBounds range)
        : m_range(std::move(range))
        , m_isAny(m_range.isUnbounded())
    {}

    /**
     * @brief 检查距离是否匹配
     * @param x1 起点X
     * @param y1 起点Y
     * @param z1 起点Z
     * @param x2 终点X
     * @param y2 终点Y
     * @param z2 终点Z
     * @return 是否匹配
     */
    [[nodiscard]] bool test(f64 x1, f64 y1, f64 z1, f64 x2, f64 y2, f64 z2) const;

    /**
     * @brief 检查距离是否匹配（已计算平方距离）
     * @param distanceSq 平方距离
     * @return 是否匹配
     */
    [[nodiscard]] bool testSquared(f64 distanceSq) const;

    /**
     * @brief 检查是否匹配任意距离
     */
    [[nodiscard]] bool isAny() const noexcept { return m_isAny; }

    /**
     * @brief 从JSON解析
     */
    static Result<DistancePredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    // ========== 静态工厂方法 ==========

    static DistancePredicate exactly(f64 distance) { return DistancePredicate(DoubleBounds::exactly(distance)); }

    static DistancePredicate atLeast(f64 min) { return DistancePredicate(DoubleBounds::atLeast(min)); }

    static DistancePredicate atMost(f64 max) { return DistancePredicate(DoubleBounds::atMost(max)); }

    static DistancePredicate between(f64 min, f64 max) { return DistancePredicate(DoubleBounds::between(min, max)); }

private:
    DoubleBounds m_range;
    bool m_isAny = true;
};

} // namespace mc::advancement
