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

#include "Placement.hpp"
#include "Placements.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

/**
 * @brief 放置器注册表
 *
 * 管理所有已注册的放置器类型。
 */
class PlacementRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static PlacementRegistry& instance() noexcept;

    /**
     * @brief 初始化注册表
     *
     * 注册所有内置放置器。
     */
    void initialize();

    /**
     * @brief 注册放置器
     * @param name 放置器名称
     * @param placement 放置器实例
     */
    void registerPlacement(const std::string& name, std::unique_ptr<Placement> placement);

    /**
     * @brief 获取放置器
     * @param name 放置器名称
     * @return 放置器指针，如果不存在则返回 nullptr
     */
    [[nodiscard]] const Placement* get(const std::string& name) const noexcept;

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    /**
     * @brief 获取所有已注册的放置器名称
     */
    [[nodiscard]] std::vector<std::string> getNames() const;

private:
    PlacementRegistry() = default;
    ~PlacementRegistry() = default;

    PlacementRegistry(const PlacementRegistry&) = delete;
    PlacementRegistry& operator=(const PlacementRegistry&) = delete;

    std::unordered_map<std::string, std::unique_ptr<Placement>> m_placements;
    bool m_initialized = false;
};

/**
 * @brief 放置器类型工厂
 *
 * 提供创建放置器的静态方法。
 */
namespace Placements {

/**
 * @brief 创建数量放置器
 */
std::unique_ptr<Placement> count();

/**
 * @brief 创建高度范围放置器
 */
std::unique_ptr<Placement> heightRange();

/**
 * @brief 创建方形分散放置器
 */
std::unique_ptr<Placement> square();

/**
 * @brief 创建生物群系过滤放置器
 */
std::unique_ptr<Placement> biome();

/**
 * @brief 创建概率放置器
 */
std::unique_ptr<Placement> chance();

/**
 * @brief 创建地表放置器
 */
std::unique_ptr<Placement> surface();

/**
 * @brief 创建噪声阈值放置器
 */
std::unique_ptr<Placement> noise();

/**
 * @brief 创建噪声数量放置器
 */
std::unique_ptr<Placement> countNoise();

/**
 * @brief 创建深度平均放置器
 */
std::unique_ptr<Placement> depthAverage();

/**
 * @brief 创建顶层固体放置器
 */
std::unique_ptr<Placement> topSolid();

/**
 * @brief 创建雕刻掩码放置器
 */
std::unique_ptr<Placement> carvingMask();

/**
 * @brief 创建随机偏移放置器
 */
std::unique_ptr<Placement> randomOffset();

/**
 * @brief 创建水深阈值放置器
 */
std::unique_ptr<Placement> waterDepthThreshold();

/**
 * @brief 创建海平面放置器
 */
std::unique_ptr<Placement> seaLevel();

/**
 * @brief 创建扩散放置器
 */
std::unique_ptr<Placement> spread();

} // namespace Placements

} // namespace mc
