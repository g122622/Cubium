#include "ItemEntityRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/Item.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::renderer::projectile {

ItemEntityRenderer::ItemEntityRenderer()
{
    // ItemEntity 通常没有阴影
    m_shadowSize = 0.0f;
    m_shadowAlpha = 0.0f;
}

void ItemEntityRenderer::render(Entity& entity, f64 partialTicks) {
    // ItemEntity 渲染由 EntityRendererManager::renderWithPipeline 处理
    // 这里是传统渲染路径，暂时不实现
    (void)entity;
    (void)partialTicks;
}

void ItemEntityRenderer::renderShadow(Entity& entity, f64 partialTicks) {
    // ItemEntity 没有阴影
    (void)entity;
    (void)partialTicks;
}

f64 ItemEntityRenderer::calculateBobOffset(u32 ticksExisted, f64 partialTick) const {
    f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    return std::sin(ticks * BOB_FREQUENCY) * BOB_AMPLITUDE;
}

f64 ItemEntityRenderer::calculateRotation(u32 ticksExisted, f64 partialTick) const {
    return static_cast<f64>(ticksExisted) * ROTATION_SPEED + partialTick * ROTATION_SPEED;
}

const TextureRegion* ItemEntityRenderer::getItemTextureRegion(const ItemStack& stack) const {
    if (stack.isEmpty() || m_itemTextureAtlas == nullptr) {
        return nullptr;
    }

    const Item* item = stack.getItem();
    if (item == nullptr) {
        return nullptr;
    }

    // 尝试使用资源路径获取纹理
    const ResourceLocation& itemId = item->itemLocation();

    // 尝试 "item/" 前缀
    ResourceLocation itemPath(itemId.namespace_(), "item/" + itemId.path());
    const TextureRegion* region = m_itemTextureAtlas->getRegion(itemPath);
    if (region != nullptr) {
        return region;
    }

    // 尝试完整路径
    ResourceLocation itemTexturePath(itemId.namespace_(), "textures/item/" + itemId.path());
    region = m_itemTextureAtlas->getRegion(itemTexturePath);
    if (region != nullptr) {
        return region;
    }

    return nullptr;
}

} // namespace mc::client::renderer::entity::renderer::projectile
