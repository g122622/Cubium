#include "ItemEntityRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::renderer::projectile {

ItemEntityRenderer::ItemEntityRenderer()
{
    // MC 1.16.5: ItemEntity 阴影大小为 0.15
    m_shadowSize = 0.15f;
    m_shadowAlpha = 0.75f; // MC 1.16.5: shadowOpaque = 0.75F for items
}

void ItemEntityRenderer::render(Entity& entity, f64 partialTicks)
{
    // ItemEntity 渲染由 EntityRendererManager::renderWithPipeline 处理
    // 这里是传统渲染路径，暂时不实现
    (void)entity;
    (void)partialTicks;
}

void ItemEntityRenderer::renderShadow(Entity& entity, f64 partialTicks)
{
    // MC 1.16.5: ItemEntity 有阴影
    core::EntityRenderer::renderShadow(entity, partialTicks);
}

f64 ItemEntityRenderer::calculateBobOffset(u32 ticksExisted, f64 partialTick, f32 hoverStart)
{
    const f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    return std::sin(ticks / 10.0 + static_cast<f64>(hoverStart)) * BOB_AMPLITUDE + BOB_BASE + GROUND_TRANSFORM_Y_OFFSET;
}

f64 ItemEntityRenderer::calculateRotation(u32 ticksExisted, f64 partialTick, f32 hoverStart)
{
    return ((static_cast<f64>(ticksExisted) + partialTick) / 20.0 + static_cast<f64>(hoverStart)) *
        static_cast<f64>(math::RAD_TO_DEG);
}

i32 ItemEntityRenderer::getItemCountForRender(i32 count)
{
    // MC 1.16.5 ItemRenderer.getRenderAmount():
    // if (count <= 1) return 1;
    // else if (count <= 16) return 2;
    // else if (count <= 32) return 3;
    // else if (count <= 48) return 4;
    // else return 5;

    if (count <= 1) {
        return 1;
    } else if (count <= 16) {
        return 2;
    } else if (count <= 32) {
        return 3;
    } else if (count <= 48) {
        return 4;
    } else {
        return 5;
    }
}

const TextureRegion* ItemEntityRenderer::getItemTextureRegion(const ItemStack& stack) const
{
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
