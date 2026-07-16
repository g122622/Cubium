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

#include "Dimension.hpp"
#include "common/core/Result.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {

/**
 * @brief 维度管理器
 *
 * 管理所有维度实例的注册表，提供维度访问、遍历等功能。
 * 这是维度系统的基础设施，服务端和客户端都可使用。
 *
 * 使用示例:
 * @code
 * DimensionManager manager;
 *
 * // 访问维度
 * Dimension* overworld = manager.getDimension(DimensionManager::OVERWORLD);
 *
 * // 遍历所有维度
 * manager.forEachDimension([](Dimension& dim) {
 *     // 处理每个维度
 * });
 * @endcode
 */
class DimensionManager {
public:
    // ========== 维度ID常量 ==========
    // 主世界 = 0，下界 = -1 (存档目录 DIM-1)，末地 = 1 (存档目录 DIM1)

    /// 主世界维度ID
    static constexpr DimensionId OVERWORLD = 0;

    /// 下界维度ID
    static constexpr DimensionId NETHER = -1;

    /// 末地维度ID
    static constexpr DimensionId THE_END = 1;

    // ========== 构造与析构 ==========

    DimensionManager() = default;
    virtual ~DimensionManager() = default;

    // 禁止拷贝
    DimensionManager(const DimensionManager&) = delete;
    DimensionManager& operator=(const DimensionManager&) = delete;

    // 允许移动
    DimensionManager(DimensionManager&&) noexcept = default;
    DimensionManager& operator=(DimensionManager&&) noexcept = default;

    // ========== 生命周期 ==========

    /**
     * @brief 关闭维度管理器
     *
     * 清理所有维度实例。
     */
    void shutdown();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    // ========== 维度注册 ==========

    /**
     * @brief 注册维度
     *
     * @param dimension 维度实例
     * @return 是否注册成功
     */
    bool registerDimension(std::unique_ptr<Dimension> dimension);

    /**
     * @brief 注销维度
     *
     * @param id 维度ID
     * @return 是否注销成功
     */
    bool unregisterDimension(DimensionId id);

    // ========== 维度访问 ==========

    /**
     * @brief 获取维度
     *
     * @param id 维度ID
     * @return 维度指针，如果不存在则返回 nullptr
     */
    [[nodiscard]] Dimension* getDimension(DimensionId id);
    [[nodiscard]] const Dimension* getDimension(DimensionId id) const;

    /**
     * @brief 检查维度是否存在
     */
    [[nodiscard]] bool hasDimension(DimensionId id) const;

    /**
     * @brief 获取主世界维度
     */
    [[nodiscard]] Dimension* getOverworld();
    [[nodiscard]] const Dimension* getOverworld() const;

    /**
     * @brief 获取下界维度
     */
    [[nodiscard]] Dimension* getNether();
    [[nodiscard]] const Dimension* getNether() const;

    /**
     * @brief 获取末地维度
     */
    [[nodiscard]] Dimension* getTheEnd();
    [[nodiscard]] const Dimension* getTheEnd() const;

    // ========== 维度类型 ==========

    /**
     * @brief 获取维度类型
     *
     * @param id 维度ID
     * @return 维度类型指针，如果不存在则返回 nullptr
     */
    [[nodiscard]] const DimensionType* getDimensionType(DimensionId id) const;

    /**
     * @brief 根据名称获取维度ID
     *
     * @param name 维度名称（如 "minecraft:overworld"）
     * @return 维度ID，如果不存在则返回 -1
     */
    [[nodiscard]] DimensionId getDimensionIdByName(const std::string& name) const;

    // ========== 遍历 ==========

    /**
     * @brief 遍历所有维度
     *
     * @param func 对每个维度调用的函数
     */
    void forEachDimension(std::function<void(Dimension&)> func);
    void forEachDimension(std::function<void(const Dimension&)> func) const;

    // ========== 信息 ==========

    /**
     * @brief 获取所有注册的维度ID
     */
    [[nodiscard]] std::vector<DimensionId> getDimensionIds() const;

    /**
     * @brief 获取维度数量
     */
    [[nodiscard]] size_t dimensionCount() const { return m_dimensions.size(); }

    /**
     * @brief 获取默认出生维度
     *
     * 默认为主世界。
     */
    [[nodiscard]] DimensionId defaultSpawnDimension() const { return OVERWORLD; }

protected:
    std::unordered_map<DimensionId, std::unique_ptr<Dimension>> m_dimensions;
    std::unordered_map<std::string, DimensionId> m_nameToId;
    bool m_initialized = false;
};

} // namespace mc
