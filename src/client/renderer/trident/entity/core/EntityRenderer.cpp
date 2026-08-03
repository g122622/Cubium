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

#include "EntityRenderer.hpp"
#include "client/renderer/trident/entity/util/NameTagRenderer.hpp"
#include "client/renderer/trident/entity/util/ShadowRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/ray/Ray.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/IWorld.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::core {

void EntityRenderer::renderShadow(Entity& entity, f64 partialTicks)
{
    if (!shouldRenderShadow(entity)) {
        return;
    }

    f64 scale = getShadowScale(entity, partialTicks);
    if (scale <= 0.0) {
        return;
    }

    // 调用阴影渲染器
    util::ShadowRenderer::renderShadow(entity, partialTicks, m_shadowSize, m_shadowAlpha);
}

void EntityRenderer::renderNameTag(Entity& entity)
{
    // 获取实体显示名称
    const std::string displayName = entity.customNameText();
    if (displayName.empty()) {
        return;
    }

    // 检查是否应该渲染名称标签
    if (!util::NameTagRenderer::shouldRenderNameTag(entity, 0.0)) {
        return;
    }

    // 渲染名称标签
    util::NameTagRenderer::renderNameTag(entity, displayName, 0.0);
}

void EntityRenderer::renderLayersPipelineClient(::mc::client::ClientEntity& entity,
    VkCommandBuffer cmd,
    const AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 默认实现：空操作
    // 子类（如 LivingRenderer 和 PlayerRenderer）应重写此方法
    (void)entity;
    (void)cmd;
    (void)context;
    (void)pipeline;
}

void EntityRenderer::renderShadowClient(
    ::mc::client::ClientEntity& entity, f64 partialTicks, VkCommandBuffer cmd, pipeline::EntityPipeline& pipeline)
{
    if (m_shadowSize <= 0.0 || m_shadowAlpha <= 0.0) {
        return;
    }

    // 使用 ClientEntity 的属性渲染阴影
    f64 shadowRadius = static_cast<f64>(entity.width()) * 0.5;
    util::ShadowRenderer::renderShadow(cmd, entity, partialTicks, shadowRadius, m_shadowAlpha, pipeline);
}

bool EntityRenderer::shouldRenderShadow(Entity& entity) const
{
    // 检查实体是否可见（非隐身）
    if (entity.hasFlag(EntityFlags::Invisible)) {
        return false;
    }

    // 只有阴影大小大于0时才渲染
    if (m_shadowSize <= 0.0 || m_shadowAlpha <= 0.0) {
        return false;
    }

    return true;
}

f64 EntityRenderer::getShadowScale(Entity& entity, f64 partialTicks) const
{
    // 阴影在实体距离地面超过此高度时完全消失
    static constexpr f64 SHADOW_MAX_HEIGHT = 16.0;
    // 射线检测距离，略大于 SHADOW_MAX_HEIGHT 以确保检测到地面
    static constexpr f32 SHADOW_RAYCAST_DISTANCE = 17.0f;

    // 基础阴影大小
    f64 baseScale = static_cast<f64>(m_shadowSize);

    // 根据实体高度衰减阴影
    f64 entityHeight = static_cast<f64>(entity.height());

    // 如果实体太高，阴影消失
    if (entityHeight > SHADOW_MAX_HEIGHT) {
        return 0.0;
    }

    // 计算距离地面的高度
    // 使用射线检测从实体位置向下发射射线，获取实际地面高度
    f64 distanceToGround = 0.0;

    IWorld* world = entity.world();
    if (world) {
        // 从实体脚部位置向下发射射线
        Vector3f origin(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));
        Vector3f direction(0.0f, -1.0f, 0.0f); // 向下

        Ray ray(origin, direction);
        RaycastContext context(ray, SHADOW_RAYCAST_DISTANCE);

        BlockRaycastResult result = raycastBlocks(context, *world);
        if (result.isHit()) {
            // 击中地面，计算距离
            distanceToGround = static_cast<f64>(result.distance());
        } else {
            // 未击中地面，假设距离超过阈值
            distanceToGround = static_cast<f64>(SHADOW_RAYCAST_DISTANCE);
        }
    }

    // 阴影透明度随高度衰减
    f64 heightFactor = 1.0 - (distanceToGround / SHADOW_MAX_HEIGHT);
    if (heightFactor < 0.0) {
        heightFactor = 0.0;
    }

    (void)partialTicks;
    return baseScale * heightFactor;
}

// EntityRendererFactory 实现
std::unordered_map<std::string, EntityRendererFactory::CreatorFunc> EntityRendererFactory::s_creators;

std::unique_ptr<EntityRenderer> EntityRendererFactory::createRenderer(const std::string& typeId)
{
    auto it = s_creators.find(typeId);
    if (it != s_creators.end()) {
        return it->second();
    }
    return nullptr;
}

void EntityRendererFactory::registerRenderer(const std::string& typeId, CreatorFunc creator)
{
    s_creators[typeId] = creator;
}

} // namespace mc::client::renderer::entity::core
