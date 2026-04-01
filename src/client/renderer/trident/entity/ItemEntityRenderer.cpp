#include "ItemEntityRenderer.hpp"
#include "../../../world/entity/ClientEntity.hpp"
#include "../../../resource/ItemTextureAtlas.hpp"
#include "../item/ItemRenderer.hpp"
#include "../../../../common/item/core/ItemStack.hpp"
#include "../../../../common/item/core/Item.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer {

namespace {
    // ItemEntity 动画常量（参考 MC 1.16.5）
    constexpr f64 BOB_AMPLITUDE = 0.1f;       // 浮动高度
    constexpr f64 BOB_FREQUENCY = 0.1f;       // 浮动速度（弧度/tick）
    constexpr f64 ROTATION_SPEED = 2.0f;      // 旋转速度（度/tick）
    constexpr f64 GROUND_OFFSET = 0.25f;      // 地面高度偏移
    constexpr f64 ITEM_SIZE = 0.25f;          // 渲染大小
}

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

    // 尝试通过物品 ID 获取纹理
    const TextureRegion* region = m_itemTextureAtlas->getItemTexture(item->itemId());
    if (region != nullptr) {
        return region;
    }

    // 尝试使用资源路径获取纹理
    const ResourceLocation& itemId = item->itemLocation();

    // 尝试 "item/" 前缀
    ResourceLocation itemPath(itemId.namespace_(), "item/" + itemId.path());
    region = m_itemTextureAtlas->getItemTexture(itemPath);
    if (region != nullptr) {
        return region;
    }

    // 尝试完整路径
    ResourceLocation itemTexturePath(itemId.namespace_(), "textures/item/" + itemId.path());
    region = m_itemTextureAtlas->getItemTexture(itemTexturePath);
    if (region != nullptr) {
        return region;
    }

    return nullptr;
}

} // namespace mc::client::renderer
