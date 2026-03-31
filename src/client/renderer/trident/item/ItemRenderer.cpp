#include "ItemRenderer.hpp"
#include "../gui/GuiRenderer.hpp"
#include "../../../resource/ResourceManager.hpp"
#include "../../../resource/ItemTextureAtlas.hpp"
#include "../../../../common/item/core/Item.hpp"
#include "../../../../common/item/core/ItemStack.hpp"
#include "../../../../common/item/items/block/BlockItem.hpp"
#include "../../../../common/world/block/Block.hpp"
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident::item {

ItemRenderer::ItemRenderer()
    : m_resourceManager(nullptr)
    , m_itemTextureAtlas(nullptr)
    , m_initialized(false)
{
}

Result<void> ItemRenderer::initialize(
    ResourceManager* resourceManager,
    ItemTextureAtlas* itemTextureAtlas)
{
    if (resourceManager == nullptr) {
        return Error(ErrorCode::NullPointer, "ResourceManager is null");
    }

    m_resourceManager = resourceManager;
    m_itemTextureAtlas = itemTextureAtlas;
    m_initialized = true;

    spdlog::info("ItemRenderer: Initialized");
    return {};
}

void ItemRenderer::renderItem(gui::GuiRenderer& gui, const ItemStack& stack, f32 x, f32 y, f32 size) {
    if (stack.isEmpty()) {
        return;
    }

    const Item* item = stack.getItem();
    if (item == nullptr) {
        return;
    }

    renderItem(gui, item, x, y, size);
}

void ItemRenderer::renderItem(gui::GuiRenderer& gui, const Item* item, f32 x, f32 y, f32 size) {
    if (item == nullptr || !m_initialized) {
        return;
    }

    const TextureRegion* region = getItemTextureRegion(item);
    if (region == nullptr) {
        // 没有找到纹理，绘制占位符
        // 使用半透明纯色矩形表示缺失纹理
        gui.fillRect(x, y, size, size, 0x80FF0000);
        return;
    }

    renderItem(gui, *region, x, y, size);
}

void ItemRenderer::renderItem(gui::GuiRenderer& gui, const TextureRegion& region, f32 x, f32 y, f32 size) {
    // 使用drawTexturedRect绘制物品纹理
    // 使用 alpha=254 的颜色，确保走物品纹理采样分支且保持可见
    gui.drawTexturedRect(x, y, size, size,
                         region.u0, region.v0, region.u1, region.v1,
                         gui::GuiRenderer::ITEM_TEXTURE_COLOR);
}

bool ItemRenderer::isBlockItem(const Item* item) const {
    if (item == nullptr) {
        return false;
    }

    // 检查是否为BlockItem类型
    const BlockItem* blockItem = dynamic_cast<const BlockItem*>(item);
    return blockItem != nullptr;
}

const TextureRegion* ItemRenderer::getItemTextureRegion(const Item* item) const {
    if (item == nullptr || !m_initialized) {
        return nullptr;
    }

    // 统一优先使用物品图集，避免将方块图集UV错误用于GUI物品图集采样。
    if (m_itemTextureAtlas != nullptr) {
        if (const TextureRegion* region = m_itemTextureAtlas->getItemTexture(item->itemId())) {
            return region;
        }

        const ResourceLocation& itemId = item->itemLocation();
        const ResourceLocation itemPath(itemId.namespace_(), "item/" + itemId.path());
        if (const TextureRegion* region = m_itemTextureAtlas->getItemTexture(itemPath)) {
            return region;
        }

        const ResourceLocation itemTexturePath(itemId.namespace_(), "textures/item/" + itemId.path());
        if (const TextureRegion* region = m_itemTextureAtlas->getItemTexture(itemTexturePath)) {
            return region;
        }
    }

    // 首先检查是否为方块物品
    const BlockItem* blockItem = dynamic_cast<const BlockItem*>(item);
    if (blockItem != nullptr) {
        // 方块物品：尝试方块纹理路径别名（ItemTextureAtlas 在加载时会建立别名）
        const ResourceLocation& blockId = blockItem->block().blockLocation();
        if (m_itemTextureAtlas != nullptr) {
            const ResourceLocation blockPath(blockId.namespace_(), "block/" + blockId.path());
            if (const TextureRegion* region = m_itemTextureAtlas->getItemTexture(blockPath)) {
                return region;
            }

            const ResourceLocation blockTexturePath(blockId.namespace_(), "textures/block/" + blockId.path());
            if (const TextureRegion* region = m_itemTextureAtlas->getItemTexture(blockTexturePath)) {
                return region;
            }
        }

        // 注意：不要回退到方块图集UV。
        // GUI物品绘制固定采样“物品图集”，若返回方块图集的UV会导致采样错位。
    }

    return nullptr;
}

} // namespace mc::client::renderer::trident::item
