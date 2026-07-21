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

#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::client::renderer::entity::core {

/**
 * @brief 动画网格缓存
 *
 * 为动画实体管理网格更新。检测动画状态变化，按需重新生成网格。
 *
 * 工作流程：
 * 1. 每帧调用 getOrUpdateMesh() 检查动画状态
 * 2. 如果状态变化超过阈值，重新调用 model.generateMesh() + pipeline.updateMesh()
 * 3. 如果状态未变化，返回缓存的网格
 *
 * 在 Vulkan 中，我们不能每帧即时渲染，而是通过重新生成网格数据来实现动画。
 */
class AnimatedMeshCache {
public:
    AnimatedMeshCache();
    ~AnimatedMeshCache();

    // 禁止拷贝
    AnimatedMeshCache(const AnimatedMeshCache&) = delete;
    AnimatedMeshCache& operator=(const AnimatedMeshCache&) = delete;

    /**
     * @brief 获取或更新实体网格
     *
     * 根据动画状态决定是否需要重新生成网格。
     *
     * @param entityId 实体 ID
     * @param model 已设置动画角度的模型实例
     * @param typeId 实体类型 ID（用于纹理 UV 重映射）
     * @param state 当前动画状态
     * @param pipeline 渲染管线（用于创建/更新网格）
     * @return 指向网格的指针，如果创建失败返回 nullptr
     */
    pipeline::EntityMesh* getOrUpdateMesh(EntityInstanceId entityId,
        model::EntityModel& model,
        const std::string& typeId,
        const AnimationContext& state,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 移除实体的缓存网格
     */
    void removeEntity(EntityInstanceId entityId, pipeline::EntityPipeline* pipeline);

    /**
     * @brief 清除所有缓存
     */
    void clear(pipeline::EntityPipeline* pipeline);

    /**
     * @brief 获取缓存的实体数量
     */
    [[nodiscard]] Size size() const { return m_cache.size(); }

    /**
     * @brief 设置 UV 重映射回调
     *
     * 用于将模型局部 UV 映射到纹理图集区域。
     * 回调接收 entityId（玩家实体按此查皮肤区域）与 typeId（非玩家走类型纹理路径）。
     */
    using UvRemapFunc =
        std::function<void(EntityInstanceId entityId, const std::string& typeId, std::vector<model::ModelVertex>&)>;
    void setUvRemapFunc(UvRemapFunc func) { m_uvRemapFunc = std::move(func); }

private:
    /**
     * @brief 缓存的网格条目
     */
    struct CacheEntry {
        /// 实体网格数据
        pipeline::EntityMesh mesh;

        /// 上一次的动画状态
        AnimationContext lastState;

        /// 网格是否已创建
        bool created = false;

        /// 帧计数（用于延迟更新策略）
        u32 frameCount = 0;

        /// 距离上次更新的帧数
        u32 framesSinceUpdate = 0;
    };

    /// 实体 ID -> 缓存条目
    std::unordered_map<EntityInstanceId, CacheEntry> m_cache;

    /// UV 重映射回调
    UvRemapFunc m_uvRemapFunc;

    /// 动画更新策略参数
    static constexpr u32 ACTIVE_UPDATE_INTERVAL = 2;    ///< 活跃动画更新间隔（帧）
    static constexpr u32 IDLE_UPDATE_INTERVAL = 6;      ///< 非活跃动画更新间隔（帧）
    static constexpr u32 FORCE_UPDATE_INTERVAL = 12;    ///< 强制刷新间隔，防止长期漂移
    static constexpr f64 STATE_CHANGE_THRESHOLD = 0.08; ///< 动画参数变化阈值

    /// 缓存大小限制
    static constexpr Size MAX_CACHE_SIZE = 1024;
};

} // namespace mc::client::renderer::entity::core
