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

#include "PathNodeType.hpp"
#include "PathPoint.hpp"
#include "Region.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 节点处理器基类
 *
 * 负责生成和缓存路径节点，计算相邻节点。
 * 不同移动类型（行走、飞行、游泳）有不同的实现。
 */
class NodeProcessor {
public:
    NodeProcessor() = default;
    virtual ~NodeProcessor() = default;

    // ========== 移动语义 ==========

    NodeProcessor(NodeProcessor&& other) noexcept
        : m_region(other.m_region)
        , m_entityWidth(other.m_entityWidth)
        , m_entityHeight(other.m_entityHeight)
        , m_nodeCache(std::move(other.m_nodeCache))
        , m_openNodes(std::move(other.m_openNodes))
    {
        other.m_region = nullptr;
    }

    NodeProcessor& operator=(NodeProcessor&& other) noexcept
    {
        if (this != &other) {
            m_region = other.m_region;
            m_entityWidth = other.m_entityWidth;
            m_entityHeight = other.m_entityHeight;
            m_nodeCache = std::move(other.m_nodeCache);
            m_openNodes = std::move(other.m_openNodes);
            other.m_region = nullptr;
        }
        return *this;
    }

    // ========== 配置 ==========

    /**
     * @brief 设置世界区域
     */
    void setRegion(const Region* region) { m_region = region; }

    /**
     * @brief 设置实体尺寸
     * @param width 实体宽度
     * @param height 实体高度
     */
    void setEntitySize(f32 width, f32 height)
    {
        m_entityWidth = width;
        m_entityHeight = height;
    }

    // ========== 节点访问 ==========

    /**
     * @brief 获取或创建指定位置的路径点
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 路径点，如果位置无效返回nullptr
     */
    [[nodiscard]] PathPoint* getNode(i32 x, i32 y, i32 z)
    {
        // 检查缓存
        u64 hash = makeHash(x, y, z);
        auto it = m_nodeCache.find(hash);
        if (it != m_nodeCache.end()) {
            return it->second.get();
        }

        // 创建新节点
        auto node = createNode(x, y, z);
        if (!node) {
            return nullptr;
        }

        PathPoint* result = node.get();
        m_nodeCache[hash] = std::move(node);
        return result;
    }

    /**
     * @brief 获取指定位置的节点类型
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 节点类型
     */
    [[nodiscard]] virtual PathNodeType getNodeType(i32 x, i32 y, i32 z) = 0;

    /**
     * @brief 获取指定位置的节点类型（考虑实体尺寸）
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 节点类型
     */
    [[nodiscard]] virtual PathNodeType getNodeTypeWithEntity(i32 x, i32 y, i32 z) = 0;

    // ========== 虚方法 ==========

    /**
     * @brief 清除缓存
     */
    virtual void clear()
    {
        m_nodeCache.clear();
        m_openNodes.clear();
    }

    /**
     * @brief 获取起始节点
     * @param x 实体X坐标
     * @param y 实体Y坐标
     * @param z 实体Z坐标
     * @return 起始节点
     */
    [[nodiscard]] virtual PathPoint* getStartNode(i32 x, i32 y, i32 z) = 0;

    /**
     * @brief 获取相邻节点
     * @param current 当前节点
     * @return 相邻节点数组（指针所有权属于NodeProcessor）
     */
    [[nodiscard]] virtual std::vector<PathPoint*> getNeighbors(PathPoint* current) = 0;

    // ========== 工具方法 ==========

    /**
     * @brief 获取开放节点列表（用于调试）
     */
    [[nodiscard]] const std::vector<PathPoint*>& getOpenNodes() const { return m_openNodes; }

protected:
    const Region* m_region = nullptr;
    f32 m_entityWidth = 0.6f;
    f32 m_entityHeight = 1.8f;
    std::unordered_map<u64, std::unique_ptr<PathPoint>> m_nodeCache;
    std::vector<PathPoint*> m_openNodes;

    /**
     * @brief 创建新节点
     */
    [[nodiscard]] virtual std::unique_ptr<PathPoint> createNode(i32 x, i32 y, i32 z) = 0;

    /**
     * @brief 生成哈希值
     */
    [[nodiscard]] static u64 makeHash(i32 x, i32 y, i32 z)
    {
        return (static_cast<u64>(static_cast<u16>(y)) << 48) | (static_cast<u64>(static_cast<u32>(x)) << 16) |
            static_cast<u64>(static_cast<u32>(z));
    }
};

} // namespace mc::entity::ai::pathfinding
