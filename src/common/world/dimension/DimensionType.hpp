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

#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include <optional>
#include <string>
#include <utility>

namespace mc {

/**
 * @brief 维度类型定义
 *
 * 定义维度的固有属性，如坐标缩放、环境特性、高度限制等。
 *
 * 使用示例:
 * @code
 * auto nether = DimensionType::nether();
 * if (nether.ultraWarm()) {
 *     // 水会蒸发
 * }
 * Vector3d scaled = nether.scaleFromOverworld(pos);
 * @endcode
 *
 * @note 维度类型是不可变的，应在初始化时创建。
 */
class DimensionType {
public:
    /**
     * @brief 获取主世界维度类型
     */
    static DimensionType overworld();

    /**
     * @brief 获取下界维度类型
     */
    static DimensionType nether();

    /**
     * @brief 获取末地维度类型
     */
    static DimensionType theEnd();

    /**
     * @brief 根据维度ID获取维度类型
     *
     * @param id 维度ID (0=主世界, -1=下界, 1=末地)
     * @return 对应的维度类型
     */
    [[nodiscard]] static DimensionType fromId(DimensionId id);

    // ========== 标识 ==========

    /**
     * @brief 获取维度ID
     */
    [[nodiscard]] DimensionId id() const { return m_id; }

    /**
     * @brief 获取维度名称
     */
    [[nodiscard]] const std::string& name() const { return m_name; }

    // ========== 环境属性 ==========

    /**
     * @brief 是否有天花板（下界有基岩天花板）
     */
    [[nodiscard]] bool hasCeiling() const { return m_hasCeiling; }

    /**
     * @brief 是否有天空光照
     */
    [[nodiscard]] bool hasSkyLight() const { return m_hasSkyLight; }

    /**
     * @brief 是否超热（下界为 true，水会蒸发）
     */
    [[nodiscard]] bool ultraWarm() const { return m_ultraWarm; }

    /**
     * @brief 是否自然维度（影响床和重生锚行为）
     *
     * 自然维度中床可以设置重生点，非自然维度中床会爆炸。
     */
    [[nodiscard]] bool natural() const { return m_natural; }

    /**
     * @brief 是否有末影龙战斗
     */
    [[nodiscard]] bool hasEnderDragonFight() const { return m_hasEnderDragonFight; }

    // ========== 功能属性 ==========

    /**
     * @brief 床是否可用
     *
     * 主世界为 true，下界和末地为 false（床会爆炸）。
     */
    [[nodiscard]] bool bedWorks() const { return m_bedWorks; }

    /**
     * @brief 重生锚是否可用
     *
     * 下界为 true，其他维度为 false。
     */
    [[nodiscard]] bool respawnAnchorWorks() const { return m_respawnAnchorWorks; }

    /**
     * @brief 是否会发生袭击事件
     */
    [[nodiscard]] bool hasRaids() const { return m_hasRaids; }

    /**
     * @brief 猪灵是否安全（不会僵尸化）
     */
    [[nodiscard]] bool piglinSafe() const { return m_piglinSafe; }

    // ========== 高度限制 ==========

    /**
     * @brief 获取最低建筑高度
     */
    [[nodiscard]] i32 minHeight() const { return m_minHeight; }

    /**
     * @brief 获取最高建筑高度
     */
    [[nodiscard]] i32 maxHeight() const { return m_maxHeight; }

    /**
     * @brief 获取逻辑高度上限
     *
     * 用于传送门放置等逻辑，下界为 128。
     */
    [[nodiscard]] i32 logicalHeight() const { return m_logicalHeight; }

    // ========== 坐标转换 ==========

    /**
     * @brief 获取坐标缩放比例
     *
     * 主世界和末地为 1.0，下界为 8.0。
     * 用于计算维度间传送的坐标转换。
     *
     * @return 缩放比例（相对于主世界）
     */
    [[nodiscard]] f32 coordinateScale() const { return m_coordinateScale; }

