#include "EntityRenderer.hpp"
#include "../util/ShadowRenderer.hpp"
#include "../util/NameTagRenderer.hpp"
#include "common/entity/core/Entity.hpp"

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
    const String& displayName = entity.customName();
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

bool EntityRenderer::shouldRenderShadow(Entity& entity) const {
    // 检查实体是否在地面上方一定距离内
    // 只有实体可见且阴影大小大于0时才渲染
    if (m_shadowSize <= 0.0f || m_shadowAlpha <= 0.0f) {
        return false;
    }

    // 检查实体是否可见（非隐身）
    // TODO: 从实体获取隐身状态
    (void)entity;
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
std::unordered_map<String, EntityRendererFactory::CreatorFunc> EntityRendererFactory::s_creators;

std::unique_ptr<EntityRenderer> EntityRendererFactory::createRenderer(const String& typeId) {
    auto it = s_creators.find(typeId);
    if (it != s_creators.end()) {
        return it->second();
    }
    return nullptr;
}

void EntityRendererFactory::registerRenderer(const String& typeId, CreatorFunc creator) {
    s_creators[typeId] = creator;
}

} // namespace mc::client::renderer::entity::core
