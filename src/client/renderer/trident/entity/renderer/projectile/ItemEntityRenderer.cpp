#include "ItemEntityRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/Item.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::renderer::projectile {

ItemEntityRenderer::ItemEntityRenderer()
{
    // MC 1.16.5: ItemEntity 阴影大小为 0.15
    m_shadowSize = 0.15f;
    m_shadowAlpha = 0.75f;  // MC 1.16.5: shadowOpaque = 0.75F for items
}

void ItemEntityRenderer::render(Entity& entity, f64 partialTicks) {
    // ItemEntity 渲染由 EntityRendererManager::renderWithPipeline 处理
    // 这里是传统渲染路径，暂时不实现
    (void)entity;
    (void)partialTicks;
}

void ItemEntityRenderer::renderShadow(Entity& entity, f64 partialTicks) {
    // MC 1.16.5: ItemEntity 有阴影
    core::EntityRenderer::renderShadow(entity, partialTicks);
}

f64 ItemEntityRenderer::calculateBobOffset(u32 ticksExisted, f64 partialTick, f32 hoverStart) const {
    // MC 1.16.5 ItemRenderer.java:47:
    // f1 = MathHelper.sin(((float)entityIn.getAge() + partialTicks) / 10.0F + entityIn.hoverStart) * 0.1F + 0.1F
    //
    // 关键点：
    // 1. 使用 (age + partialTick) / 10.0 作为正弦参数
    // 2. 加上 hoverStart（每个物品实体随机生成，使不同物品浮动相位不同）
    // 3. 乘以 0.1 作为幅度
    // 4. 加上 0.1 作为基础高度偏移

    f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    f64 phase = ticks / 10.0 + static_cast<f64>(hoverStart);
    return std::sin(phase) * BOB_AMPLITUDE + BOB_BASE;
}

f64 ItemEntityRenderer::calculateRotation(u32 ticksExisted, f64 partialTick) const {
    // MC 1.16.5: 物品在 Y 轴旋转
    // 旋转速度为每 tick 2 度
    return static_cast<f64>(ticksExisted) * ROTATION_SPEED + partialTick * ROTATION_SPEED;
}

i32 ItemEntityRenderer::getItemCountForRender(i32 count) {
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
