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

#include "common/core/Types.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>

namespace mc {

/**
 * @brief 竖直列扫描工具（MC net.minecraft.world.level.levelgen.Column）
 *
 * 描述一根竖直柱内的“空腔”范围：天花板（ceiling，向下指的第一块实体方块 Y）
 * 与地板（floor，向上指的第一块实体方块 Y）。两者都可能缺失（开放向上/向下）。
 *
 * 三种具体形态：
 *  - Range：floor 与 ceiling 均存在，height = ceiling - floor - 1
 *  - Ray：仅一端存在，向另一端开放（pointingUp=true 表示仅有 floor，向上开放）
 *  - Line：两端均开放（整列通透）
 *
 * scan 从 origin 起向上/向下各走 range 步：origin 必须满足 isEmpty 谓词才继续，
 * 否则返回 nullopt；向上首个满足 isStop 谓词的格记为 ceiling，向下首个记为 floor。
 *
 * 项目无 LevelSimulatedReader，直接用 IWorld& + 两个状态谓词替代。
 */
class Column {
public:
    using StatePredicate = std::function<bool(const BlockState*)>;

    class Range;
    class Ray;
    class Line;

    virtual ~Column() = default;

    [[nodiscard]] virtual std::optional<i32> getCeiling() const = 0;
    [[nodiscard]] virtual std::optional<i32> getFloor() const = 0;
    [[nodiscard]] virtual std::optional<i32> getHeight() const = 0;

    [[nodiscard]] std::unique_ptr<Column> withFloor(std::optional<i32> floor) const;
    [[nodiscard]] std::unique_ptr<Column> withCeiling(std::optional<i32> ceiling) const;

    /**
     * @brief 从 origin 起竖直扫描列
     *
     * @param world 世界
     * @param origin 起点（必须满足 isEmptyPredicate 才会扫描，否则返回 nullopt）
     * @param range 单方向最大扫描步数
     * @param isEmptyPredicate 空腔判定（air/water 等）
     * @param isStopPredicate 边界判定（滴石基座/岩浆/实体方块等）
     */
    [[nodiscard]] static std::optional<std::unique_ptr<Column>> scan(IWorld& world,
        const BlockPos& origin,
        i32 range,
        StatePredicate isEmptyPredicate,
        StatePredicate isStopPredicate);

    static std::unique_ptr<Column> create(std::optional<i32> floor, std::optional<i32> ceiling);

private:
    static std::optional<i32> scanDirection(IWorld& world,
        i32 range,
        const StatePredicate& isEmptyPredicate,
        const StatePredicate& isStopPredicate,
        BlockPosMutable& cursor,
        i32 originY,
        Direction direction);
};

/**
 * @brief 两端闭合的列范围
 */
class Column::Range final : public Column {
public:
    Range(i32 floor, i32 ceiling)
        : m_floor(floor)
        , m_ceiling(ceiling)
    {
        if (height() < 0) {
            throw std::invalid_argument("Column of negative height");
        }
    }

    [[nodiscard]] std::optional<i32> getCeiling() const override { return m_ceiling; }
    [[nodiscard]] std::optional<i32> getFloor() const override { return m_floor; }
    [[nodiscard]] std::optional<i32> getHeight() const override { return height(); }

    [[nodiscard]] i32 ceiling() const noexcept { return m_ceiling; }
    [[nodiscard]] i32 floor() const noexcept { return m_floor; }
    [[nodiscard]] i32 height() const noexcept { return m_ceiling - m_floor - 1; }

private:
    i32 m_floor;
    i32 m_ceiling;
};

/**
 * @brief 单端闭合的列射线
 *
 * pointingUp=true 表示仅有 floor（向上开放）；false 表示仅有 ceiling（向下开放）。
 */
class Column::Ray final : public Column {
public:
    Ray(i32 edge, bool pointingUp)
        : m_edge(edge)
        , m_pointingUp(pointingUp)
    {}

    [[nodiscard]] std::optional<i32> getCeiling() const override
    {
        return m_pointingUp ? std::nullopt : std::optional<i32>(m_edge);
    }
    [[nodiscard]] std::optional<i32> getFloor() const override
    {
        return m_pointingUp ? std::optional<i32>(m_edge) : std::nullopt;
    }
    [[nodiscard]] std::optional<i32> getHeight() const override { return std::nullopt; }

private:
    i32 m_edge;
    bool m_pointingUp;
};

/**
 * @brief 两端开放的列（整列通透）
 */
class Column::Line final : public Column {
public:
    [[nodiscard]] std::optional<i32> getCeiling() const override { return std::nullopt; }
    [[nodiscard]] std::optional<i32> getFloor() const override { return std::nullopt; }
    [[nodiscard]] std::optional<i32> getHeight() const override { return std::nullopt; }
};

} // namespace mc