    /**
     * @brief 从主世界坐标转换到当前维度坐标
     *
     * @param pos 主世界坐标
     * @return 当前维度坐标
     */
    [[nodiscard]] Vector3d scaleFromOverworld(const Vector3d& pos) const;

    /**
     * @brief 从当前维度坐标转换到主世界坐标
     *
     * @param pos 当前维度坐标
     * @return 主世界坐标
     */
    [[nodiscard]] Vector3d scaleToOverworld(const Vector3d& pos) const;

    /**
     * @brief 计算两个维度之间的坐标转换
     *
     * @param pos 源坐标
     * @param from 源维度类型
     * @param to 目标维度类型
     * @return 转换后的坐标
     */
    [[nodiscard]] static Vector3d transformPosition(
        const Vector3d& pos, const DimensionType& from, const DimensionType& to);

    // ========== 光照和时间 ==========

    /**
     * @brief 获取环境光照强度
     */
    [[nodiscard]] f32 ambientLight() const { return m_ambientLight; }

    /**
     * @brief 是否有固定时间
     */
    [[nodiscard]] bool hasFixedTime() const { return m_fixedTime.has_value(); }

    /**
     * @brief 获取固定时间值
     *
     * 下界固定为 18000（午夜），末地固定为 6000（正午）。
     * @return 固定时间值（tick），如果没有固定时间则返回空
     */
    [[nodiscard]] std::optional<i64> fixedTimeValue() const { return m_fixedTime; }

    // ========== 方块标签 ==========

    /**
     * @brief 获取无限燃烧方块标签
     *
     * 主世界: minecraft:infiniburn_overworld
     * 下界: minecraft:infiniburn_nether
     * 末地: minecraft:infiniburn_end
     */
    [[nodiscard]] const std::string& infiniburn() const { return m_infiniburn; }

    // ========== 比较 ==========

    bool operator==(const DimensionType& other) const { return m_id == other.m_id; }
    bool operator!=(const DimensionType& other) const { return m_id != other.m_id; }

    // 移动操作（noexcept 提升性能）
    DimensionType(DimensionType&& other) noexcept = default;
    DimensionType& operator=(DimensionType&& other) noexcept = default;

    // 拷贝操作
    DimensionType(const DimensionType& other) = default;
    DimensionType& operator=(const DimensionType& other) = default;

    /**
     * @brief 检查是否为主世界
     */
    [[nodiscard]] bool isOverworld() const { return m_id == 0; }

    /**
     * @brief 检查是否为下界
     */
    [[nodiscard]] bool isNether() const { return m_id == -1; }

    /**
     * @brief 检查是否为末地
     */
    [[nodiscard]] bool isTheEnd() const { return m_id == 1; }

private:
    DimensionType(DimensionId id, std::string name)
        : m_id(id)
        , m_name(std::move(name))
    {}

    // 标识
    DimensionId m_id = 0;
    std::string m_name;

    // 环境属性
    bool m_hasCeiling = false;
    bool m_hasSkyLight = true;
    bool m_ultraWarm = false;
    bool m_natural = true;
    bool m_hasEnderDragonFight = false;

    // 功能属性
    bool m_bedWorks = true;
    bool m_respawnAnchorWorks = false;
    bool m_hasRaids = true;
    bool m_piglinSafe = false;

    // 高度
    i32 m_minHeight = world::MIN_BUILD_HEIGHT;
    i32 m_maxHeight = world::MAX_BUILD_HEIGHT;
    i32 m_logicalHeight = world::MAX_BUILD_HEIGHT;

    // 坐标转换
    f32 m_coordinateScale = 1.0f;

    // 光照和时间
    f32 m_ambientLight = 0.0f;
    std::optional<i64> m_fixedTime;

    // 方块标签
    std::string m_infiniburn;
};

} // namespace mc
