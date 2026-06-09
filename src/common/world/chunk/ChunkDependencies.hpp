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
 */

#pragma once

#include "../../core/Types.hpp"
#include <stdexcept>
#include <string>
#include <vector>

namespace mc {

// 前向声明
class ChunkStatus;

// ============================================================================
// 区块依赖关系（MC 1.21 ChunkDependencies）
// ============================================================================

/**
 * @brief 区块生成阶段的依赖关系
 *
 * 对应 MC 1.21 的 ChunkDependencies 类。
 * 将依赖关系按半径索引存储：dependencyByRadius[radius] = 所需的 ChunkStatus。
 * 同时提供反向查找：给定 ChunkStatus，返回所需的最小半径。
 *
 * 示例（NOISE 阶段的 directDependencies）：
 *   dependencyByRadius = [BIOMES, STRUCTURE_STARTS, STRUCTURE_STARTS, ...]
 *   含义：半径 0 需要 BIOMES，半径 1-8 需要 STRUCTURE_STARTS
 */
class ChunkDependencies {
public:
    ChunkDependencies() = default;

    /**
     * @brief 从按半径索引的状态列表构造依赖关系
     * @param dependencyByRadius 按半径索引的依赖状态列表
     *
     * 列表索引为半径，值为该半径处所需的最低 ChunkStatus。
     * 例如 [BIOMES, STRUCTURE_STARTS, STRUCTURE_STARTS, ...] 表示：
     * - 半径 0（中心区块自身）需要 BIOMES
     * - 半径 1-8 需要 STRUCTURE_STARTS
     */
    explicit ChunkDependencies(std::vector<const ChunkStatus*> dependencyByRadius);

    /** @brief 依赖列表是否为空 */
    [[nodiscard]] bool empty() const { return m_dependencyByRadius.empty(); }

    /** @brief 获取依赖半径数量（= 最大半径 + 1） */
    [[nodiscard]] i32 size() const { return static_cast<i32>(m_dependencyByRadius.size()); }

    /** @brief 获取最大依赖半径（= size - 1） */
    [[nodiscard]] i32 getRadius() const { return std::max(0, static_cast<i32>(m_dependencyByRadius.size()) - 1); }

    /**
     * @brief 获取指定半径处的依赖状态
     * @param radius 半径（0 = 中心区块自身）
     * @return 该半径处所需的 ChunkStatus，如果 radius 超出范围则返回 nullptr
     */
    [[nodiscard]] const ChunkStatus* get(i32 radius) const;

    /**
     * @brief 获取指定状态所需的最低半径
     * @param status 要查找的 ChunkStatus
     * @return 该状态所需的最低半径
     * @throws std::invalid_argument 如果状态不在依赖范围内
     */
    [[nodiscard]] i32 getRadiusOf(const ChunkStatus& status) const;

    /**
     * @brief 获取底层依赖列表（用于调试/测试）
     */
    [[nodiscard]] const std::vector<const ChunkStatus*>& asList() const { return m_dependencyByRadius; }

private:
    /** 按半径索引的依赖状态列表 */
    std::vector<const ChunkStatus*> m_dependencyByRadius;

    /** 反向查找：statusIndex → 最小半径 */
    std::vector<i32> m_radiusByDependency;
};

} // namespace mc
