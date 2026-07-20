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

#include "client/renderer/trident/entity/core/AnimatedMeshCache.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::core {

AnimatedMeshCache::AnimatedMeshCache() = default;

AnimatedMeshCache::~AnimatedMeshCache()
{
    clear(nullptr);
}

pipeline::EntityMesh* AnimatedMeshCache::getOrUpdateMesh(EntityInstanceId entityId,
    model::EntityModel& model,
    const std::string& typeId,
    const AnimationContext& state,
    pipeline::EntityPipeline& pipeline)
{
    // 查找现有缓存
    auto it = m_cache.find(entityId);

    if (it == m_cache.end()) {
        // 新实体，创建缓存条目
        if (m_cache.size() >= MAX_CACHE_SIZE) {
            // 缓存已满，移除最旧的条目（简单的 FIFO 策略）
            // 在实际应用中可以使用 LRU 策略
            spdlog::warn("AnimatedMeshCache: Cache full, removing oldest entry");
            if (m_cache.begin()->second.created) {
                pipeline.destroyMesh(m_cache.begin()->second.mesh);
            }
            m_cache.erase(m_cache.begin());
        }

        // 创建新条目
        auto [newIt, inserted] = m_cache.emplace(entityId, CacheEntry{});
        it = newIt;
        it->second.lastState = state;
        it->second.created = false;
        it->second.frameCount = 0;
        it->second.framesSinceUpdate = 0;
    }

    CacheEntry& entry = it->second;
    entry.frameCount++;
    entry.framesSinceUpdate++;

    // 检查是否需要更新网格
    bool needsUpdate = false;
    const bool postureChanged = state.isSitting != entry.lastState.isSitting ||
        state.isChild != entry.lastState.isChild || state.isSneaking != entry.lastState.isSneaking ||
        state.isSwimming != entry.lastState.isSwimming || state.isRiding != entry.lastState.isRiding ||
        std::abs(state.standingProgress - entry.lastState.standingProgress) > static_cast<f32>(STATE_CHANGE_THRESHOLD);
    const bool hasActiveAnimation = state.limbSwingAmount > 0.01 || std::abs(state.netHeadYaw) > 1.0 ||
        std::abs(state.headPitch) > 1.0 || state.swingProgress > 0.001f || state.standingProgress > 0.001f;
    const u32 updateInterval = hasActiveAnimation ? ACTIVE_UPDATE_INTERVAL : IDLE_UPDATE_INTERVAL;

    if (!entry.created) {
        // 首次创建
        needsUpdate = true;
    } else if (postureChanged) {
        // 姿态状态变化需要立即更新，避免模型状态滞后
        needsUpdate = true;
    } else if (entry.framesSinceUpdate >= updateInterval &&
        state.hasSignificantChange(entry.lastState, STATE_CHANGE_THRESHOLD)) {
        // 动画有明显变化且达到更新间隔
        needsUpdate = true;
    } else if (hasActiveAnimation && entry.framesSinceUpdate >= FORCE_UPDATE_INTERVAL) {
        // 长时间未刷新时强制更新，避免动画累计误差
        needsUpdate = true;
    }

    if (needsUpdate) {
        // 生成新的网格数据
        std::vector<model::ModelVertex> vertices;
        std::vector<u32> indices;

        // 实体管线在着色器 push constant 中统一应用 1/16 缩放，CPU 网格保持 MC 模型单位。
        model.generateMesh(vertices, indices, 1.0);

        // 应用 UV 重映射（如果设置）
        if (m_uvRemapFunc) {
            m_uvRemapFunc(typeId, vertices);
        }

        // 检查网格是否有效
        if (vertices.empty() || indices.empty()) {
            // 空网格，返回现有的（如果有）
            if (entry.created) {
                return &entry.mesh;
            }
            return nullptr;
        }

        if (!entry.created) {
            // 首次创建网格
            auto result = pipeline.createMesh(vertices, indices);
            if (result.success()) {
                entry.mesh = std::move(result.value());
                entry.created = true;
            } else {
                spdlog::error(
                    "AnimatedMeshCache: Failed to create mesh for entity {}: {}", entityId, result.error().message());
                return nullptr;
            }
        } else {
            // 更新现有网格
            auto result = pipeline.updateMesh(entry.mesh, vertices, indices);
            if (!result.success()) {
                spdlog::warn(
                    "AnimatedMeshCache: Failed to update mesh for entity {}: {}", entityId, result.error().message());

                // 安全重建：先销毁旧 mesh，再创建新 mesh，避免资源泄漏
                pipeline.destroyMesh(entry.mesh);
                entry.created = false;

                auto createResult = pipeline.createMesh(vertices, indices);
                if (createResult.success()) {
                    entry.mesh = std::move(createResult.value());
                    entry.created = true;
                } else {
                    spdlog::error("AnimatedMeshCache: Failed to recreate mesh for entity {}: {}",
                        entityId,
                        createResult.error().message());
                    return nullptr;
                }
            }
        }

        // 更新缓存的状态
        entry.lastState = state;
        entry.framesSinceUpdate = 0;
    }

    return entry.created ? &entry.mesh : nullptr;
}

void AnimatedMeshCache::removeEntity(EntityInstanceId entityId, pipeline::EntityPipeline* pipeline)
{
    auto it = m_cache.find(entityId);
    if (it != m_cache.end()) {
        if (pipeline != nullptr && it->second.created) {
            pipeline->destroyMesh(it->second.mesh);
        }
        m_cache.erase(it);
    }
}

void AnimatedMeshCache::clear(pipeline::EntityPipeline* pipeline)
{
    if (pipeline != nullptr) {
        for (auto& [entityId, entry] : m_cache) {
            if (entry.created) {
                pipeline->destroyMesh(entry.mesh);
            }
        }
    }
    m_cache.clear();
}

} // namespace mc::client::renderer::entity::core
