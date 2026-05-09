#include "EntityRenderer.hpp"
#include "../util/ShadowRenderer.hpp"
#include "../util/NameTagRenderer.hpp"
#include "common/entity/core/Entity.hpp"
#include "client/world/entity/ClientEntity.hpp"

namespace mc::client::renderer::entity::core {

void EntityRenderer::renderShadow(Entity& entity, f64 partialTicks) {
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

void EntityRenderer::renderNameTag(Entity& entity) {
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

void EntityRenderer::renderLayersPipelineClient(
    ::mc::client::ClientEntity& entity,
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
    ::mc::client::ClientEntity& entity,
    f64 partialTicks,
    VkCommandBuffer cmd,
    pipeline::EntityPipeline& pipeline)
{
    if (m_shadowSize <= 0.0 || m_shadowAlpha <= 0.0) {
        return;
    }

    // 使用 ClientEntity 的属性渲染阴影
    f64 shadowRadius = static_cast<f64>(entity.width()) * 0.5;
    util::ShadowRenderer::renderShadow(cmd, entity, partialTicks, shadowRadius, m_shadowAlpha, pipeline);
}

bool EntityRenderer::shouldRenderShadow(Entity& entity) const {
    // 检查实体是否可见（非隐身）
    // 参考 MC 1.16.5 EntityRenderer.shouldRenderShadow()
    if (entity.hasFlag(EntityFlags::Invisible)) {
        return false;
    }

    // 只有阴影大小大于0时才渲染
    if (m_shadowSize <= 0.0f || m_shadowAlpha <= 0.0f) {
        return false;
    }

    return true;
}

f64 EntityRenderer::getShadowScale(Entity& entity, f64 partialTicks) const {
    // 基础阴影大小
    f64 baseScale = static_cast<f64>(m_shadowSize);

    // 根据实体高度衰减阴影
    // 参考 MC 1.16.5: 阴影大小受实体到地面距离影响
    f64 entityHeight = static_cast<f64>(entity.height());

    // 如果实体太高，阴影消失
    if (entityHeight > 16.0) {
        return 0.0;
    }

    // 计算距离地面的高度
    // TODO: 使用射线检测获取实际地面高度
    // 暂时使用实体Y坐标作为近似
    f64 distanceToGround = 0.0;  // 假设在地面上

    // 阴影透明度随高度衰减
    f64 heightFactor = 1.0 - (distanceToGround / 16.0);
    if (heightFactor < 0.0) {
        heightFactor = 0.0;
    }

    (void)partialTicks;
    return baseScale * heightFactor;
}

// EntityRendererFactory 实现
std::unordered_map<std::string, EntityRendererFactory::CreatorFunc> EntityRendererFactory::s_creators;

std::unique_ptr<EntityRenderer> EntityRendererFactory::createRenderer(const std::string& typeId) {
    auto it = s_creators.find(typeId);
    if (it != s_creators.end()) {
        return it->second();
    }
    return nullptr;
}

void EntityRendererFactory::registerRenderer(const std::string& typeId, CreatorFunc creator) {
    s_creators[typeId] = creator;
}

} // namespace mc::client::renderer::entity::core
