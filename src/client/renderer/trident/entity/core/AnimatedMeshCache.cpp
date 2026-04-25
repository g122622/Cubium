#include "AnimatedMeshCache.hpp"
#include "../model/core/ModelRenderer.hpp"
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::core {

AnimatedMeshCache::AnimatedMeshCache() = default;

AnimatedMeshCache::~AnimatedMeshCache() {
    clear();
}

pipeline::EntityMesh* AnimatedMeshCache::getOrUpdateMesh(
    EntityId entityId,
    model::EntityModel& model,
    const String& typeId,
    const AnimationContext& state,
    pipeline::EntityPipeline& pipeline
) {
    // 查找现有缓存
    auto it = m_cache.find(entityId);

    if (it == m_cache.end()) {
        // 新实体，创建缓存条目
        if (m_cache.size() >= MAX_CACHE_SIZE) {
            // 缓存已满，移除最旧的条目（简单的 FIFO 策略）
            // 在实际应用中可以使用 LRU 策略
            spdlog::warn("AnimatedMeshCache: Cache full, removing oldest entry");
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

    if (!entry.created) {
        // 首次创建
        needsUpdate = true;
    } else if (state.hasSignificantChange(entry.lastState, STATE_CHANGE_THRESHOLD)) {
        // 动画状态显著变化
        needsUpdate = true;
    } else if (entry.framesSinceUpdate >= MIN_UPDATE_INTERVAL) {
        // 到达更新间隔（确保至少每 N 帧更新一次）
        // 对于持续动画（如行走），这确保动画流畅
        if (state.limbSwingAmount > 0.01 || std::abs(state.netHeadYaw) > 1.0 || std::abs(state.headPitch) > 1.0) {
            needsUpdate = true;
        }
    }

    if (needsUpdate) {
        // 生成新的网格数据
        std::vector<model::ModelVertex> vertices;
        std::vector<u32> indices;

        // 调用模型的 generateMesh 方法，这会使用当前设置的旋转角度
        model.generateMesh(vertices, indices, state.scale);

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
                spdlog::error("AnimatedMeshCache: Failed to create mesh for entity {}: {}",
                             entityId, result.error().message());
                return nullptr;
            }
        } else {
            // 更新现有网格
            auto result = pipeline.updateMesh(entry.mesh, vertices, indices);
            if (!result.success()) {
                spdlog::warn("AnimatedMeshCache: Failed to update mesh for entity {}: {}",
                            entityId, result.error().message());
                // 更新失败时重新创建
                auto createResult = pipeline.createMesh(vertices, indices);
                if (createResult.success()) {
                    entry.mesh = std::move(createResult.value());
                }
            }
        }

        // 更新缓存的状态
        entry.lastState = state;
        entry.framesSinceUpdate = 0;
    }

    return entry.created ? &entry.mesh : nullptr;
}

void AnimatedMeshCache::removeEntity(EntityId entityId) {
    auto it = m_cache.find(entityId);
    if (it != m_cache.end()) {
        // 销毁网格资源（如果需要）
        // 注意：EntityMesh 的析构函数应该处理 Vulkan 资源释放
        m_cache.erase(it);
    }
}

void AnimatedMeshCache::clear() {
    m_cache.clear();
}

} // namespace mc::client::renderer::entity::core
