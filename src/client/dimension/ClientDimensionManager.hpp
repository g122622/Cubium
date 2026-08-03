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
#include "common/util/math/Vector3.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

// 前向声明
namespace client {
class ClientWorld;
}

/**
 * @brief 客户端维度信息结构体
 *
 * 存储从服务器接收的维度信息，包含维度属性。
 */
struct ClientDimensionInfo {
    DimensionId id = DimensionManager::OVERWORLD; ///< 维度ID
    std::string name;                             ///< 维度名称 (如 "minecraft:overworld")
    bool hasSkyLight = true;                      ///< 是否有天空光照
    bool hasCeiling = false;                      ///< 是否有天花板
    f32 ambientLight = 0.0f;                      ///< 环境光照强度
};

/**
 * @brief 客户端维度管理器
 *
 * 管理客户端的维度状态，处理维度切换。
 */
class ClientDimensionManager {
public:
    /**
     * @brief 维度切换状态
     */
    enum class TransitionState {
        None,    ///< 无切换
        Leaving, ///< 正在离开当前维度
        Loading, ///< 正在加载新维度
        Entering ///< 正在进入新维度
    };

    ClientDimensionManager();
    ~ClientDimensionManager() = default;

    // 禁止拷贝
    ClientDimensionManager(const ClientDimensionManager&) = delete;
    ClientDimensionManager& operator=(const ClientDimensionManager&) = delete;

    // ========== 初始化 ==========

    /**
     * @brief 初始化维度信息（仅ID列表）
     *
     * @param dimensionInfo 从服务器接收的维度ID列表
     * @deprecated 请使用 initialize(const std::vector<ClientDimensionInfo>&) 代替
     */
    void initialize(const std::vector<DimensionId>& dimensionInfo);

    /**
     * @brief 初始化维度信息（完整信息）
     *
     * @param dimensionInfo 从服务器接收的完整维度信息列表
     */
    void initialize(const std::vector<ClientDimensionInfo>& dimensionInfo);

    /**
     * @brief 重置状态
     */
    void reset();

    // ========== 当前维度 ==========

    /**
     * @brief 获取当前维度ID
     */
    [[nodiscard]] DimensionId currentDimension() const { return m_currentDimension; }

    /**
     * @brief 设置当前维度
     */
    void setCurrentDimension(DimensionId dimension);

    /**
     * @brief 获取当前维度类型
     */
    [[nodiscard]] const DimensionType* currentDimensionType() const;

    // ========== 维度切换 ==========

    /**
     * @brief 开始维度切换
     *
     * @param targetDimension 目标维度ID
     * @param position 目标位置
     */
    void beginDimensionChange(DimensionId targetDimension, const Vector3d& position);

    /**
     * @brief 完成维度切换
     *
     * 在客户端加载完新区块后调用。
     */
    void completeDimensionChange();

    /**
     * @brief 取消维度切换
     */
    void cancelDimensionChange();

    /**
     * @brief 获取切换状态
     */
    [[nodiscard]] TransitionState transitionState() const { return m_transitionState; }

    /**
     * @brief 是否正在切换维度
     */
    [[nodiscard]] bool isChangingDimension() const { return m_transitionState != TransitionState::None; }

    /**
     * @brief 获取目标维度
     */
    [[nodiscard]] DimensionId targetDimension() const { return m_targetDimension; }

    /**
     * @brief 获取目标位置
     */
    [[nodiscard]] const Vector3d& targetPosition() const { return m_targetPosition; }

    // ========== 维度信息 ==========

    /**
     * @brief 获取可用维度ID列表
     */
    [[nodiscard]] const std::vector<DimensionId>& availableDimensions() const { return m_availableDimensionIds; }

    /**
     * @brief 获取可用维度信息列表
     */
    [[nodiscard]] const std::vector<ClientDimensionInfo>& availableDimensionInfos() const
    {
        return m_availableDimensions;
    }

    /**
     * @brief 检查维度是否可用
     */
    [[nodiscard]] bool isDimensionAvailable(DimensionId dimension) const;

    /**
     * @brief 获取维度信息
     *
     * @param dimension 维度ID
     * @return 维度信息，如果维度不存在则返回 nullptr
     */
    [[nodiscard]] const ClientDimensionInfo* getDimensionInfo(DimensionId dimension) const;

    /**
     * @brief 获取维度类型
     *
     * @param dimension 维度ID
     * @return 维度类型，如果维度不存在则返回 nullptr
     */
    [[nodiscard]] const DimensionType* getDimensionType(DimensionId dimension) const;

    // ========== 渲染设置 ==========

    /**
     * @brief 是否需要清除渲染状态
     *
     * 维度切换时需要清除区块缓存等。
     */
    [[nodiscard]] bool needsRenderReset() const { return m_needsRenderReset; }

    /**
     * @brief 标记渲染已重置
     */
    void markRenderReset() { m_needsRenderReset = false; }

private:
    DimensionId m_currentDimension = DimensionManager::OVERWORLD;
    DimensionId m_targetDimension = DimensionManager::OVERWORLD;
    Vector3d m_targetPosition;
    TransitionState m_transitionState = TransitionState::None;

    // 完整的维度信息列表（从服务器接收）
    std::vector<ClientDimensionInfo> m_availableDimensions;
    // 维度ID列表（快速查找用）
    std::vector<DimensionId> m_availableDimensionIds;
    // 维度ID到维度信息的映射（快速查找用）
    std::unordered_map<DimensionId, size_t> m_dimensionIndexMap;

    bool m_needsRenderReset = false;
};

} // namespace mc
